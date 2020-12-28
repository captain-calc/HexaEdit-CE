#include "debug.h"

#include "colors.h"
#include "editor.h"
#include "editor_actions.h"
#include "editor_gui.h"
#include "gui.h"
#include "asmutil.h"

#include <fileioc.h>
#include <graphx.h>
#include <keypadc.h>

#include <math.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


editor_t *editor;
cursor_t *cursor;

/*-----------------------------
IMPORTANT:

If any files are created after the editor and cursor pointers have been set inside the edit file,
the contents of RAM will be shifted by an indeterminate amount, rendering all of the editor and
cursor pointers invaild.

Any files that the editor needs must be created either before the editor and cursor pointers are
set or after the pointers are no longer needed.
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
			{
				decimal += place * j;
			};
		};
		
		place *= 16;
	};
	
	//dbg_sprintf(dbgout, "%s -> %d\n", hex, decimal);
	
	return decimal;
}

static void move_cursor(uint8_t direction, bool accelerated_cursor)
{
	uint8_t i;
	uint8_t *old_cursor_address;
	
	// dbg_sprintf(dbgout, "direction = %d\n", direction);
	
	if (direction == CURSOR_LEFT && cursor->primary > editor->min_address)
	{
		cursor->primary--;
	};
	
	if (direction == CURSOR_RIGHT && cursor->primary < editor->max_address)
	{
		cursor->primary++;
	};
	
	if (direction == CURSOR_DOWN)
	{
		old_cursor_address = cursor->primary;
		while (cursor->primary < (old_cursor_address + COLS_ONSCREEN) && cursor->primary < editor->max_address)
			cursor->primary++;
		
		if (accelerated_cursor)
		{
			i = (ROWS_ONSCREEN - 1) * COLS_ONSCREEN;
			while (cursor->primary < editor->max_address && i-- > 0)
			{
				cursor->primary++;
			};
		};
	};
	
	if (direction == CURSOR_UP)
	{
		old_cursor_address = cursor->primary;
		while (cursor->primary > old_cursor_address - COLS_ONSCREEN && cursor->primary > editor->min_address)
			cursor->primary--;
		
		if (accelerated_cursor) {
			i = (ROWS_ONSCREEN - 1) * COLS_ONSCREEN;
			while (cursor->primary > editor->min_address && i-- > 0)
			{
				cursor->primary--;
			};
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

static void goto_prompt(char buffer[], uint8_t buffer_size)
{
	uint24_t goto_input;
	char *keymap[1] = {GOTO_HEX};
	
	if (editor->type == FILE_EDITOR)
	{
		keymap[0] = NUMBERS;
	};
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	gfx_PrintStringXY("Goto:", 5, 226);
	gfx_SetColor(BLACK);
	gfx_FillRectangle_NoClip(50, 223, 102, FONT_HEIGHT + 6);
	gfx_SetColor(WHITE);
	gfx_FillRectangle_NoClip(51, 224, 100, FONT_HEIGHT + 4);
	gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	goto_input = (uint24_t)decimal(gui_Input(buffer, buffer_size, keymap, 0, 1, 52, 225, 99, FONT_HEIGHT + 4));
	
	if (!(kb_Data[6] & kb_Clear))
	{
		editact_Goto(editor, cursor, goto_input);
	};
	return;
}

static bool insert_bytes_prompt(char buffer[], uint8_t buffer_size)
{
	uint24_t num_bytes_insert;
	char *keymap[1] = {NUMBERS};
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	gfx_PrintStringXY("Insert:", 5, 226);
	gfx_SetColor(BLACK);
	gfx_FillRectangle_NoClip(60, 223, 102, FONT_HEIGHT + 6);
	gfx_SetColor(WHITE);
	gfx_FillRectangle_NoClip(61, 224, 100, FONT_HEIGHT + 4);
	gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	num_bytes_insert = (uint24_t)atoi(gui_Input(buffer, buffer_size, keymap, 0, 1, 62, 225, 99, FONT_HEIGHT + 4));
	
	if (editact_CreateUndoInsertBytesAction(editor, cursor, num_bytes_insert))
	{
		if (editact_InsertBytes(editor, cursor->primary, num_bytes_insert))
		{
			editor->num_changes++;
			return true;
		};
	};
	return false;
}

static uint8_t save_prompt(void)
{
	int8_t key;
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
	
	gfx_PrintStringXY("Save changes?", 5, 226);
	gfx_PrintStringXY("No", 152, 226);
	gfx_PrintStringXY("Yes", 226, 226);
	gfx_PrintStringXY("Cancel", 270, 226);
	gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	
	delay(200);
	
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

	/* Open the editor's edit file and another file that will become
	   the new changed file. */
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

	// Make the new file as big as the temporary file
	if (edit_file_size > 0) {
		if (ti_Resize(edit_file_size, new_file) <= 0)
		{
			ti_CloseAll();
			gui_DrawMessageDialog_Blocking("Failed to resize new file");
			return false;
		};

		// Copy the contents of the temporary file into the new file
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
		if (!ti_SetArchiveStatus(1, new_file))
			goto ERROR_MEM;

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

static void run_editor(void)
{
	int8_t key;
	bool redraw_top_bar = true;
	bool redraw_tool_bar = true;
	uint8_t save_code;
	
	char buffer[7] = {'\0'};
	
	//dbg_sprintf(dbgout, "window_address = 0x%6x | max_address = 0x%6x\n", editor->window_address, editor->max_address);
	
	for (;;)
	{
		gfx_SetColor(LT_GRAY);
		gfx_FillRectangle_NoClip(0, 20, LCD_WIDTH, LCD_HEIGHT - 40);
		
		if (editor->type == FILE_EDITOR)
		{
			editorgui_DrawFileOffsets(editor, 5, 22);
		} else {
			editorgui_DrawMemAddresses(editor, 5, 22);
		};
		
		gfx_SetColor(BLACK);
		gfx_VertLine_NoClip(58, 20, LCD_HEIGHT - 40);
		gfx_VertLine_NoClip(59, 20, LCD_HEIGHT - 40);
		gfx_VertLine_NoClip(228, 20, LCD_HEIGHT - 40);
		gfx_VertLine_NoClip(229, 20, LCD_HEIGHT - 40);
		gfx_SetColor(WHITE);
		gfx_FillRectangle_NoClip(60, 20, 168, LCD_HEIGHT - 40);
		
		if (editor->type == FILE_EDITOR && editor->is_file_empty)
		{
			editorgui_DrawEmptyFileMessage(60, 25);
		}
		else
		{
			editorgui_DrawHexTable(editor, cursor, 65, 22);
			editorgui_DrawAsciiTable(editor, cursor, 235, 22);
		};
		
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
		//dbg_sprintf(dbgout, "key = %d\n", key);
		
		/* Since pressing '0' writes a NULL nibble, it is a special case. */
		if ((EDITOR_HEX[key] != '\0' || key == sk_0) && !cursor->multibyte_selection && (editor->type == FILE_EDITOR || editor->type == RAM_EDITOR))
		{
			if (editact_GetNibble(cursor, cursor->primary) != EDITOR_HEX[key])
			{
				if (editact_CreateUndoWriteNibbleAction(editor, cursor, editact_GetNibble(cursor, cursor->primary)))
				{
					editact_WriteNibble(cursor, EDITOR_HEX[key]);
					redraw_top_bar = true;
					redraw_tool_bar = true;
					editor->num_changes++;
				};
			};
			if (cursor->high_nibble)
			{
				cursor->high_nibble = false;
			} else {
				move_cursor(CURSOR_RIGHT, false);
			};
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
		
		if (key == sk_Yequ && !cursor->multibyte_selection)
		{
			goto_prompt(buffer, 6);
			redraw_tool_bar = true;
		};
		
		//dbg_sprintf(dbgout, "min_address = 0x%6x\n", editor->min_address);
		
		if (key == sk_Window && editor->type == FILE_EDITOR && !cursor->multibyte_selection)
		{
			if (insert_bytes_prompt(buffer, 5))
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
			{
				redraw_tool_bar = true;
			};
		};
		
		/* If arrow key pressed, move cursor. If two keys are pressed simultaneously,
		asm_GetCSC only detects the first one it finds in the key registers, so kb_Data
		should be used for simultaneous keypresses. */
		if (kb_Data[7])
		{
			if (kb_Data[7] & kb_Up)
			{
				move_cursor(CURSOR_UP, kb_Data[2] & kb_Alpha);
			}
			else if (kb_Data[7] & kb_Down)
			{
				move_cursor(CURSOR_DOWN, kb_Data[2] & kb_Alpha);
			}
			else if (kb_Data[7] & kb_Left)
			{
				if (!cursor->high_nibble)
				{
					cursor->high_nibble = true;
				};
				move_cursor(CURSOR_LEFT, false);
			}
			else
			{
				move_cursor(CURSOR_RIGHT, false);
			};
			
			if (!cursor->multibyte_selection)
			{
				redraw_tool_bar = true;
			};
		};
		
		if (key == sk_Clear || key == sk_Graph)
		{
			if (editor->num_changes == 0)
			{
				return;
			};
			
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
					/* Execute all of the undo actions. */
					gui_DrawMessageDialog("Undoing changes to RAM...");
					while (editact_UndoAction(editor, cursor));
					return;
				};
			};
			
			if (save_code > 0)
			{
				return;
			};
			
			redraw_tool_bar = true;
		};
		
		if (editor->max_address - editor->min_address < COLS_ONSCREEN * ROWS_ONSCREEN)
			delay(200);
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

static bool create_edit_file(char *name, uint8_t type)
{
	ti_var_t file, edit_file;
	uint24_t file_size;
	
	ti_CloseAll();
	if ((file = ti_OpenVar(name, "r", type)) == 0)
	{
		gui_DrawMessageDialog_Blocking("Failed to open file");
		goto ERROR;
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

static bool is_file_accessible(char *name, uint8_t type)
{
	ti_var_t slot;
	
	ti_CloseAll();
	if ((slot = ti_OpenVar(name, "r", type)) == 0)
	{
		ti_Close(slot);
		return false;
	};
	ti_Close(slot);
	return true;
}

static bool file_normal_start(const char *name, uint8_t type)
{
	ti_var_t slot;
	
	memset(editor->name, '\0', EDITOR_NAME_LEN);
	if (strlen(name) <= EDITOR_NAME_LEN)
	{
		strcpy(editor->name, name);
	} else {
		return false;
	};
	
	ti_CloseAll();
	slot = ti_Open(EDIT_FILE, "r");
	
	editor->min_address = ti_GetDataPtr(slot);
	
	//dbg_sprintf(dbgout, "min_address = 0x%6x\n", editor->min_address);
	
	editor->max_address = editor->min_address + ti_GetSize(slot) - 1;
	editor->window_address = editor->min_address;
	editor->num_changes = 0;
	editor->file_type = type;
	editor->is_file_empty = false;
	if (ti_GetSize(slot) == 0)
	{
		editor->is_file_empty = true;
	};
	editor->type = FILE_EDITOR;
	ti_Close(slot);
	
	cursor->primary = editor->min_address;
	cursor->secondary = editor->min_address;
	cursor->high_nibble = true;
	cursor->multibyte_selection = false;
	
	return true;
}

void editor_FileNormalStart(char *name, uint8_t type)
{
	if (!is_file_accessible(name, type))
	{
		gui_DrawMessageDialog_Blocking("Could not open file");
		return;
	};
	
	editor = malloc(sizeof(editor_t));
	cursor = malloc(sizeof(cursor_t));
	
	if (!create_edit_file(name, type))
	{
		gui_DrawMessageDialog_Blocking("Could not create edit file");
		goto RETURN;
	};
	
	if (!file_normal_start(name, type))
	{
		gui_DrawMessageDialog_Blocking("File name is too long");
		goto RETURN;
	};
	
	if (!create_undo_appvar())
	{
		goto RETURN;
	};
	
	run_editor();
	ti_Delete(UNDO_APPVAR);
	ti_Delete(EDIT_FILE);
	
	RETURN:
	free(editor);
	free(cursor);
	return;
}

static void RAM_normal_start(void)
{
	memset(editor->name, '\0', EDITOR_NAME_LEN);
	strcpy(editor->name, "RAM Editor");
	
	editor->min_address = RAM_MIN_ADDRESS;
	editor->max_address = RAM_MAX_ADDRESS;
	editor->window_address = editor->min_address;
	editor->type = RAM_EDITOR;
	editor->num_changes = 0;
	cursor->primary = editor->min_address;
	cursor->secondary = cursor->primary;
	cursor->high_nibble = true;
	cursor->multibyte_selection = false;
	return;
}

void editor_RAMNormalStart(void)
{
	editor = malloc(sizeof(editor_t));
	cursor = malloc(sizeof(cursor_t));
	
	RAM_normal_start();
	
	if (!create_undo_appvar())
	{
		goto RETURN;
	};
	
	run_editor();
	ti_Delete(UNDO_APPVAR);
	
	RETURN:
	free(editor);
	free(cursor);
	return;
}

void editor_ROMViewer(void)
{
	editor = malloc(sizeof(editor_t));
	cursor = malloc(sizeof(cursor_t));
	
	memset(editor->name, '\0', EDITOR_NAME_LEN);
	strcpy(editor->name, "ROM Viewer");
	editor->type = ROM_VIEWER;
	editor->min_address = ROM_MIN_ADDRESS;
	editor->max_address = ROM_MAX_ADDRESS;
	editor->window_address = editor->min_address;
	editor->num_changes = 0;
	cursor->primary = editor->window_address;
	cursor->secondary = cursor->primary;
	cursor->high_nibble = true;
	cursor->multibyte_selection = false;
	
	run_editor();
	
	free(editor);
	free(cursor);
	return;
}

static bool get_config_data(char *config_appvar_name)
{
	ti_var_t config_appvar, file;
	uint24_t window_offset, cursor_primary_offset, cursor_secondary_offset;
	
	memset(editor->name, '\0', EDITOR_NAME_LEN);
	config_appvar = ti_Open(config_appvar_name, "r");
	ti_Read(&editor->type, 1, 1, config_appvar);
	editor->num_changes = 0;
	
	if (editor->type == RAM_EDITOR || editor->type == ROM_VIEWER)
	{
		ti_Read(&editor->window_address, 3, 1, config_appvar);
		ti_Read(&cursor->primary, 3, 1, config_appvar);
		ti_Read(&cursor->secondary, 3, 1, config_appvar);
		if (editor->type == RAM_EDITOR)
		{
			strcpy(editor->name, "RAM Editor");
			editor->min_address = RAM_MIN_ADDRESS;
			editor->max_address = RAM_MAX_ADDRESS;
		} else {
			strcpy(editor->name, "ROM Viewer");
			editor->min_address = ROM_MIN_ADDRESS;
			editor->max_address = ROM_MAX_ADDRESS;
		};
		
		/* Sanity checking. */
		if (cursor->primary < cursor->secondary || editor->window_address < editor->min_address || editor->window_address > editor->max_address || cursor->primary < editor->min_address || cursor->primary > editor->max_address || cursor->secondary < editor->min_address || cursor->secondary > editor->max_address)
		{
			gui_DrawMessageDialog_Blocking("Invaild cursor or window addresses");
			return false;
		}
		
		cursor->high_nibble = true;
		cursor->multibyte_selection = false;
		if (cursor->primary != cursor->secondary)
			cursor->multibyte_selection = true;
		ti_Close(config_appvar);
		return true;
	};
	
	ti_Read(&editor->name, 8, 1, config_appvar);
	ti_Read(&editor->file_type, 1, 1, config_appvar);
	ti_Read(&window_offset, 3, 1, config_appvar);
	ti_Read(&cursor_primary_offset, 3, 1, config_appvar);
	ti_Read(&cursor_secondary_offset, 3, 1, config_appvar);
	ti_Close(config_appvar);
	
	//dbg_sprintf(dbgout, "name = %s | type = %d\n", editor->name, editor->file_type);
	
	if ((file = ti_OpenVar(editor->name, "r", editor->file_type)) == 0)
	{
		gui_DrawMessageDialog_Blocking("Could not open file");
		return false;
	};
	
	editor->min_address = ti_GetDataPtr(file);
	editor->max_address = editor->min_address + ti_GetSize(file) - 1;
	editor->is_file_empty = false;
	if (ti_GetSize(file) == 0)
		editor->is_file_empty = true;
	ti_Close(file);
	
	editor->window_address = editor->min_address + window_offset;
	cursor->primary = editor->min_address + cursor_primary_offset;
	cursor->secondary = editor->min_address + cursor_secondary_offset;
	
	/* Sanity checking. */
	if (cursor->primary < cursor->secondary || editor->window_address < editor->min_address || editor->window_address > editor->max_address || cursor->primary < editor->min_address || cursor->primary > editor->max_address || cursor->secondary < editor->min_address || cursor->secondary > editor->max_address)
	{
		gui_DrawMessageDialog_Blocking("Invaild cursor or window addresses");
		return false;
	}
	
	
	cursor->high_nibble = true;
	cursor->multibyte_selection = false;
	if (cursor->primary != cursor->secondary)
		cursor->multibyte_selection = true;
	return true;
}

void editor_HeadlessStart(char *config_appvar_name)
{
	if (!is_file_accessible(config_appvar_name, TI_APPVAR_TYPE))
	{
		gui_DrawMessageDialog_Blocking("Could not open file config appvar");
		return;
	};
	
	if (!create_undo_appvar())
	{
		gui_DrawMessageDialog_Blocking("Could not create undo file");
		return;
	};
	
	editor = malloc(sizeof(editor_t));
	cursor = malloc(sizeof(cursor_t));
	
	if (!get_config_data(config_appvar_name))
	{
		goto RETURN;
	};
	
	run_editor();
	
	RETURN:
	ti_Delete(UNDO_APPVAR);
	
	/* Delete the configuraton appvar, so HexaEdit can be run normally the next
	time it is opened. */
	ti_Delete(config_appvar_name);
	free(editor);
	free(cursor);
	return;
}