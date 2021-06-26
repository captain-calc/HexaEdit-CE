#include "debug.h"

#include "asmutil.h"
#include "colors.h"
#include "editor.h"
#include "editor_actions.h"
#include "editor_gui.h"
#include "gui.h"
#include "settings.h"

#include <fileioc.h>
#include <graphx.h>
#include <keypadc.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


/*-----------------------------
IMPORTANT:

If any files are created after the editor and cursor pointers have been set inside the edit file,
the contents of RAM will be shifted by an indeterminate amount, rendering all of the editor and
cursor pointers invalid.

Any files that the editor needs must be created either before the editor and cursor pointers are
set or after the pointers are no longer needed.
*/


/**
 * The Headless Start feature uses the following data configuration in the
 * Headless Start appvar (Note: Always include the general configuration data!):
 *
 *
 * General Configuration
 * ===============================
 * Color Theme/Editor Type	1 byte
 *
 *
 * Color Theme Override (7 bytes)
 * ===============================
 * Background Color		1
 * Bar Color			1
 * Bar Text Color		1
 * Table BG Color		1
 * Table Text Color		1
 * Selected Table Text Color	1
 * Cursor Color			1
 * ===============================
 *
 *
 * RAM Editor/ROM Viewer
 * ===============================
 * Cursor Primary Address	3
 * Cursor Secondary Address	3
 * ===============================
 *
 *
 * File Editor
 * ===============================
 * File Name			10
 * File Type			1
 * Cursor Primary Offset	3
 * Cursor Secondary Offset	3
 * ===============================
 *
 *
 * Only include the data sections you will need. For example, if you wanted to
 * start a file editor and override the default color scheme, you would include
 * the following sections:
 *
 * ===========================
 * General Configuration Data
 * Color Theme Override
 * File Editor
 * ===========================
 *
 *
 * Configuration Data Notes
 * =================================
 *
 * The Color Theme/Editor Type byte looks like this:
 *
 * 0000 0000
 * ^      ^
 * |      |
 * |      * The two least significant bytes specify the editor type (File = 0, RAM = 1, ROM = 2)
 * |
 * * The most significant byte should be set to specify a color override. It should be set to 0 if
 *   you do not want to change the color scheme.
 *
 * If you want to override the color scheme and open a file editor, for example, the byte would look
 * like: 1000 0010.
 *
 *
 * You may notice that the values for the window and cursor pointers for the file editor are OFFSETS
 * instead of memory pointers. This is because HexaEdit does not edit the specified file directly but,
 * rather, a copy of it. HexaEdit does not create this copy until after it reads out the configuration
 * data and creates the necessary memory pointers out of the file offsets.
*/


static uint24_t decimal(const char *hex)
{
	const char *hex_chars = {"0123456789abcdef"};
	uint8_t i, j;
	uint24_t place = 1;
	uint24_t decimal = 0;
	
	
	i = strlen(hex);
	
	while (i > 0)
	{
		i--;
		
		for (j = 0; j < 16; j++)
		{
			if (*(hex + i) == hex_chars[j])
				decimal += place * j;
		};
		
		place *= 16;
	};
	
	return decimal;
}


static void move_cursor(editor_t *editor, cursor_t *cursor, uint8_t direction, bool accelerated_cursor)
{
	uint8_t i;
	uint8_t *old_cursor_address;
	
	
	if (direction == CURSOR_LEFT && cursor->primary > editor->min_address)
		cursor->primary--;
	
	if (direction == CURSOR_RIGHT && cursor->primary < editor->max_address)
		cursor->primary++;
	
	if (direction == CURSOR_DOWN)
	{
		old_cursor_address = cursor->primary;
		while (cursor->primary < (old_cursor_address + COLS_ONSCREEN) && cursor->primary < editor->max_address)
			cursor->primary++;
		
		if (accelerated_cursor)
		{
			i = (ROWS_ONSCREEN - 1) * COLS_ONSCREEN;
			
			while (cursor->primary < editor->max_address && i-- > 0)
				cursor->primary++;
		};
	};
	
	if (direction == CURSOR_UP)
	{
		old_cursor_address = cursor->primary;
		while (cursor->primary > old_cursor_address - COLS_ONSCREEN && cursor->primary > editor->min_address)
			cursor->primary--;
		
		if (accelerated_cursor)
		{
			i = (ROWS_ONSCREEN - 1) * COLS_ONSCREEN;
			
			while (cursor->primary > editor->min_address && i-- > 0)
				cursor->primary--;
		};
	};
	
	/* Move the window offset if necessary */
	while (cursor->primary < editor->window_address && editor->window_address > editor->min_address)
	{
		editor->window_address -= COLS_ONSCREEN;
	};
	
	while ((cursor->primary - editor->window_address) >= ((ROWS_ONSCREEN + 1) * COLS_ONSCREEN) && editor->window_address < (editor->max_address + COLS_ONSCREEN))
	{
		editor->window_address += COLS_ONSCREEN;
	};
	
	/* Any time the cursor is moved, reset the nibble selector to the high nibble. */
	cursor->high_nibble = true;
	
	if (!cursor->multibyte_selection)
	{
		cursor->secondary = cursor->primary;
	} else {
		if (cursor->primary < cursor->secondary)
		{
			cursor->multibyte_selection = false;
			cursor->secondary = cursor->primary;
		};
	};
	
	return;
}


