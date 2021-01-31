#include "colors.h"
#include "editor.h"
#include "editor_gui.h"
#include "gui.h"

#include <graphx.h>

#include <math.h>
#include <stdint.h>

static void draw_editing_size(editor_t *editor)
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

void editorgui_DrawTopBar(editor_t *editor)
{
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(0, 0, 320, 20);
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	
	draw_editing_size(editor);

	gfx_SetTextXY(5, 6);
	
	if (editor->num_changes > 0)
	{
		gfx_PrintString("* ");
	};
	gui_PrintFileName(editor->name, color_theme.bar_text_color);
	
	gui_DrawBatteryStatus();
	return;
}

void editorgui_DrawToolBar(editor_t *editor)
{
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	
	gfx_PrintStringXY("Find", 5, 226);
	
	gfx_PrintStringXY("Goto", 138, 226);
	
	if (editor->type == FILE_EDITOR)
	{
		gfx_PrintStringXY("Ins", 70, 226);
	};
	if (editor->num_changes > 0)
	{
		gfx_PrintStringXY("Undo", 224, 226);
	};
	gfx_PrintStringXY("Exit", 286, 226);
	return;
}

void editorgui_DrawAltToolBar(cursor_t *cursor)
{
	uint8_t i;
	uint24_t byte_value = 0;
	
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, 280, 20);
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	
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

void editorgui_DrawMemAddresses(editor_t *editor, uint24_t x, uint8_t y)
{
	uint8_t row = 0;
	uint8_t byte;
	char hex[7] = {'\0'};
	
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

void editorgui_DrawFileOffsets(editor_t *editor, uint24_t x, uint8_t y)
{
	uint8_t row = 0;
	
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

static bool is_current_byte_selected(cursor_t *cursor, uint8_t *line_ptr, uint8_t byte_num, uint24_t num_bytes_selected)
{
	if (((line_ptr + byte_num) - cursor->secondary) < (int)num_bytes_selected)
	{
		if (((line_ptr + byte_num) - cursor->secondary) >= 0)
			return true;
	};
	return false;
}

static void print_hex_line(editor_t *editor, cursor_t *cursor, uint24_t x, uint8_t y, uint8_t *line)
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
		if (is_current_byte_selected(cursor, line, byte_num, num_bytes_selected))
		{
			gfx_SetTextBGColor(color_theme.cursor_color);
			gfx_SetTextFGColor(color_theme.selected_table_text_color);
			gfx_SetTextTransparentColor(color_theme.cursor_color);
			gfx_SetColor(color_theme.cursor_color);
			gfx_FillRectangle_NoClip(x - 1, y - 1, HEX_COL_WIDTH, ROW_HEIGHT);
			if ((line + byte_num) == cursor->primary)
			{
				gfx_SetColor(color_theme.table_text_color);
				gfx_HorizLine_NoClip(x - 1 + (9 * !cursor->high_nibble), y + FONT_HEIGHT + 1, 9);
			}
		} else {
			gfx_SetTextBGColor(color_theme.table_bg_color);
			gfx_SetTextFGColor(color_theme.table_text_color);
			gfx_SetTextTransparentColor(color_theme.table_bg_color);
		};
		
		print_hex_value(x, y, *(line + byte_num));
		
		x += HEX_COL_WIDTH;
		byte_num++;
	};
}

void editorgui_DrawHexTable(editor_t *editor, cursor_t *cursor, uint24_t x, uint8_t y)
{
	uint8_t line = 0;
	
	for (;;)
	{
		if (line > ROWS_ONSCREEN)
		{
			return;
		};
		print_hex_line(editor, cursor, x, y + (line * ROW_HEIGHT), editor->window_address + (line * COLS_ONSCREEN));
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

static void print_ascii_line(editor_t *editor, cursor_t *cursor, uint24_t x, uint8_t y, uint8_t *line)
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
		if (is_current_byte_selected(cursor, line, byte_num, num_bytes_selected))
		{
			gfx_SetTextBGColor(color_theme.cursor_color);
			gfx_SetTextFGColor(color_theme.selected_table_text_color);
			gfx_SetTextTransparentColor(color_theme.cursor_color);
			gfx_SetColor(color_theme.cursor_color);
			gfx_FillRectangle_NoClip(x - 1, y - 1, ASCII_COL_WIDTH, ROW_HEIGHT);
		} else {
			gfx_SetTextBGColor(color_theme.background_color);
			gfx_SetTextFGColor(color_theme.table_text_color);
			gfx_SetTextTransparentColor(color_theme.background_color);
		};
		
		print_ascii_value(x, y, *(line + byte_num));
		
		x += ASCII_COL_WIDTH;
		byte_num++;
	};
}

void editorgui_DrawAsciiTable(editor_t *editor, cursor_t *cursor, uint24_t x, uint8_t y)
{
	uint8_t line = 0;
	
	for (;;)
	{
		if (line > ROWS_ONSCREEN)
		{
			return;
		};
		print_ascii_line(editor, cursor, x, y + (line * ROW_HEIGHT), editor->window_address + (line * COLS_ONSCREEN));
		line++;
	};
}

void editorgui_DrawEmptyFileMessage(uint24_t hex_x, uint8_t y)
{
	char message[] = "-- Empty --";
	
	gfx_SetTextBGColor(color_theme.background_color);
	gfx_SetTextFGColor(color_theme.table_text_color);
	gfx_SetTextTransparentColor(color_theme.background_color);
	gfx_SetTextXY(hex_x + ((COLS_ONSCREEN * HEX_COL_WIDTH) - gfx_GetStringWidth(message)) / 2, y);
	gfx_PrintString(message);
	return;
}