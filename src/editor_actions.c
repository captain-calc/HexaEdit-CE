// Name:    Captain Calc
// Date:    July 13, 2021,
// File:    editor_actions.c
// Purpose: Provides the definitions of the functions declared in
//          editor_actions.h.


#include "asmutil.h"
#include "colors.h"
#include "editor.h"
#include "editor_actions.h"
#include "gui.h"
#include "settings.h"

#include <graphx.h>
#include <tice.h>

#include <math.h>
#include <stdint.h>
#include <string.h>


/* Define common error messages as globals. */
const char *EDIT_FILE_OPEN_FAIL = "Could not open edit file";
const char *EDIT_FILE_RESIZE_FAIL = "Could not resize edit file";
const char *UNDO_APPVAR_OPEN_FAIL = "Could not open undo appvar";
const char *UNDO_APPVAR_RESIZE_FAIL = "Could not resize undo appvar";


void editact_SpriteViewer(uint8_t *ptr, uint8_t *max_address)
{
  const uint8_t MAX_SPRITE_HEIGHT    = 245;
  const uint8_t MAX_SCALE_ONE_WIDTH  = 115;
  const uint8_t MAX_SCALE_ONE_HEIGHT = 115;
  
	gfx_sprite_t *sprite_data;
	uint8_t sprite_width, sprite_height;
	uint24_t sprite_size;
	uint24_t xPos = 0;
	uint8_t yPos = 0;
	uint8_t scale = 2;
	
  if (ptr == max_address)
    return;
  
	// Get the sprite's size
	sprite_width = *ptr;
	sprite_height = *(ptr + 1);
	sprite_size = sprite_width * sprite_height;
	
	if (sprite_size == 0 || sprite_height > MAX_SPRITE_HEIGHT)
		return;
	
	if ((uint24_t)(max_address - ptr) < sprite_size)
		return;
	
	if (!(sprite_data = gfx_MallocSprite(sprite_width, sprite_height)))
		return;
	
	// Set the scale to one if the sprite is very large
	if (sprite_width > MAX_SCALE_ONE_WIDTH || sprite_height > MAX_SCALE_ONE_HEIGHT)
		scale = 1;
	
	asm_CopyData(ptr + 2, sprite_data->data, sprite_size, 1);
	
	// Draw rectangle border
	gfx_SetColor(WHITE);
	gfx_FillRectangle_NoClip(xPos, yPos, sprite_width * scale + 10, sprite_height * scale + 10);
	gfx_SetColor(BLACK);
	gfx_FillRectangle_NoClip(xPos + 2, yPos + 2, sprite_width * scale + 6, sprite_height * scale + 6);
	gfx_SetColor(WHITE);
	gfx_FillRectangle_NoClip(xPos + 3, yPos + 3, sprite_width * scale + 4, sprite_height * scale + 4);

	gfx_ScaledSprite_NoClip(sprite_data, xPos + 5, yPos + 5, scale, scale);
	gfx_BlitRectangle(1, xPos, yPos, sprite_width * scale + 10, sprite_height * scale + 10);
	free(sprite_data);
	
	delay(200);
	while (!os_GetCSC());
	return;
}


void editact_Goto(editor_t *editor, cursor_t *cursor, uint8_t *ptr)
{
  
/*
dbg_sprintf(
  dbgout,
  "min_address = 0x%6x | window_address = 0x%6x | ptr = 0x%6x\n",
  editor->min_address,
  editor->window_address,
  ptr
);
*/
	
	cursor->primary = ptr;
	
	if (cursor->primary > editor->max_address)
	{
		cursor->primary = editor->max_address;
	}
	else if (cursor->primary <= editor->min_address)
	{
		cursor->primary = editor->min_address;
	};
	
	cursor->secondary = cursor->primary;
	
// The right-hand side of this expression is a potential overflow point if
// cursor->primary == 0xffffff.
//	while (
//    cursor->primary > (editor->window_address +
//    (((ROWS_ONSCREEN - 1) * COLS_ONSCREEN) / 2))
//  )
//  {
//		editor->window_address += COLS_ONSCREEN;
//	};
  
  if (cursor->primary > editor->window_address)
  {
    editor->window_address = editor->min_address
      + (((cursor->primary - editor->min_address) / COLS_ONSCREEN)
      * COLS_ONSCREEN);
  };

/*		
dbg_sprintf(
  dbgout,
  "min_address = 0x%6x | window_address = 0x%6x\n",
  editor->min_address,
  editor->window_address
);
*/

	if (cursor->primary < editor->window_address)
	{
		editor->window_address = editor->min_address
      + (((cursor->primary - editor->min_address) / COLS_ONSCREEN)
      * COLS_ONSCREEN);
	};
  
	return;
}