static void goto_prompt(editor_t *editor, cursor_t *cursor, uint8_t editor_index_method)
{
	char buffer[8] = {'\0'};
	uint8_t buffer_size;
	int8_t key;
	
	uint24_t offset;
	char *keymap;
	char keymap_indicator;
	
	
	if (editor_index_method == OFFSET_INDEXING)
	{
		keymap = NUMBERS;
		keymap_indicator = '0';
		buffer_size = 7;
	} else {
		keymap = ASCII_HEX_KEYMAP;
		keymap_indicator = 'x';
		buffer_size = 6;
	};
	
	for (;;)
	{
		gui_DrawInputPrompt("Goto:", 102);
		gui_DrawKeymapIndicator(keymap_indicator, 147, 223);
		gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
		
		gfx_SetTextBGColor(color_theme.table_bg_color);
		gfx_SetTextFGColor(color_theme.table_text_color);
		gfx_SetTextTransparentColor(color_theme.table_bg_color);
		
		key = gui_Input(buffer, buffer_size, 44, 224, 99, keymap);
		
		if (key == sk_Clear)
			return;
		
		if (key == sk_2nd || key == sk_Enter)
			break;
		
		delay(200);
	};
	
	if (editor_index_method == OFFSET_INDEXING)
	{
		offset = atoi(buffer);
		if ((uint24_t)(editor->max_address - editor->min_address) > offset)
			editact_Goto(editor, cursor, editor->min_address + offset);
		else
			editact_Goto(editor, cursor, editor->max_address);
	} else {
		editact_Goto(editor, cursor, (uint8_t *)decimal(buffer));
	};
	return;
}

static bool insert_bytes_prompt(editor_t *editor, cursor_t *cursor)
{
	char buffer[6] = {'\0'};
	uint8_t buffer_size = 5;
	int8_t key;
	
	uint24_t num_bytes_insert;
	
	for (;;)
	{
		gui_DrawInputPrompt("Insert:", 102);
		gui_DrawKeymapIndicator('0', 163, 223);
		gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
		
		gfx_SetTextBGColor(color_theme.table_bg_color);
		gfx_SetTextFGColor(color_theme.table_text_color);
		gfx_SetTextTransparentColor(color_theme.table_bg_color);
		
		key = gui_Input(buffer, buffer_size, 60, 224, 99, NUMBERS);
		
		if (key == sk_Clear)
			return false;
		
		if (key == sk_2nd || key == sk_Enter)
			break;
		
		delay(200);
	};
	
	num_bytes_insert = (uint24_t)atoi(buffer);
	
// dbg_sprintf(dbgout, "num_bytes_insert = %d\n", num_bytes_insert);
	
	if (editact_CreateUndoInsertBytesAction(editor, cursor, num_bytes_insert))
	{
		if (editact_InsertBytes(editor, cursor->primary, num_bytes_insert))
		{
			editor->num_changes++;
			return true;
		};
	} else {
		gui_DrawMessageDialog_Blocking("Could not insert bytes");
	};
	return false;
}

// Converts an ASCII hexadecimal character (0 - 9, a - f) into a nibble.
static void ascii_to_nibble(const char *in, char *out, uint8_t in_len)
{
	const char *hex_chars = "0123456789abcdef";
	uint8_t byte;
	uint8_t read_offset = 0;
	uint8_t write_offset = 0;
	
	while (read_offset < in_len - 1)
	{
		// Read out two ascii characters and convert them into one byte
		byte = (uint8_t)(strchr(hex_chars, (int)(*(in + read_offset++))) - hex_chars);
		byte *= 16;
		byte += (uint8_t)(strchr(hex_chars, (int)(*(in + read_offset++))) - hex_chars);
		
// dbg_sprintf(dbgout, "byte = %x\n", byte);
		
		*(out + write_offset++) = (unsigned char)byte;
	};
	
	return;
}


