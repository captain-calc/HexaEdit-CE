#include "asmutil.h"
#include "colors.h"
#include "debug.h"
#include "editor.h"
#include "editor_actions.h"
#include "gui.h"

#include <graphx.h>
#include <tice.h>

#include <stdint.h>


/* Define common error messages as globals. */
const char *EDIT_FILE_OPEN_FAIL = "Could not open edit file";
const char *EDIT_FILE_RESIZE_FAIL = "Could not resize edit file";
const char *UNDO_APPVAR_OPEN_FAIL = "Could not open undo appvar";
const char *UNDO_APPVAR_RESIZE_FAIL = "Could not resize undo appvar";


void editact_SpriteViewer(editor_t *editor, cursor_t *cursor)
{
	gfx_sprite_t *sprite_data;
	uint8_t sprite_width, sprite_height;
	uint24_t sprite_size;
	uint24_t xPos = 0;
	uint8_t yPos = 0;
	uint8_t scale = 2;
		
	// Get the sprite's size
	sprite_width = *cursor->primary;
	sprite_height = *(cursor->secondary + 1);
	sprite_size = sprite_width * sprite_height;
	
	if (sprite_size == 0 || sprite_height > 245)
		return;
	
	if ((uint24_t)(editor->max_address - cursor->primary) < sprite_size)
		return;
	
	sprite_data = gfx_MallocSprite(sprite_width, sprite_height);
	if (sprite_data == NULL)
		return;
	
	// Set the scale to one if the sprite is very large
	if (sprite_width > 155 || sprite_height > 115)
		scale = 1;
	
	asm_CopyData(cursor->primary + 2, sprite_data->data, sprite_size, 1);
	
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

void editact_Goto(editor_t *editor, cursor_t *cursor, uint24_t offset)
{
	dbg_sprintf(dbgout, "min_address = 0x%6x | window_address = 0x%6x | offset = %d\n", editor->min_address, editor->window_address, offset);
	
	if (editor->type == FILE_EDITOR)
	{
		offset += (uint24_t)editor->min_address;
	};
	
	if (offset >= (uint24_t)editor->max_address)
	{
		cursor->primary = editor->max_address;
	}
	else if (offset <= (uint24_t)editor->min_address)
	{
		cursor->primary = editor->min_address;
	}
	else
	{
		cursor->primary = (uint8_t *)offset;
	};
	cursor->secondary = cursor->primary;
	
	while (cursor->primary > editor->window_address + (((ROWS_ONSCREEN - 1) * COLS_ONSCREEN) / 2))
	{
		editor->window_address += COLS_ONSCREEN;
	};
		
	//dbg_sprintf(dbgout, "min_address = 0x%6x | window_address = 0x%6x\n", editor->min_address, editor->window_address);

	if (cursor->primary < editor->window_address)
	{
		editor->window_address = editor->min_address + (((cursor->primary - editor->min_address) / COLS_ONSCREEN) * COLS_ONSCREEN);
	};
	return;
}

bool editact_DeleteBytes(editor_t *editor, cursor_t *cursor, uint8_t *deletion_point, uint24_t num_bytes)
{
	/* When the TI-OS resizes a file, any pointers to data within it
	   become inaccurate. It also does not add or remove bytes
	   from the end of the file, but from the start of the file */
	
	/* The above means that the editor->min_address and editor->max_address will be inaccurate */
	
	/* If cursor->primary is at the end of the file and the number of bytes requested to be deleted
	   includes the byte the cursor is on, move the cursor->primary to the last byte of the file */
	
	uint24_t num_bytes_shift = deletion_point - editor->min_address;
	
	dbg_sprintf(dbgout, "num_bytes_shift = %d | num_bytes = %d\n", num_bytes_shift, num_bytes);
	dbg_sprintf(dbgout, "deletion_point = 0x%6x\n", deletion_point);
	
	ti_var_t edit_file;
	
	if (num_bytes == 0)
	{
		return false;
	};
	
	if ((edit_file = ti_Open(EDIT_FILE, "r")) == 0)
	{
		gui_DrawMessageDialog_Blocking(EDIT_FILE_OPEN_FAIL);
		return false;
	};
	
	if (num_bytes_shift > 0)
	{
		asm_CopyData(deletion_point - 1, deletion_point + num_bytes - 1, num_bytes_shift, 0);
	};
	
	dbg_sprintf(dbgout, "Before re-assignment\neditor->min_address = 0x%6x\n", editor->min_address);
	dbg_sprintf(dbgout, "primary = 0x%6x | secondary = 0x%6x\n", cursor->primary, cursor->secondary);
	
	if (ti_Resize(ti_GetSize(edit_file) - num_bytes, edit_file) != -1)
	{
		editor->min_address = ti_GetDataPtr(edit_file);
		
		/* The minus one is very important. If the file size is 0, max_address == min_address - 1.*/
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
			editor->window_address = editor->min_address + ((cursor->primary - editor->min_address) / COLS_ONSCREEN) * COLS_ONSCREEN;
		};
	}
	else
	{
		ti_Close(edit_file);
		gui_DrawMessageDialog_Blocking(EDIT_FILE_RESIZE_FAIL);
		return false;
	};
	
	if (ti_GetSize(edit_file) == 0)
	{
		editor->is_file_empty = true;
	};
	
	dbg_sprintf(dbgout, "After re-assignment\neditor->min_address = 0x%6x\n", editor->min_address);
	
	ti_Close(edit_file);
	return true;
}