bool editact_DeleteBytes(
  editor_t *editor,
  cursor_t *cursor,
  uint8_t *deletion_point,
  uint24_t num_bytes
)
{
	// When the TI-OS resizes a file, any pointers to data within it
	// become inaccurate. It also does not add or remove bytes
	// from the end of the file, but from the start of the file.
	
	// The above means that the editor->min_address and editor->max_address will
  // be inaccurate.
	
	// If cursor->primary is at the end of the file and the number of bytes
  // requested to be deleted includes the byte the cursor is on, move the
  // cursor->primary to the last byte of the file.
	
	uint24_t num_bytes_shift = deletion_point - editor->min_address;

/*	
dbg_sprintf(
  dbgout, "num_bytes_shift = %d | num_bytes = %d\n", num_bytes_shift, num_bytes
);
dbg_sprintf(dbgout, "deletion_point = 0x%6x\n", deletion_point);
*/
	
	ti_var_t edit_file;
	
	if (num_bytes == 0)
		return false;

	ti_CloseAll();
	
	if ((edit_file = ti_Open(EDIT_FILE, "r")) == 0)
	{
		gui_DrawMessageDialog_Blocking(EDIT_FILE_OPEN_FAIL);
		return false;
	};
	
	if (num_bytes_shift > 0)
  {
		asm_CopyData(
      deletion_point - 1, deletion_point + num_bytes - 1, num_bytes_shift, 0
    );
  };

/*	
dbg_sprintf(
  dbgout,
  "Before re-assignment\neditor->min_address = 0x%6x\n",
  editor->min_address
);

dbg_sprintf(
  dbgout,
  "primary = 0x%6x | secondary = 0x%6x\n",
  cursor->primary,
  cursor->secondary
);
*/
  
	if (ti_Resize(ti_GetSize(edit_file) - num_bytes, edit_file) != -1)
	{
		editor->min_address = ti_GetDataPtr(edit_file);
		
		//The minus one is very important. If the file size is 0,
    // max_address == min_address - 1.
		if (ti_GetSize(edit_file) == 0)
		{
			editor->max_address = editor->min_address + ti_GetSize(edit_file);
		} else {
			editor->max_address = editor->min_address + ti_GetSize(edit_file) - 1;
		};
		
		cursor->secondary = editor->min_address + num_bytes_shift;
		cursor->primary = cursor->secondary;
		
		if (cursor->primary < editor->min_address)
		{
			cursor->primary = editor->min_address;
			cursor->secondary = editor->min_address;
		};
		
		if (cursor->primary > editor->max_address)
		{
			cursor->primary = editor->max_address;
			cursor->secondary = editor->max_address;
		};
		
		if (cursor->primary < editor->window_address)
		{
//dbg_sprintf(dbgout, "Re-assigned window_address\n");
			editor->window_address = editor->min_address
        + (((cursor->primary - editor->min_address) / COLS_ONSCREEN)
        * COLS_ONSCREEN);
		};
	}
	else
	{
		ti_Close(edit_file);
		gui_DrawMessageDialog_Blocking(EDIT_FILE_RESIZE_FAIL);
		return false;
	};
	
	if (!ti_GetSize(edit_file))
		editor->is_file_empty = true;

/*	
dbg_sprintf(
  dbgout,
  "After re-assignment\neditor->min_address = 0x%6x\n",
  editor->min_address
);
*/
	
	ti_Close(edit_file);
	return true;
}