static void phrase_search_prompt(editor_t *editor, cursor_t *cursor, uint8_t editor_index_method)
{
	char buffer[17] = {'\0'};
	char phrase[17] = {'\0'};
	char draw_buffer[8] = {'\0'};
	uint8_t phrase_len;
	
	const char *prompt = "Find:";
	uint8_t input_box_x = gfx_GetStringWidth(prompt) + 10;
	uint8_t input_box_width = 200;
	int8_t key;
	
	const char *keymaps[] = {
		ASCII_HEX_KEYMAP,
		UPPERCASE_LETTERS,
		LOWERCASE_LETTERS,
		NUMBERS
	};
	const char keymap_indicators[] = {'x', 'A', 'a', '0'};
	uint8_t keymap_num = 0;
	
  uint24_t OCCURANCES_ALLOC_SIZE = MAX_NUM_PHRASE_OCCURANCES * sizeof(uint8_t *);
	uint8_t **occurances = (uint8_t **)malloc(OCCURANCES_ALLOC_SIZE);
	uint8_t num_occurances = 0;
	uint8_t occurance_offset = 0;
	uint24_t search_range;
	
	search_range = settings_GetPhraseSearchRange();
	
// dbg_sprintf(dbgout, "search_range = %d\n", search_range);
	
	for (;;)
	{
		gui_DrawInputPrompt(prompt, input_box_width);
		gui_DrawKeymapIndicator(
			keymap_indicators[keymap_num],
			input_box_x + input_box_width + 1,
			223
		);
		
		// Draw occurance_offset and number of occurances
		memset(draw_buffer, '\0', sizeof(draw_buffer));
		
		if (num_occurances == 0)
		{
			sprintf(draw_buffer, "%d/%d", occurance_offset, num_occurances);
		} else {
			sprintf(draw_buffer, "%d/%d", occurance_offset + 1, num_occurances);
		};
		
		gfx_PrintStringXY(
			draw_buffer,
			LCD_WIDTH - 5 - gfx_GetStringWidth(draw_buffer),
			226
		);
		gfx_BlitBuffer();
		
		gfx_SetTextBGColor(color_theme.table_bg_color);
		gfx_SetTextFGColor(color_theme.table_text_color);
		gfx_SetTextTransparentColor(color_theme.table_bg_color);
		
		key = gui_Input(buffer, 16, input_box_x, 224, input_box_width, keymaps[keymap_num]);
		
		delay(50);
		
// dbg_sprintf(dbgout, "num_occurances = %d | occurance_offset = %d\n", num_occurances, occurance_offset);
		
		if (key == sk_Down)
		{
			if (occurance_offset + 1 < num_occurances)
				occurance_offset++;
			else
				occurance_offset = 0;
		}
		else if (key == sk_Up)
		{
			if (occurance_offset > 0)
			{
				occurance_offset--;
			}
			else if (num_occurances > 0)
			{
				occurance_offset = num_occurances - 1;
			};
		}
		else if (key == sk_Alpha && *buffer == '\0')
		{
			// Only allow keymap switching when there is no data in the buffer
			if (keymap_num < 3)
				keymap_num++;
			else
				keymap_num = 0;
		}
		else if (key == sk_Clear)
		{
			break;
		};
		
		if (key == sk_2nd || key == sk_Enter)
		{
			// If the keymap is hexadecimal, each character is equivalent to
			// one nibble. ascii_to_nibble() converts each character in buffer
			// into a nibble in phrase, e.g. "30ab" -> "\x30\xab". This process is
			// not necessary for letter or number input since those are already in
			// the proper byte format.
			
			if (keymap_num == 0)
			{
				ascii_to_nibble(buffer, phrase, strlen(buffer));
				phrase_len = strlen(buffer) / 2;
			} else {
				strncpy(phrase, buffer, strlen(buffer));
				phrase_len = strlen(buffer);
			};
			
      cursor->primary = cursor->secondary;
      num_occurances = 0;
      occurance_offset = 0;
      memset(occurances, '\0', OCCURANCES_ALLOC_SIZE);
      
			if (phrase_len < 2)
				goto SKIP_FIND_PHRASE;
			
			// If hexadecimal input is being used, do not call the find
			// phrase routine, unless the buffer input is divisible by two
			// (the phrase contains no half-bytes or nibbles at the end).
			
			if (keymap_num == 0 && (strlen(buffer) % 2 != 0))
				goto SKIP_FIND_PHRASE;
      
			num_occurances = editact_FindPhraseOccurances(
				cursor->primary,
				search_range,
				editor->min_address,
				editor->max_address,
				phrase,
				phrase_len,
				occurances
			);
		};
		
		SKIP_FIND_PHRASE:
		
		if (num_occurances > 0)
		{
			editact_Goto(editor, cursor, occurances[occurance_offset]);
			cursor->primary += phrase_len - 1;
			cursor->multibyte_selection = true;
		};
		
		editorgui_DrawEditorContents(editor, cursor, editor_index_method);
	};
	
  free(occurances);
	return;
}

