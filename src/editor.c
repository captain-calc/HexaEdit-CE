#include "debug.h"

#include "colors.h"
#include "editor.h"
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


/* Define common error messages as globals. Saves 7 bytes. */
const char *EDIT_FILE_OPEN_FAIL = "Could not open edit file";
const char *EDIT_FILE_RESIZE_FAIL = "Could not resize edit file";
const char *UNDO_APPVAR_OPEN_FAIL = "Could not open undo appvar";
const char *UNDO_APPVAR_RESIZE_FAIL = "Could not resize undo appvar";


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
		
		for (j = 0; j < 15; j++)
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

static void sprite_viewer(void)
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

static void gotof(uint24_t offset)
{
	//dbg_sprintf(dbgout, "min_address = 0x%6x | window_address = 0x%6x\n", editor->min_address, editor->window_address);
	
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

static bool delete_bytes(uint8_t *deletion_point, uint24_t num_bytes)
{
	/* When the TI-OS resizes a file, any pointers to data within it
	   become inaccurate. It also does not add or remove bytes
	   from the end of the file, but from the start of the file */
	
	/* The above means that the editor->min_address and editor->max_address will be inaccurate */
	
	/* If cursor->primary is at the end of the file and the number of bytes requested to be deleted
	   includes the byte the cursor is on, move the cursor->primary to the last byte of the file */
	
	uint24_t num_bytes_shift = deletion_point - editor->min_address;
	
	//dbg_sprintf(dbgout, "num_bytes_shift = %d | num_bytes = %d\n", num_bytes_shift, num_bytes);
	
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
		asm_CopyData(deletion_point - 1, cursor->primary, num_bytes_shift, 0);
	};
	
	//dbg_sprintf(dbgout, "Before re-assignment\neditor->min_address = 0x%6x\n", editor->min_address);
	//dbg_sprintf(dbgout, "primary = 0x%6x | secondary = 0x%6x\n", cursor->primary, cursor->secondary);
	
	if (ti_Resize(ti_GetSize(edit_file) - num_bytes, edit_file) != -1)
	{
		editor->min_address = ti_GetDataPtr(edit_file);
		
		/* The minus one is very important. If the file size is 0, max_address == min_address - 1.*/
		if (ti_GetSize(edit_file) == 0)
		{
			editor->max_address = editor->min_address + ti_GetSize(edit_file);
		}
		else
		{
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
	
	//dbg_sprintf(dbgout, "After re-assignment\neditor->min_address = 0x%6x\n", editor->min_address);
	
	ti_Close(edit_file);
	return true;
}

static bool insert_bytes(uint8_t *insertion_point, uint24_t num_bytes)
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
	
	editor->max_address += num_bytes;
	editor->is_file_empty = false;
	
	ti_Close(edit_file);
	return true;
	
	ERROR:
	ti_Close(edit_file);
	return false;
}

static bool create_undo_insert_bytes_action(uint24_t num_bytes)
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

static void undo_insert_bytes(ti_var_t undo_appvar)
{
	uint24_t num_bytes;
	
	ti_Read(&editor->window_address, 3, 1, undo_appvar);
	ti_Read(&cursor->primary, 3, 1, undo_appvar);
	ti_Read(&num_bytes, 3, 1, undo_appvar);
	
	//dbg_sprintf(dbgout, "num_bytes = %d\n", num_bytes);
	
	delete_bytes(cursor->primary, num_bytes);
	
	if (ti_Resize(ti_GetSize(undo_appvar) - ti_Tell(undo_appvar), undo_appvar) == -1)
	{
		gui_DrawMessageDialog_Blocking("Could not delete undo action");
		return;
	};
	
	cursor->secondary = cursor->primary;
	return;
}

static bool create_delete_bytes_undo_action(uint24_t num_bytes)
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

static void undo_delete_bytes(ti_var_t undo_appvar)
{
	uint24_t num_bytes, i;
	
	ti_Read(&editor->window_address, 3, 1, undo_appvar);
	ti_Read(&cursor->secondary, 3, 1, undo_appvar);
	ti_Read(&num_bytes, 3, 1, undo_appvar);
	
	//dbg_sprintf(dbgout, "num_bytes = %d\n", num_bytes);
	
	insert_bytes(cursor->secondary, num_bytes);
	
	//dbg_sprintf(dbgout, "Inserted bytes for undo-ing.\n");
	
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

static uint8_t get_nibble(uint8_t *ptr)
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

static void write_nibble(uint8_t nibble)
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

static bool create_undo_write_nibble_action(uint8_t nibble)
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

static void undo_write_nibble(ti_var_t undo_appvar)
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
	
	write_nibble(nibble);
	cursor->secondary = cursor->primary;
	return;
}

static bool undo_action(void)
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
	
	//dbg_sprintf(dbgout, "undo_code = %d\n", undo_code);
	
	switch(undo_code)
	{
		case UNDO_INSERT_BYTES:
		undo_insert_bytes(undo_appvar);
		ti_Close(undo_appvar);
		return true;
		
		case UNDO_DELETE_BYTES:
		undo_delete_bytes(undo_appvar);
		ti_Close(undo_appvar);
		return true;
		
		case UNDO_WRITE_NIBBLE:
		undo_write_nibble(undo_appvar);
		ti_Close(undo_appvar);
		return true;
		
		default:
		ti_Close(undo_appvar);
		return false;
	};
}