bool editact_InsertBytes(
  editor_t *editor, uint8_t *insertion_point, uint24_t num_bytes
)
{
	ti_var_t edit_file;
	uint24_t i;
	uint24_t num_bytes_shift = insertion_point - editor->min_address;
	
	if (num_bytes == 0)
		return false;

	ti_CloseAll();
	
	if ((edit_file = ti_Open(EDIT_FILE, "r")) == 0)
	{
		gui_DrawMessageDialog_Blocking(EDIT_FILE_OPEN_FAIL);
		goto ERROR;
	};
	
	if (!ti_Resize(ti_GetSize(edit_file) + num_bytes, edit_file))
	{
		gui_DrawMessageDialog_Blocking(EDIT_FILE_RESIZE_FAIL);
		goto ERROR;
	};

/*	
dbg_sprintf(
  dbgout,
  "file_data_ptr = 0x%6x | editor->min_address = 0x%6x\n",
  ti_GetDataPtr(edit_file),
  editor->min_address
);
*/

	if (ti_Rewind(edit_file) == EOF)
	{
		gui_DrawMessageDialog_Blocking("Could not rewind edit file");
		goto ERROR;
	};

/*	
dbg_sprintf(dbgout, "num_bytes_shift = %d\n", num_bytes_shift);

dbg_sprintf(
  dbgout,
  "editor->min_address = 0x%6x | editor->max_address = 0x%6x\n",
  editor->min_address,
  editor->max_address
);

dbg_sprintf(
  dbgout,
  "primary = 0x%6x | secondary = 0x%6x\n",
  cursor->primary,
  cursor->secondary
);
*/

	if (num_bytes_shift > 0)
	{
		asm_CopyData(
      editor->min_address + num_bytes, editor->min_address, num_bytes_shift, 1
    );
	};
	
	for (i = 0; i < num_bytes; i++)
	{
		*(insertion_point + i) = '\0';
	};
	
	if (editor->type == FILE_EDITOR && editor->is_file_empty)
	{
		editor->is_file_empty = false;
		editor->max_address += num_bytes - 1;
	} else {
		editor->max_address += num_bytes;
	}
	
	ti_Close(edit_file);
	return true;
	
	ERROR:
	ti_Close(edit_file);
	return false;
}


bool editact_CreateUndoInsertBytesAction(
  editor_t *editor, cursor_t *cursor, uint24_t num_bytes
)
{
	ti_var_t undo_appvar;
	uint8_t undo_code = UNDO_INSERT_BYTES;
	
	if (num_bytes == 0)
		return false;

	ti_CloseAll();
	
	if ((undo_appvar = ti_Open(UNDO_APPVAR, "a")) == 0)
	{
		gui_DrawMessageDialog_Blocking(UNDO_APPVAR_OPEN_FAIL);
		goto ERROR;
	};
	
	if (ti_Resize(ti_GetSize(undo_appvar) + 10, undo_appvar) == -1)
	{
		gui_DrawMessageDialog_Blocking(UNDO_APPVAR_RESIZE_FAIL);
		goto ERROR;
	};
	
	ti_Write(&undo_code, 1, 1, undo_appvar);
	ti_Write(&editor->window_address, 3, 1, undo_appvar);
	ti_Write(&cursor->primary, 3, 1, undo_appvar);
	ti_Write(&num_bytes, 3, 1, undo_appvar);
	
	ti_Close(undo_appvar);
	return true;
	
	ERROR:
	ti_Close(undo_appvar);
	return false;
}


/**
* Deletes the last bytes inserted in the file being edited.
*
* Arguments:
*   Besides the regular arguments, the UNDO_APPVAR's offset is set to the byte after the
*   end of the undo action code.
* Returns:
*   undo_action_size = On success, the size (in bytes) of the undo action.
*                      On failure, 0.
*   Leaves the UNDO_APPVAR slot open.
*/
static uint24_t undo_insert_bytes(
  editor_t *editor, cursor_t *cursor, ti_var_t undo_appvar
)
{
	uint24_t num_bytes;
	uint24_t undo_action_size;
	bool deleted_bytes = false;
	
	ti_Read(&editor->window_address, 3, 1, undo_appvar);
	ti_Read(&cursor->primary, 3, 1, undo_appvar);
	ti_Read(&num_bytes, 3, 1, undo_appvar);
	undo_action_size = ti_Tell(undo_appvar);
	ti_Close(undo_appvar);

	// This function closes all open slots
	deleted_bytes = editact_DeleteBytes(
    editor, cursor, cursor->primary, num_bytes
  );

	if (!deleted_bytes)
		return 0;

	if ((undo_appvar = ti_Open(UNDO_APPVAR, "r")) == 0)
			return 0;

	cursor->secondary = cursor->primary;
	return undo_action_size;
}