static uint8_t save_prompt(void)
{
	int8_t key;
	
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	
	gfx_PrintStringXY("Save changes?", 5, 226);
	gfx_PrintStringXY("No", 152, 226);
	gfx_PrintStringXY("Yes", 226, 226);
	gfx_PrintStringXY("Cancel", 270, 226);
	gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	
	// Prevent long keypress from triggering fall-through.
	delay(500);
	
	do {
		kb_Scan();
		key = asm_GetCSC();
	} while (key < 49 && key < 51);
	
	if (key == sk_Zoom)
	{
		return 2;
	}
	else if (key == sk_Trace)
	{
		return 1;
	}
	else
	{
		return 0;
	};
}

static bool save_file(char *name, uint8_t type)
{
	ti_var_t original_file, new_file, edit_file;
	uint24_t edit_file_size;
	bool is_archived;

	ti_CloseAll();

	if ((original_file = ti_OpenVar(name, "r", type)) == 0)
	{
		gui_DrawMessageDialog_Blocking("Failed to open original file");
		return false;
	};

	is_archived = ti_IsArchived(original_file);

	ti_CloseAll();

	// Open the editor's edit file and another file that will become
	// the new changed file.
	if ((edit_file = ti_Open(EDIT_FILE, "r")) == 0)
	{
		gui_DrawMessageDialog_Blocking("Could not open edit file");
		return false;
	};
	
	edit_file_size = ti_GetSize(edit_file);

	if ((new_file = ti_OpenVar("HEXATMP2", "w", type)) == 0)
	{
		ti_CloseAll();
		gui_DrawMessageDialog_Blocking("Failed to open new file");
		return false;
	};

	// Make the new file as big as the edit file
	if (edit_file_size > 0) {
		if (ti_Resize(edit_file_size, new_file) <= 0)
		{
			ti_CloseAll();
			ti_DeleteVar("HEXATMP2", type);
			gui_DrawMessageDialog_Blocking("Failed to resize new file");
			return false;
		};

		// Copy the contents of the edit file into the new file
		asm_CopyData(ti_GetDataPtr(edit_file), ti_GetDataPtr(new_file), edit_file_size, 1);
	};

	ti_CloseAll();

	// Delete the original file.
	if (!ti_DeleteVar(name, type))
	{
		gui_DrawMessageDialog_Blocking("Failed to delete original file");
		return false;
	};

	// If the original file was archived, archive the new file.
	if ((new_file = ti_OpenVar("HEXATMP2", "r", type)) == 0)
	{
		gui_DrawMessageDialog_Blocking("Failed to open new file");
		return false;
	};

	if (is_archived)
	{
		if (!ti_SetArchiveStatus(1, new_file))
			goto ERROR_MEM;
	};

	ti_CloseAll();

	// Finally, rename the new file as the original file.
	if (ti_RenameVar("HEXATMP2", name, type) != 0)
	{
		gui_DrawMessageDialog_Blocking("Failed to rename new file");
		return false;
	};

	return true;

	ERROR_MEM:
	ti_CloseAll();
	gui_DrawMessageDialog_Blocking("Insufficient ROM to save file");
	return false;
}