static void draw_editing_size(void)
{
	uint8_t magnitude = 6;
	
	if (editor->type == ROM_VIEWER)
	{
		magnitude = 7;
	};
	
	gfx_SetTextXY(100, 6);
	if (editor->type == FILE_EDITOR && editor->is_file_empty)
	{
		gfx_PrintUInt(0, 6);
	}
	else
	{
		gfx_PrintUInt(editor->max_address - editor->min_address + 1, magnitude);
	};
	gfx_PrintString(" B");
	return;
}

static void draw_top_bar(void)
{
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, 0, 320, 20);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
	
	draw_editing_size();

	gfx_SetTextXY(5, 6);
	
	if (editor->num_changes > 0)
	{
		gfx_PrintString("* ");
	};
	gui_PrintFileName(editor->name);
	
	gui_DrawBatteryStatus();
	return;
}

static void draw_tool_bar(void)
{
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
	gfx_PrintStringXY("Goto", 5, 226);
	if (editor->type == FILE_EDITOR)
	{
		gfx_PrintStringXY("Ins", 70, 226);
	};
	if (editor->num_changes > 0)
	{
		gfx_PrintStringXY("Undo", 226, 226);
	};
	gfx_PrintStringXY("Exit", 286, 226);
	return;
}

static void draw_alternate_tool_bar(void)
{
	uint8_t i;
	uint24_t byte_value = 0;
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, 220, 140, 20);
	gfx_FillRectangle_NoClip(226, 220, 50, 20);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
	
	if ((cursor->primary - cursor->secondary) < 3)
	{
		gfx_PrintStringXY("DEC:", 5, 226);
		gfx_SetTextXY(40, 226);
		
		for (i = 0; i < (cursor->primary - cursor->secondary + 1); i++)
		{
			byte_value += (*(cursor->secondary + i) << (8 * i));
		};
		
		gfx_PrintUInt(byte_value, log10((double)byte_value + 10));
	};
	return;
}

static void draw_mem_addresses(uint24_t x, uint8_t y)
{
	uint8_t row = 0;
	uint8_t byte;
	char hex[7] = {'\0'};
	
	gfx_SetTextBGColor(LT_GRAY);
	gfx_SetTextFGColor(BLACK);
	gfx_SetTextTransparentColor(LT_GRAY);
	
	for (;;)
	{
		if (row > ROWS_ONSCREEN || (editor->window_address + (row * COLS_ONSCREEN)) > editor->max_address)
		{
			return;
		};
		
		gfx_SetTextXY(x, y);
		sprintf(hex, "%6x", (unsigned int)(editor->window_address + (row * COLS_ONSCREEN)));
		byte = 0;
		
		while (*(hex + byte) == ' ')
		{
			*(hex + byte++) = '0';
		};
		gfx_PrintString(hex);
		
		row++;
		y += ROW_HEIGHT;
	};
	
	return;
}