bool editact_CreateDeleteBytesUndoAction(
  editor_t *editor, cursor_t *cursor, uint24_t num_bytes
)
{
	ti_var_t undo_appvar;
	uint8_t undo_code = UNDO_DELETE_BYTES;
	uint24_t i;
	
	if (num_bytes == 0)
		return false;

	ti_CloseAll();
	
	if ((undo_appvar = ti_Open(UNDO_APPVAR, "a")) == 0)
	{
		gui_DrawMessageDialog_Blocking(UNDO_APPVAR_OPEN_FAIL);
		goto ERROR;
	};
	
  // TODO: Is the "10" a magic number?
	if (ti_Resize(ti_GetSize(undo_appvar) + 10 + num_bytes, undo_appvar) == -1)
	{
		gui_DrawMessageDialog_Blocking(UNDO_APPVAR_RESIZE_FAIL);
		goto ERROR;
	};
	
	ti_Write(&undo_code, 1, 1, undo_appvar);
	ti_Write(&editor->window_address, 3, 1, undo_appvar);
	ti_Write(&cursor->secondary, 3, 1, undo_appvar);
	ti_Write(&num_bytes, 3, 1, undo_appvar);
	
	for (i = 0; i < num_bytes; i++)
	{
// dbg_sprintf(dbgout, "0x%2x ", *(cursor->secondary + i));
		ti_Write(cursor->secondary + i, 1, 1, undo_appvar);
	};
	
	ti_Close(undo_appvar);
	return true;
	
	ERROR:
	ti_Close(undo_appvar);
	return false;
}


/**
* Restores the last bytes deleted from the file being edited.
*
* Arguments:
*   Besides the regular arguments, the UNDO_APPVAR's offset is set to the byte after the
*   end of the undo action code.
* Returns:
*   undo_action_size = On success, the size (in bytes) of the undo action.
*                      On failure, 0.
*   Leaves the UNDO_APPVAR slot open.
*/
static uint24_t undo_delete_bytes(
  editor_t *editor, cursor_t *cursor, ti_var_t undo_appvar
)
{
	uint24_t num_bytes;
	uint24_t byte_data_offset;
	bool inserted_bytes = false;
	
	ti_Read(&editor->window_address, 3, 1, undo_appvar);
	ti_Read(&cursor->secondary, 3, 1, undo_appvar);
	ti_Read(&num_bytes, 3, 1, undo_appvar);
	byte_data_offset = ti_Tell(undo_appvar);
	ti_Close(undo_appvar);

	// This function closes all open slots.
	inserted_bytes = editact_InsertBytes(editor, cursor->secondary, num_bytes);

	if ((undo_appvar = ti_Open(UNDO_APPVAR, "r")) == 0)
			return 0;

	ti_Seek(byte_data_offset, SEEK_SET, undo_appvar);

	if (!inserted_bytes)
		return 0;

	for (uint24_t i = 0; i < num_bytes; i++)
		ti_Read(cursor->secondary + i, 1, 1, undo_appvar);

	editor->is_file_empty = false;
	cursor->primary = cursor->secondary + num_bytes - 1;
	cursor->multibyte_selection = true;
	return ti_Tell(undo_appvar);
}


uint8_t editact_GetNibble(cursor_t *cursor, uint8_t *ptr)
{
	uint8_t nibble = *ptr;
	
	if (cursor->high_nibble)
	{
		nibble = asm_HighToLowNibble(nibble);
	} else {
		nibble = asm_LowToHighNibble(nibble);
		nibble = asm_HighToLowNibble(nibble);
	};
	
	return nibble;
}


void editact_WriteNibble(cursor_t *cursor, uint8_t nibble)
{
	uint8_t i;
	
	if (cursor->high_nibble)
		nibble *= 16;
	
	for (i = 0; i < 4; i++)
	{
		*cursor->primary &= ~(1 << (i + (4 * cursor->high_nibble)));
	};
  
	*cursor->primary |= nibble;
	return;
}