static void run_editor(editor_t *editor, cursor_t *cursor)
{
	int8_t key;
	uint8_t editor_index_method = ADDRESS_INDEXING;
	bool redraw_top_bar = true;
	bool redraw_tool_bar = true;
	uint8_t save_code;
	
//dbg_sprintf(dbgout, "window_address = 0x%6x | min_address = 0x%6x | max_address = 0x%6x\n", editor->window_address, editor->min_address, editor->max_address);
	
	if (editor->type == FILE_EDITOR)
		editor_index_method = OFFSET_INDEXING;
	
	for (;;)
	{
		editorgui_DrawEditorContents(editor, cursor, editor_index_method);
		
		if (redraw_top_bar)
		{
			editorgui_DrawTopBar(editor);
			redraw_top_bar = false;
		};
		
		if (redraw_tool_bar)
		{
			editorgui_DrawToolBar(editor);
			redraw_tool_bar = false;
		}
		
		if (cursor->multibyte_selection)
		{
			editorgui_DrawAltToolBar(cursor);
		};
		
		gfx_BlitBuffer();
		
		do {
			kb_Scan();
		} while ((key = asm_GetCSC()) == -1);
		
		// Since pressing '0' writes a NULL nibble, it is a special case.
		if ((HEX_VAL_KEYMAP[key] != '\0' || key == sk_0) && !cursor->multibyte_selection && (editor->type == FILE_EDITOR || editor->type == RAM_EDITOR))
		{
			if (editact_GetNibble(cursor, cursor->primary) != HEX_VAL_KEYMAP[key])
			{
				if (editact_CreateUndoWriteNibbleAction(editor, cursor, editact_GetNibble(cursor, cursor->primary)))
				{
					editact_WriteNibble(cursor, HEX_VAL_KEYMAP[key]);
					redraw_top_bar = true;
					redraw_tool_bar = true;
					editor->num_changes++;
				};
			};
			if (cursor->high_nibble)
				cursor->high_nibble = false;
			else
				move_cursor(editor, cursor, CURSOR_RIGHT, false);
		};
		
		if (key == sk_2nd || key == sk_Enter)
		{
			if (cursor->multibyte_selection)
			{
				cursor->multibyte_selection = false;
				cursor->secondary = cursor->primary;
				redraw_tool_bar = true;
			}
			else
			{
				cursor->multibyte_selection = true;
			};
			
			delay(200);
		};
		
		if (key == sk_Del && editor->type == FILE_EDITOR)
		{
			if (editact_CreateDeleteBytesUndoAction(editor, cursor, cursor->primary - cursor->secondary + 1))
			{
				editact_DeleteBytes(editor, cursor, cursor->secondary, cursor->primary - cursor->secondary + 1);
			};
			cursor->multibyte_selection = false;
			redraw_top_bar = true;
			redraw_tool_bar = true;
			editor->num_changes++;
		};
		
		if (key == sk_Stat && !cursor->multibyte_selection)
		{
			editact_SpriteViewer(editor, cursor);
			redraw_top_bar = true;
			redraw_tool_bar = true;
		};
		
		
		if (key == sk_Yequ)
		{
			phrase_search_prompt(editor, cursor, editor_index_method);
			redraw_tool_bar = true;
		};
		
		
		if (key == sk_Mode)
		{
			if (editor_index_method == ADDRESS_INDEXING)
			{
				editor_index_method = OFFSET_INDEXING;
			} else {
				editor_index_method = ADDRESS_INDEXING;
			};
			redraw_tool_bar = true;
		};
		
		if (key == sk_Zoom && !cursor->multibyte_selection)
		{
			goto_prompt(editor, cursor, editor_index_method);
			redraw_tool_bar = true;
			delay(200);
		};
		
		if (key == sk_Window && editor->type == FILE_EDITOR && !cursor->multibyte_selection)
		{
			if (insert_bytes_prompt(editor, cursor))
			{
				redraw_top_bar = true;
			};
			redraw_tool_bar = true;
		};
		
		if (key == sk_Trace && editor->num_changes > 0)
		{
			editact_UndoAction(editor, cursor);
			editor->num_changes--;
			redraw_top_bar = true;
			if (editor->num_changes == 0)
				redraw_tool_bar = true;
		};
		
		// If arrow key pressed, move cursor. If two keys are pressed simultaneously,
		// asm_GetCSC only detects the first one it finds in the key registers, so kb_Data
		// should be used for simultaneous keypresses.
		if (kb_Data[7])
		{
			if (kb_Data[7] & kb_Up)
			{
				move_cursor(editor, cursor, CURSOR_UP, kb_Data[2] & kb_Alpha);
			}
			else if (kb_Data[7] & kb_Down)
			{
				move_cursor(editor, cursor, CURSOR_DOWN, kb_Data[2] & kb_Alpha);
			}
			else if (kb_Data[7] & kb_Left)
			{
				if (!cursor->high_nibble)
					cursor->high_nibble = true;
				move_cursor(editor, cursor, CURSOR_LEFT, false);
			}
			else
			{
				move_cursor(editor, cursor, CURSOR_RIGHT, false);
			};
			
			if (!cursor->multibyte_selection)
				redraw_tool_bar = true;
		};
		
		if (key == sk_Clear || key == sk_Graph)
		{
			if (editor->num_changes == 0)
				return;
			
			save_code = save_prompt();
			if (save_code == 1)
			{
				save_file(editor->name, editor->file_type);
				return;
			}
			else if (save_code == 2)
			{
				if (editor->type == RAM_EDITOR)
				{
					// Execute all of the undo actions.
					gui_DrawMessageDialog("Undoing changes to RAM...");
					while (editact_UndoAction(editor, cursor));
					return;
				};
			};
			
			if (save_code > 0)
				return;
			
			redraw_tool_bar = true;
		};
		
		if (editor->max_address - editor->window_address < COLS_ONSCREEN * ROWS_ONSCREEN)
			delay(100);
	};
}