static void draw_file_offsets(uint24_t x, uint8_t y)
{
	uint8_t row = 0;
	
	gfx_SetTextBGColor(LT_GRAY);
	gfx_SetTextFGColor(BLACK);
	gfx_SetTextTransparentColor(LT_GRAY);
	for (;;)
	{
		if (row > ROWS_ONSCREEN || (editor->window_address + (row * COLS_ONSCREEN)) > editor->max_address)
		{
			return;
		};
		gfx_SetTextXY(x, y);
		gfx_PrintUInt((editor->window_address - editor->min_address) + (row * COLS_ONSCREEN), 6);
		row++;
		y += ROW_HEIGHT;
	};
}

static void print_hex_value(uint24_t x, uint8_t y, uint8_t value)
{
	char hex[3] = {'\0'};
	
	gfx_SetTextXY(x, y);
	sprintf(hex, "%2x", value);
	if (*hex == ' ')
	{
		*hex = '0';
	}
	gfx_PrintString(hex);
	return;
}

static void print_hex_line(uint24_t x, uint8_t y, uint8_t *line)
{
	uint8_t byte_num = 0;
	uint24_t num_bytes_selected;
	
	num_bytes_selected = cursor->primary - cursor->secondary;
	/*
	If the primary and secondary addresses are the same, num_bytes_selected
	will initially be zero.
	*/
	num_bytes_selected++;
	
	for (;;)
	{
		if (byte_num == COLS_ONSCREEN || (line + byte_num) > editor->max_address)
		{
			return;
		};
		if (((line + byte_num) - cursor->secondary) < (int)num_bytes_selected && ((line + byte_num) - cursor->secondary) >= 0)
		{
			gfx_SetTextBGColor(CURSOR_COLOR);
			gfx_SetTextFGColor(WHITE);
			gfx_SetTextTransparentColor(CURSOR_COLOR);
			gfx_SetColor(CURSOR_COLOR);
			gfx_FillRectangle_NoClip(x - 1, y - 1, HEX_COL_WIDTH, ROW_HEIGHT);
			if ((line + byte_num) == cursor->primary)
			{
				gfx_SetColor(BLACK);
				gfx_HorizLine_NoClip(x - 1 + (9 * !cursor->high_nibble), y + FONT_HEIGHT + 1, 9);
			}
		} else {
			gfx_SetTextBGColor(LT_GRAY);
			gfx_SetTextFGColor(BLACK);
			gfx_SetTextTransparentColor(LT_GRAY);
		};
		
		print_hex_value(x, y, *(line + byte_num));
		
		x += HEX_COL_WIDTH;
		byte_num++;
	};
}

static void draw_hex_table(uint24_t x, uint8_t y)
{
	uint8_t line = 0;
	
	for (;;)
	{
		if (line > ROWS_ONSCREEN)
		{
			return;
		};
		print_hex_line(x, y + (line * ROW_HEIGHT), editor->window_address + (line * COLS_ONSCREEN));
		line++;
	};
}

static void print_ascii_value(uint24_t x, uint8_t y, uint8_t c)
{
	gfx_SetTextXY(x, y);
	if (c < 20 || c > 127)
	{
		gfx_PrintChar('.');
	} else {
		gfx_PrintChar(c);
	};
	return;
}

static void print_ascii_line(uint24_t x, uint8_t y, uint8_t *line)
{
	uint8_t byte_num = 0;
	uint24_t num_bytes_selected;
	
	num_bytes_selected = cursor->primary - cursor->secondary;
	/*
	If the primary and secondary addresses are the same, num_bytes_selected
	will initially be zero.
	*/
	num_bytes_selected++;
	
	for (;;)
	{
		if (byte_num == COLS_ONSCREEN || (line + byte_num) > editor->max_address)
		{
			return;
		};
		if (((line + byte_num) - cursor->secondary) < (int)num_bytes_selected && ((line + byte_num) - cursor->secondary) >= 0)
		{
			gfx_SetTextBGColor(CURSOR_COLOR);
			gfx_SetTextFGColor(WHITE);
			gfx_SetTextTransparentColor(CURSOR_COLOR);
			gfx_SetColor(CURSOR_COLOR);
			gfx_FillRectangle_NoClip(x - 1, y - 1, ASCII_COL_WIDTH, ROW_HEIGHT);
		} else {
			gfx_SetTextBGColor(LT_GRAY);
			gfx_SetTextFGColor(BLACK);
			gfx_SetTextTransparentColor(LT_GRAY);
		};
		
		print_ascii_value(x, y, *(line + byte_num));
		
		x += ASCII_COL_WIDTH;
		byte_num++;
	};
}