bool editact_CreateUndoWriteNibbleAction(
  editor_t *editor, cursor_t *cursor, uint8_t nibble
)
{
	const uint8_t UNDO_WN_ACTION_SIZE = 9;

	ti_var_t undo_appvar;
	uint8_t undo_code = UNDO_WRITE_NIBBLE;
	
	if ((undo_appvar = ti_Open(UNDO_APPVAR, "a")) == 0)
	{
		gui_DrawMessageDialog_Blocking(UNDO_APPVAR_OPEN_FAIL);
		goto ERROR;
	};
	
	if (
    ti_Resize(ti_GetSize(undo_appvar) + UNDO_WN_ACTION_SIZE, undo_appvar) == -1
  )
	{
		gui_DrawMessageDialog_Blocking(UNDO_APPVAR_RESIZE_FAIL);
		goto ERROR;
	};
	
	ti_Write(&undo_code, 1, 1, undo_appvar);
	ti_Write(&editor->window_address, 3, 1, undo_appvar);
	ti_Write(&cursor->primary, 3, 1, undo_appvar);
	ti_Write(&cursor->high_nibble, 1, 1, undo_appvar);
	ti_Write(&nibble, 1, 1, undo_appvar);
	
	ti_Close(undo_appvar);
	return true;
	
	ERROR:
	ti_Close(undo_appvar);
	return false;
}


/**
* Rewrites the last overwritten nibble.
*
* Arguments:
*   Besides the regular arguments, the UNDO_APPVAR's offset is set to the byte
*   after the end of the undo action code.
* Returns:
*   undo_action_size = The size (in bytes) of the undo action.
*   Leaves the UNDO_APPVAR slot open.
*/
static uint24_t undo_write_nibble(
  editor_t *editor, cursor_t *cursor, ti_var_t undo_appvar
)
{
	uint8_t nibble;
	
	ti_Read(&editor->window_address, 3, 1, undo_appvar);
	ti_Read(&cursor->primary, 3, 1, undo_appvar);
	ti_Read(&cursor->high_nibble, 1, 1, undo_appvar);
	ti_Read(&nibble, 1, 1, undo_appvar);
	
	editact_WriteNibble(cursor, nibble);
	cursor->secondary = cursor->primary;
	return ti_Tell(undo_appvar);
}


bool editact_UndoAction(editor_t *editor, cursor_t *cursor)
{
	ti_var_t undo_appvar;
	uint8_t undo_code;
	uint24_t undo_action_size;

	ti_CloseAll();
	
	if ((undo_appvar = ti_Open(UNDO_APPVAR, "r")) == 0)
	{
		gui_DrawMessageDialog_Blocking(UNDO_APPVAR_OPEN_FAIL);
		return false;
	};
	
	if (ti_GetSize(undo_appvar) == 0)
	{
		ti_Close(undo_appvar);
		return false;
	};
	
	ti_Read(&undo_code, 1, 1, undo_appvar);
	
	switch(undo_code)
	{
		case UNDO_INSERT_BYTES:
			undo_action_size = undo_insert_bytes(editor, cursor, undo_appvar);
			break;
		
		case UNDO_DELETE_BYTES:
			undo_action_size = undo_delete_bytes(editor, cursor, undo_appvar);
			break;
		
		case UNDO_WRITE_NIBBLE:
			undo_action_size = undo_write_nibble(editor, cursor, undo_appvar);
			break;
		
		default:
			undo_action_size = 0;
	};

	if (undo_action_size == 0)
	{
		gui_DrawMessageDialog_Blocking("Could not execute undo");
		ti_Close(undo_appvar);
		return false;
	};

	if (ti_Resize(ti_GetSize(undo_appvar) - undo_action_size, undo_appvar) <= 0)
		gui_DrawMessageDialog_Blocking("Could not execute undo");
	ti_Close(undo_appvar);
	return true;
}