static bool create_undo_appvar(void)
{
	ti_var_t slot;
	
	ti_CloseAll();
	if ((slot = ti_Open(UNDO_APPVAR, "w")) == 0)
	{
		gui_DrawMessageDialog_Blocking("Failed to create undo appvar");
		return false;
	};
	ti_Close(slot);
	return true;
}

static bool create_edit_file(const char *orig_file_name, uint8_t orig_file_type)
{
	ti_var_t file, edit_file;
	uint24_t file_size;
	
	if ((file = ti_OpenVar(orig_file_name, "r", orig_file_type)) == 0)
	{
		gui_DrawMessageDialog_Blocking("Failed to open file");
		return false;
	};
	
	file_size = ti_GetSize(file);
	
	if ((edit_file = ti_Open(EDIT_FILE, "w")) == 0)
	{
		gui_DrawMessageDialog_Blocking("Could not open edit file");
		goto ERROR;
	};
	
	/* If the file size is zero, ti_Resize() will fail */
	if (file_size > 0)
	{
		if (ti_Resize(file_size, edit_file) <= 0)
		{
			gui_DrawMessageDialog_Blocking("Could not resize edit file");
			goto ERROR;
		};
		
		asm_CopyData(ti_GetDataPtr(file), ti_GetDataPtr(edit_file), file_size, 1);
	};
	
	// Debugging
	//dbg_sprintf(dbgout, "Copied file\n");

	ti_CloseAll();
	return true;
	
	ERROR:
	ti_CloseAll();
	return false;
}


static bool bounds_check_offsets(editor_t *editor, uint24_t primary_cursor_offset, uint24_t secondary_cursor_offset)
{
	uint24_t storage_size;
	
	
	storage_size = editor->max_address - editor->min_address;
	
	if (primary_cursor_offset > storage_size || secondary_cursor_offset > storage_size)
		return false;
	else
		return true;
}


bool editor_FileEditor(const char *name, uint8_t type, uint24_t primary_cursor_offset,
uint24_t secondary_cursor_offset)
{
	bool return_val = false;
	ti_var_t slot;
	editor_t *editor;
	cursor_t *cursor;

//dbg_sprintf(dbgout, "name = %s | type = %d\n", name, type);

	if (primary_cursor_offset < secondary_cursor_offset)
		return false;
	
	ti_CloseAll();
	if (!create_edit_file(name, type))
			return false;

	editor = malloc(sizeof(editor_t));
	cursor = malloc(sizeof(cursor_t));

	if (!create_undo_appvar())
		goto RETURN;
	
	if ((slot = ti_Open(EDIT_FILE, "r")) == 0)
		goto RETURN;

//dbg_sprintf(dbgout, "editor = 0x%6x\n", editor);

	memset(editor->name, '\0', EDITOR_NAME_LEN);
	
	if (strlen(name) < EDITOR_NAME_LEN)
		strcpy(editor->name, name);
	else
		goto RETURN;
	
	editor->min_address = ti_GetDataPtr(slot);
	
	if (ti_GetSize(slot) == 0)
	{
		editor->is_file_empty = true;
		editor->max_address = editor->min_address;
	} else {
		editor->is_file_empty = false;
		editor->max_address = editor->min_address + ti_GetSize(slot) - 1;
	};

	ti_Close(slot);
	editor->type = FILE_EDITOR;
	editor->window_address = editor->min_address;
	editor->num_changes = 0;
	editor->file_type = type;
	cursor->high_nibble = true;
	cursor->multibyte_selection = false;

//dbg_sprintf(dbgout, "min_address = 0x%6x | size = %d\n", editor->min_address, ti_GetSize(slot));

	if (!bounds_check_offsets(editor, primary_cursor_offset, secondary_cursor_offset))
		goto RETURN;

	// The editor goto sets cursor->secondary to cursor->primary, so in order to goto
	// and have multi-byte selection, we must goto before setting cursor->secondary.
	cursor->primary = editor->min_address + primary_cursor_offset;
	editact_Goto(editor, cursor, cursor->primary);
	cursor->secondary = editor->min_address + secondary_cursor_offset;

	run_editor(editor, cursor);
	return_val = true;

	RETURN:
	ti_Delete(EDIT_FILE);
	ti_Delete(UNDO_APPVAR);
	free(editor);
	free(cursor);
	return return_val;
}