static void draw_ascii_table(uint24_t x, uint8_t y)
{
	uint8_t line = 0;
	
	for (;;)
	{
		if (line > ROWS_ONSCREEN)
		{
			return;
		};
		print_ascii_line(x, y + (line * ROW_HEIGHT), editor->window_address + (line * COLS_ONSCREEN));
		line++;
	};
}

static void draw_empty_file_message(uint24_t hex_x, uint8_t y)
{
	char message[] = "-- Empty --";
	
	gfx_SetTextBGColor(LT_GRAY);
	gfx_SetTextFGColor(BLACK);
	gfx_SetTextTransparentColor(LT_GRAY);
	gfx_SetTextXY(hex_x + ((COLS_ONSCREEN * HEX_COL_WIDTH) - gfx_GetStringWidth(message)) / 2, y);
	gfx_PrintString(message);
	return;
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
		gotof(goto_input);
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
	
	if (create_undo_insert_bytes_action(num_bytes_insert))
	{
		if (insert_bytes(cursor->primary, num_bytes_insert))
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
		gui_DrawMessageDialog_Blocking(EDIT_FILE_OPEN_FAIL);
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
			draw_file_offsets(5, 22);
		} else {
			draw_mem_addresses(5, 22);
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
			draw_empty_file_message(60, 25);
		}
		else
		{
			draw_hex_table(65, 22);
			draw_ascii_table(235, 22);
		};
		
		if (redraw_top_bar)
		{
			draw_top_bar();
			redraw_top_bar = false;
		};
		
		if (redraw_tool_bar)
		{
			draw_tool_bar();
			redraw_tool_bar = false;
		}
		
		if (cursor->multibyte_selection)
		{
			draw_alternate_tool_bar();
		};
		
		gfx_BlitBuffer();
		
		do {
			kb_Scan();
		} while ((key = asm_GetCSC()) == -1);
		//dbg_sprintf(dbgout, "key = %d\n", key);
		
		/* Since pressing '0' writes a NULL nibble, it is a special case. */
		if ((EDITOR_HEX[key] != '\0' || key == sk_0) && !cursor->multibyte_selection && (editor->type == FILE_EDITOR || editor->type == RAM_EDITOR))
		{
			if (get_nibble(cursor->primary) != EDITOR_HEX[key])
			{
				if (create_undo_write_nibble_action(get_nibble(cursor->primary)))
				{
					write_nibble(EDITOR_HEX[key]);
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
			//dbg_sprintf(dbgout, "editor->min_address = 0x%6x | cursor->secondary = 0x%6x\n", editor->min_address, cursor->secondary);
			if (create_delete_bytes_undo_action(cursor->primary - cursor->secondary + 1))
			{
				/* create_delete_bytes_undo_action() opens files, so all of the editor and cursor
				cursor pointers for the edit file are invalid. delete_bytes() sets them to their
				proper values. */
				//dbg_sprintf(dbgout, "editor->min_address = 0x%6x | cursor->secondary = 0x%6x\n", editor->min_address, cursor->secondary);
				delete_bytes(cursor->secondary, cursor->primary - cursor->secondary + 1);
			};
			cursor->multibyte_selection = false;
			redraw_top_bar = true;
			redraw_tool_bar = true;
			editor->num_changes++;
		};
		
		if (key == sk_Stat && !cursor->multibyte_selection)
		{
			sprite_viewer();
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
			undo_action();
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
					while (undo_action());
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
		gui_DrawMessageDialog_Blocking(EDIT_FILE_OPEN_FAIL);
		goto ERROR;
	};
	
	/* If the file size is zero, ti_Resize() will fail */
	if (file_size > 0)
	{
		if (ti_Resize(file_size, edit_file) <= 0)
		{
			gui_DrawMessageDialog_Blocking(EDIT_FILE_RESIZE_FAIL);
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
	
	dbg_sprintf(dbgout, "name = %s | type = %d\n", editor->name, editor->file_type);
	
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