// Finds all occurances of PHRASE starting from START in MIN to MAX.
// Returns number of occurances found (max = 255).
// Stores pointers to each occurance in OCCURANCES.
uint8_t editact_FindPhraseOccurances(
	uint8_t *search_start,
	uint24_t search_range,
	uint8_t *search_min,
	uint8_t *search_max,
	char phrase[],
	uint8_t phrase_len,
	uint8_t **occurances
)
{
  uint8_t num_phrase_occurances;
  
  // The default search range is the ROM search range (5000000 bytes). If
  // we are searching in RAM, this range plus the search_start will cause
  // an overflow. Thus, if the search range is the default, we can shorten
  // it LOCALLY to cover all RAM to avoid the overflow (Even RAM_MAX_ADDRESS
  // + length of RAM will not cause an overflow).
  
  // For custom search ranges:
  //
  // Case 1: Searching RAM with a search range smaller than the default but
  //         large enough to cause an overflow.
  // Solution: In this case the search range is larger than the length of
  //           RAM. Shorten the search range to cover all RAM.
  //
  // Case 2: Searching RAM or ROM with a search range larger than the
  //         default.
  // Solution: Shorten the search range to cover either ROM or RAM
  //           depending on the search_start address.
  // Case 3: Searching RAM or ROM with a search range that will not cause
  //         an overflow in either case.
  // Solution: Do not modify the search range.
  
  /*
  // Case 2 catch
  if (search_range > ROM_SEARCH_RANGE)
  {
    if (search_start <= ROM_MAX_ADDRESS)
      search_range = ROM_SEARCH_RANGE;
    else
      search_range = RAM_SEARCH_RANGE;
  }
  else if (search_range > RAM_SEARCH_RANGE && search_start >= RAM_MIN_ADDRESS)
  {
    // Case 1 and default ROM search range catch
    search_range = RAM_SEARCH_RANGE;
  }
  */

  // The default search range should be larger than any of the search ranges.
  // If the default search range is smaller than the x_SEARCH_RANGE, if the
  // user tries to search "x," the find function will stop searching before
  // it has searched all of the memory in "x".
  if (search_start <= ROM_MAX_ADDRESS && search_range > ROM_SEARCH_RANGE)
  {
    search_range = ROM_SEARCH_RANGE;
  }
  else if (
    search_start >= RAM_MIN_ADDRESS
    && search_start <= RAM_MAX_ADDRESS
    && search_range > RAM_SEARCH_RANGE
  )
  {
    search_range = RAM_SEARCH_RANGE;
  }
  else if (
    search_start >= PORTS_MIN_ADDRESS
    && search_range > PORTS_SEARCH_RANGE
  )
  {
    search_range = PORTS_SEARCH_RANGE;
  };

// dbg_sprintf(dbgout, "search range = %d\n", search_range);

  // Case 1: search range is stops before the end of the given memory
  // Note: These are different cases than the ones mentioned above.
  // search_range MUST be less than or equal to the size of the given memory in
  // order to avoid variable overflows/underflows.
  if (search_start < search_max - search_range)
  {
//dbg_sprintf(dbgout, "case 1 executing\n");
    num_phrase_occurances = asm_BFind_All(
      search_start,
      search_start + search_range,
      phrase,
      phrase_len,
      occurances,
      MAX_NUM_PHRASE_OCCURANCES
    );
  }
  else
  {
/*
dbg_sprintf(dbgout, "case 2 executing\n");
dbg_sprintf(dbgout, "start = 0x%6x | max = 0x%6x\n", search_start, search_max);
*/
 
    // Case 2: search range is longer than given memory
    num_phrase_occurances = asm_BFind_All(
      search_start,
      search_max,
      phrase,
      phrase_len,
      occurances,
      MAX_NUM_PHRASE_OCCURANCES
     );

    // Loop back to the start of the memory and continue searching
    // Order of operations is vitally important to avoid variable
    // overflows/underflows.
    search_range = search_range - (search_max - search_start);

    // Case 2a: search range stops before search start position
    if (search_min + search_range < search_start)
    {
// dbg_sprintf(dbgout, "case 2a executing\n");
      num_phrase_occurances += asm_BFind_All(
        search_min,
        search_min + search_range,
        phrase,
        phrase_len,
        occurances + num_phrase_occurances,
        MAX_NUM_PHRASE_OCCURANCES - num_phrase_occurances
      );
    }
    else
    {
// dbg_sprintf(dbgout, "case 2b executing\n");

      // Case 2b: search range exceeds search start position; stop searching
      // at search start position.
      num_phrase_occurances += asm_BFind_All(
        search_min,
        search_start,
        phrase,
        phrase_len,
        occurances + num_phrase_occurances,
        MAX_NUM_PHRASE_OCCURANCES - num_phrase_occurances
      );
    };
  };

/*
for (uint8_t i = 0; i < num_phrase_occurances; i++)
{
  dbg_sprintf(dbgout, "0x%6x\n", occurances[i]);
};
*/

    return num_phrase_occurances;
}