void editor_RAMEditor(uint24_t primary_cursor_offset, uint24_t secondary_cursor_offset)
{
	editor_t *editor;
	cursor_t *cursor;
	
	
	if (secondary_cursor_offset > primary_cursor_offset)
		return;
	
	if (!create_undo_appvar())
		return;
	
	if ((editor = malloc(sizeof(editor_t))) == NULL)
		return;
	if ((cursor = malloc(sizeof(cursor_t))) == NULL)
	{
		free(editor);
		ti_Delete(UNDO_APPVAR);
		return;
	};
	
	memset(editor->name, '\0', EDITOR_NAME_LEN);
	strcpy(editor->name, "RAM Editor");
	
	editor->min_address = RAM_MIN_ADDRESS;
	editor->max_address = RAM_MAX_ADDRESS;
	editor->window_address = editor->min_address;
	editor->type = RAM_EDITOR;
	editor->num_changes = 0;
	cursor->high_nibble = true;
	cursor->multibyte_selection = false;
	
	if (!bounds_check_offsets(editor, primary_cursor_offset, secondary_cursor_offset))
		goto RETURN;
	
	// The editor goto sets cursor->secondary to cursor->primary, so in order to goto
	// and have multi-byte selection, we must goto before setting cursor->secondary.
	cursor->primary = editor->min_address + primary_cursor_offset;
	editact_Goto(editor, cursor, cursor->primary);
	cursor->secondary = editor->min_address + secondary_cursor_offset;
	
	if (cursor->secondary < cursor->primary)
		cursor->multibyte_selection = true;
	
	run_editor(editor, cursor);
	
	RETURN:
	ti_Delete(UNDO_APPVAR);
	free(editor);
	free(cursor);
	return;
}


void editor_ROMViewer(uint24_t primary_cursor_offset, uint24_t secondary_cursor_offset)
{
	editor_t *editor;
	cursor_t *cursor;
	
	
	if (secondary_cursor_offset > primary_cursor_offset)
		return;
	
	editor = malloc(sizeof(editor_t));
	cursor = malloc(sizeof(cursor_t));
	
	if ((editor = malloc(sizeof(editor_t))) == NULL)
		return;
	if ((cursor = malloc(sizeof(cursor_t))) == NULL)
	{
		free(editor);
		return;
	};
	
	memset(editor->name, '\0', EDITOR_NAME_LEN);
	strcpy(editor->name, "ROM Viewer");
	editor->type = ROM_VIEWER;
	editor->min_address = ROM_MIN_ADDRESS;
	editor->max_address = ROM_MAX_ADDRESS;
	editor->window_address = editor->min_address;
	editor->num_changes = 0;
	cursor->high_nibble = true;
	cursor->multibyte_selection = false;
	
	if (!bounds_check_offsets(editor, primary_cursor_offset, secondary_cursor_offset))
		goto RETURN;
	
	// The editor goto sets cursor->secondary to cursor->primary, so in order to goto
	// and have multi-byte selection, we must goto before setting cursor->secondary.
	cursor->primary = editor->min_address + primary_cursor_offset;
	editact_Goto(editor, cursor, cursor->primary);
	cursor->secondary = editor->min_address + secondary_cursor_offset;
	
	if (cursor->secondary < cursor->primary)
		cursor->multibyte_selection = true;
	
	run_editor(editor, cursor);
	
	RETURN:
	free(editor);
	free(cursor);
	return;
}