bool editact_InsertBytes(editor_t *editor, uint8_t *insertion_point, uint24_t num_bytes)
{
	ti_var_t edit_file;
	uint24_t i;
	uint24_t num_bytes_shift = insertion_point - editor->min_address;
	
	if (num_bytes == 0)
	{
		return false;
	};
	
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
	
	dbg_sprintf(dbgout, "file_data_ptr = 0x%6x | editor->min_address = 0x%6x\n", ti_GetDataPtr(edit_file), editor->min_address);
	
	if (ti_Rewind(edit_file) == EOF)
	{
		gui_DrawMessageDialog_Blocking("Could not rewind edit file");
		goto ERROR;
	};
	
	//dbg_sprintf(dbgout, "num_bytes_shift = %d\n", num_bytes_shift);
	//dbg_sprintf(dbgout, "editor->min_address = 0x%6x | editor->max_address = 0x%6x\n", editor->min_address, editor->max_address);
	//dbg_sprintf(dbgout, "primary = 0x%6x | secondary = 0x%6x\n", cursor->primary, cursor->secondary);
	
	if (num_bytes_shift > 0)
	{
		asm_CopyData(editor->min_address + num_bytes, editor->min_address, num_bytes_shift, 1);
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

bool editact_CreateUndoInsertBytesAction(editor_t *editor, cursor_t *cursor, uint24_t num_bytes)
{
	ti_var_t undo_appvar;
	uint8_t undo_code = UNDO_INSERT_BYTES;
	
	if (num_bytes == 0)
	{
		return false;
	};
	
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
	
	ti_CloseAll();
	return true;
	
	ERROR:
	ti_CloseAll();
	return false;
}

void undo_insert_bytes(editor_t *editor, cursor_t *cursor, ti_var_t undo_appvar)
{
	uint24_t num_bytes;
	
	ti_Read(&editor->window_address, 3, 1, undo_appvar);
	ti_Read(&cursor->primary, 3, 1, undo_appvar);
	ti_Read(&num_bytes, 3, 1, undo_appvar);
	
	//dbg_sprintf(dbgout, "num_bytes = %d\n", num_bytes);
	
	editact_DeleteBytes(editor, cursor, cursor->primary, num_bytes);
	
	if (ti_Resize(ti_GetSize(undo_appvar) - ti_Tell(undo_appvar), undo_appvar) == -1)
	{
		gui_DrawMessageDialog_Blocking("Could not delete undo action");
		return;
	};
	
	cursor->secondary = cursor->primary;
	return;
}

bool editact_CreateDeleteBytesUndoAction(editor_t *editor, cursor_t *cursor, uint24_t num_bytes)
{
	ti_var_t undo_appvar;
	uint8_t undo_code = UNDO_DELETE_BYTES;
	uint24_t i;
	
	if (num_bytes == 0)
	{
		return false;
	};
	
	if ((undo_appvar = ti_Open(UNDO_APPVAR, "a")) == 0)
	{
		gui_DrawMessageDialog_Blocking(UNDO_APPVAR_OPEN_FAIL);
		goto ERROR;
	};
	
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
		dbg_sprintf(dbgout, "0x%2x ", *(cursor->secondary + i));
		ti_Write(cursor->secondary + i, 1, 1, undo_appvar);
	};
	
	ti_CloseAll();
	return true;
	
	ERROR:
	ti_CloseAll();
	return false;
}

void undo_delete_bytes(editor_t *editor, cursor_t *cursor, ti_var_t undo_appvar)
{
	uint24_t num_bytes, i;
	
	ti_Read(&editor->window_address, 3, 1, undo_appvar);
	ti_Read(&cursor->secondary, 3, 1, undo_appvar);
	ti_Read(&num_bytes, 3, 1, undo_appvar);
	
	dbg_sprintf(dbgout, "num_bytes = %d\n", num_bytes);
	
	editact_InsertBytes(editor, cursor->secondary, num_bytes);
	
	dbg_sprintf(dbgout, "Inserted bytes for undo-ing.\n");
	
	for (i = 0; i < num_bytes; i++)
	{
		ti_Read(cursor->secondary + i, 1, 1, undo_appvar);
	};
	
	if (ti_Resize(ti_GetSize(undo_appvar) - ti_Tell(undo_appvar), undo_appvar) == -1)
	{
		gui_DrawMessageDialog_Blocking("Could not delete undo action");
		return;
	};
	
	editor->is_file_empty = false;
	cursor->primary = cursor->secondary + num_bytes - 1;
	cursor->multibyte_selection = true;
	return;
}

uint8_t editact_GetNibble(cursor_t *cursor, uint8_t *ptr)
{
	uint8_t nibble = *ptr;
	
	if (cursor->high_nibble)
	{
		nibble = asm_HighToLowNibble(nibble);
	}
	else
	{
		nibble = asm_LowToHighNibble(nibble);
		nibble = asm_HighToLowNibble(nibble);
	};
	
	return nibble;
}

void editact_WriteNibble(cursor_t *cursor, uint8_t nibble)
{
	uint8_t i;
	
	if (cursor->high_nibble)
	{
		nibble *= 16;
	};
	for (i = 0; i < 4; i++)
	{
		*cursor->primary &= ~(1 << (i + (4 * cursor->high_nibble)));
	};
	*cursor->primary |= nibble;
	return;
}

bool editact_CreateUndoWriteNibbleAction(editor_t *editor, cursor_t *cursor, uint8_t nibble)
{
	ti_var_t undo_appvar;
	uint8_t undo_code = UNDO_WRITE_NIBBLE;
	
	if ((undo_appvar = ti_Open(UNDO_APPVAR, "a")) == 0)
	{
		gui_DrawMessageDialog_Blocking(UNDO_APPVAR_OPEN_FAIL);
		goto ERROR;
	};
	
	if (ti_Resize(ti_GetSize(undo_appvar) + 9, undo_appvar) == -1)
	{
		gui_DrawMessageDialog_Blocking(UNDO_APPVAR_RESIZE_FAIL);
		goto ERROR;
	};
	
	ti_Write(&undo_code, 1, 1, undo_appvar);
	ti_Write(&editor->window_address, 3, 1, undo_appvar);
	ti_Write(&cursor->primary, 3, 1, undo_appvar);
	ti_Write(&cursor->high_nibble, 1, 1, undo_appvar);
	ti_Write(&nibble, 1, 1, undo_appvar);
	
	ti_CloseAll();
	return true;
	
	ERROR:
	ti_CloseAll();
	return false;
}

void undo_write_nibble(editor_t *editor, cursor_t *cursor, ti_var_t undo_appvar)
{
	uint8_t nibble;
	
	ti_Read(&editor->window_address, 3, 1, undo_appvar);
	ti_Read(&cursor->primary, 3, 1, undo_appvar);
	ti_Read(&cursor->high_nibble, 1, 1, undo_appvar);
	ti_Read(&nibble, 1, 1, undo_appvar);
	
	if (ti_Resize(ti_GetSize(undo_appvar) - ti_Tell(undo_appvar), undo_appvar) == -1)
	{
		gui_DrawMessageDialog_Blocking("Could not delete undo action");
		return;
	};
	
	editact_WriteNibble(cursor, nibble);
	cursor->secondary = cursor->primary;
	return;
}

bool editact_UndoAction(editor_t *editor, cursor_t *cursor)
{
	ti_var_t undo_appvar;
	uint8_t undo_code;
	
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
		undo_insert_bytes(editor, cursor, undo_appvar);
		ti_Close(undo_appvar);
		return true;
		
		case UNDO_DELETE_BYTES:
		undo_delete_bytes(editor, cursor, undo_appvar);
		ti_Close(undo_appvar);
		return true;
		
		case UNDO_WRITE_NIBBLE:
		undo_write_nibble(editor, cursor, undo_appvar);
		ti_Close(undo_appvar);
		return true;
		
		default:
		ti_Close(undo_appvar);
		return false;
	};
}