static void load_color_override(color_theme_config_t *color_theme_config)
{
	color_theme.background_color = color_theme_config->background_color;
	color_theme.bar_color = color_theme_config->bar_color;
	color_theme.bar_text_color = color_theme_config->bar_text_color;
	color_theme.table_bg_color = color_theme_config->table_bg_color;
	color_theme.table_text_color = color_theme_config->table_text_color;
	color_theme.selected_table_text_color = color_theme_config->selected_table_text_color;
	color_theme.table_selector_color = color_theme_config->table_selector_color;
	color_theme.cursor_color = color_theme_config->cursor_color;
	return;
}


static void run_editor_from_config(void)
{
	const uint8_t FILE_EDITOR_FLAG = FILE_EDITOR;
	const uint8_t RAM_EDITOR_FLAG = RAM_EDITOR;
	const uint8_t ROM_VIEWER_FLAG = ROM_VIEWER;
	const uint8_t COLOR_OVERRIDE_FLAG = (1 << 7);
	
	ti_var_t config_data_slot;
	color_theme_config_t *color_theme_config;
	uint8_t config_byte;
	uint24_t primary_cursor_offset;
	uint24_t secondary_cursor_offset;
	char file_name[EDITOR_NAME_LEN] = {'\0'};
	uint8_t file_type;
	
	
	ti_CloseAll();
	if ((config_data_slot = ti_Open(HS_CONFIG_APPVAR, "r")) == 0)
		return;
	
	ti_Read(&config_byte, sizeof(config_byte), 1, config_data_slot);
	
//dbg_sprintf(dbgout, "offset = %d\n", ti_Tell(config_data_slot));
//dbg_sprintf(dbgout, "config_byte = %2x\n", config_byte);
	
	if (config_byte & (1 << 7))
	{
//dbg_sprintf(dbgout, "About to load color config\n");
		
		if ((color_theme_config = malloc(sizeof(color_theme_config_t))) == NULL)
			return;
		ti_Read(color_theme_config, sizeof(color_theme_config_t), 1, config_data_slot);
		load_color_override(color_theme_config);
		free(color_theme_config);
		config_byte ^= COLOR_OVERRIDE_FLAG;
	};
	
//dbg_sprintf(dbgout, "offset = %d\n", ti_Tell(config_data_slot));
	
	if (config_byte == FILE_EDITOR_FLAG)
	{
		ti_Read(&file_name, FILE_NAME_LEN, 1, config_data_slot);
		ti_Read(&file_type, sizeof(file_type), 1, config_data_slot);
		ti_Read(&primary_cursor_offset, sizeof(primary_cursor_offset), 1, config_data_slot);
		ti_Read(&secondary_cursor_offset, sizeof(secondary_cursor_offset), 1, config_data_slot);
		ti_Close(config_data_slot);
		editor_FileEditor(file_name, file_type, primary_cursor_offset, secondary_cursor_offset);
	}
	else if (config_byte == RAM_EDITOR_FLAG)
	{
		ti_Read(&primary_cursor_offset, sizeof(primary_cursor_offset), 1, config_data_slot);
		ti_Read(&secondary_cursor_offset, sizeof(secondary_cursor_offset), 1, config_data_slot);
		ti_Close(config_data_slot);
		editor_RAMEditor(primary_cursor_offset, secondary_cursor_offset);
	}
	else if (config_byte == ROM_VIEWER_FLAG)
	{
		ti_Read(&primary_cursor_offset, sizeof(primary_cursor_offset), 1, config_data_slot);
		ti_Read(&secondary_cursor_offset, sizeof(secondary_cursor_offset), 1, config_data_slot);
		ti_Close(config_data_slot);
		editor_ROMViewer(primary_cursor_offset, secondary_cursor_offset);
	} else {
		gui_DrawMessageDialog_Blocking("Unknown editor type");
		ti_Close(config_data_slot);
	};
	
	return;
}


void editor_HeadlessStart(void)
{
	editor_t *editor;
	cursor_t *cursor;
	
	if (!create_undo_appvar())
		return;
	
	editor = malloc(sizeof(editor_t));
	cursor = malloc(sizeof(cursor_t));
	
	run_editor_from_config();
	
	ti_Delete(UNDO_APPVAR);
	ti_Delete(HS_CONFIG_APPVAR);
	free(editor);
	free(cursor);
	return;
}