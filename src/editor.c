#include "defines.h"
#include "easter_eggs.h"
#include "editor.h"
#include "gui.h"
#include "routines.h"

#include <graphx.h>
#include <keypadc.h>
#include <fileioc.h>
#include <tice.h>
#include <debug.h>

#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

editor_t editor;
cursor_t cursor;

void draw_file_size(void) {
	
	gfx_SetTextXY(100, 6);
	gfx_PrintUInt(ti_GetSize(editor.file), 6);
	gfx_PrintStringXY("bytes", 152, 6);
	return;
}

void draw_top_bar(bool file_changed) {
	
	char name[10] = {'\0'};
	if (editor.edit_type == FILE_EDIT_TYPE) {
		if (file_changed)
			*name = '*';
		strcpy((name + file_changed), editor.file_name);
		
		gfx_SetColor(DK_GRAY);
		gfx_FillRectangle_NoClip(0, 0, 320, 20);
		gfx_SetTextBGColor(DK_GRAY);
		gfx_SetTextFGColor(WHITE);
		gfx_SetTextTransparentColor(DK_GRAY);
		gfx_SetColor(WHITE);
		gfx_SetTextXY(5, 6);
		print_file_name(name);
		draw_file_size();
	};
	
	draw_battery_status();
	gfx_BlitRectangle(1, 0, 0, 320, 20);
	return;
}

uint24_t input(const char *prompt, uint8_t char_limit, bool hex_flag) {
	
	uint24_t offset = 0;
	uint8_t offset_width = 0;
	uint8_t prompt_width = 0;
	uint8_t key = 255, i = 0;
	
	const char *hex_chars = "0123456789abcdef";
	const char *numerical_chars = "0123456789";
	char *chars;
	
	chars = numerical_chars;
	if (hex_flag)
		chars = hex_chars;
	
	prompt_width = gfx_GetStringWidth(prompt);
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, 220, 320, 20);
	gfx_SetTextFGColor(WHITE);
	gfx_PrintStringXY(prompt, 5, 226);
	gfx_SetColor(BLACK);
	gfx_FillRectangle_NoClip(8 + prompt_width, 223, 60, 13);
	gfx_SetColor(WHITE);
	gfx_FillRectangle_NoClip(9 + prompt_width, 224, 58, 11);
	
	gfx_SetTextXY(11 + prompt_width, 226);
	gfx_SetTextBGColor(WHITE);
	gfx_SetTextFGColor(BLACK);
	gfx_SetTextTransparentColor(WHITE);
	
	do {
		gfx_SetColor(WHITE);
		gfx_VertLine(11 + prompt_width + offset_width, 225, 9);
		
		if (key < strlen(chars) && i < char_limit) {
			if (hex_flag) {
				offset *= 16;
				offset += key;
			} else {
				offset *= 10;
				offset += key;
			};
			offset_width += gfx_GetCharWidth(chars[key]);
			gfx_PrintChar(chars[key]);
			i++;
		};
		
		if (key == KEY_DEL && i > 0) {
			if (hex_flag) {
				offset |= (0 << (6 - 1));
			} else {
				offset /= 10;
			};
			offset_width -= gfx_GetCharWidth(chars[--i]);
		};
		
		if (key == KEY_CLEAR)
			return 0;
		
		// Debugging
		dbg_sprintf(dbgout, "offset = %6x\n", offset);
		
		gfx_SetColor(BLACK);
		gfx_VertLine(11 + prompt_width + offset_width, 225, 9);
		gfx_BlitRectangle(1, 5, 223, 320, 13);
		delay(200);
	} while ((key = get_keypress()) != KEY_2ND);
	
	return offset;
}

void draw_alt_tool_bar(void) {
	
	uint8_t byte = 0, i;
	uint24_t byte_value = 0;
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, 220, 140, 20);
	gfx_FillRectangle_NoClip(226, 220, 50, 20);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
	if (editor.edit_type == FILE_EDIT_TYPE)
		gfx_PrintStringXY("Del", 148, 226);
	
	if (editor.num_selected_bytes < 4) {
		gfx_PrintStringXY("DEC:", 5, 226);
		gfx_SetTextXY(40, 226);
		
		for (i = 0; i < editor.num_selected_bytes; i++)
			byte_value += *(editor.sel_byte_string_offset + i) << (8 * i);
		// Debugging
		// dbg_sprintf(dbgout, "byte_value = 0x%6x\n", byte_value);
		gfx_PrintUInt(byte_value, log10((double)byte_value + 10));
	};
	gfx_BlitRectangle(1, 0, 220, 320, 20);
	return;
}

void draw_tool_bar(void) {
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, 220, 320, 20);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
	gfx_PrintStringXY("Goto", 5, 226);
	if (editor.edit_type == FILE_EDIT_TYPE) {
		gfx_PrintStringXY("Ins", 70, 226);
		if (editor.undo_activated)
			gfx_PrintStringXY("Undo", 226, 226);
	};
	gfx_PrintStringXY("Exit", 286, 226);
	gfx_BlitRectangle(1, 0, 220, 320, 20);
	return;
}

void draw_address_window(void) {
	
	uint8_t i, j;
	char hex[7] = {'\0'};
	
	draw_window("Addr", false, 5, 25, 60, 190);
	gfx_SetTextFGColor(BLACK);
	
	for (i = 0; i < MAX_LINES_ONSCREEN; i++) {
		j = 0;
		gfx_SetTextXY(10, 45 + 11 * i);
		sprintf(hex, "%6x", editor.edit_offset + 8 * i);
		while (*(hex + j) == ' ')
			*(hex + j++) = '0';
		gfx_PrintString(hex);
	};
	
	return;
}

static void draw_offset_window(void) {

	uint8_t i;

	draw_window("Offset", false, 5, 25, 60, 190);
	gfx_SetTextFGColor(BLACK);

	for (i = 0; i < MAX_LINES_ONSCREEN; i++) {
		gfx_SetTextXY(10, 45 + 11 * i);
		gfx_PrintUInt(editor.edit_offset - editor.min_offset + 8 * i, 6);
	};
	
	return;
}

char *hex(uint8_t byte) {
	static char hex[3] = {'\0'};
	sprintf(hex, "%2x", byte);
	if (*hex == ' ') *hex = '0';
	return hex;
}

void draw_byte_selector(uint8_t sel_nibble, uint8_t pixel_x, uint8_t pixel_y) {
	
	gfx_SetColor(BLUE);
	gfx_FillRectangle_NoClip(pixel_x - 1, pixel_y - 1, 17, 9);
	if (editor.num_selected_bytes == 1) {
		gfx_SetColor(BLACK);
		gfx_HorizLine(pixel_x + 8 - 9 * sel_nibble, pixel_y + 9, 9);
	};
	gfx_SetTextFGColor(WHITE);
	
	return;
}

void draw_printable_bytes(uint8_t byte, bool highlight, uint24_t print_x, uint8_t print_y) {
	
	gfx_SetTextXY(239 + 9 * print_x, 45 + 11 * print_y);

	if (highlight) {
		gfx_SetColor(BLUE);
		gfx_FillRectangle_NoClip(238 + 9 * print_x, 44 + 11 * print_y, 9, 9);
	};
	
	if (byte < 20 || byte > 127)
		gfx_PrintChar('.');
	else
		gfx_PrintChar(byte);
	return;
}

void draw_right_two_windows(uint8_t sel_nibble) {
	
	const char *middle_window_title[3] = {"File Contents", "RAM Contents", "ROM Contents"};
	const char *empty_file_msg = "-- Empty --";
	uint8_t print_x = 0, print_y = 0;
	
	uint8_t *offset;
	
	uint24_t print_offset;
	uint24_t pixel_x;
	uint8_t pixel_y;
	uint8_t adj_sel_byte_string_offset;
	uint8_t byte;
	char *hex_byte;
	
	draw_window(middle_window_title[editor.edit_type], true, 67, 25, 164, 190);
	draw_window("ASCII", false, 234, 25, 81, 190);
	
	offset = editor.edit_offset;
	
	// Debugging
	dbg_sprintf(dbgout, "[editor.c] [draw_right_two_windows()] : edit_offset = 0x%6x\n", offset);
	
	adj_sel_byte_string_offset = (editor.sel_byte_string_offset - editor.edit_offset);
	
	while (offset < editor.max_offset && print_y < MAX_LINES_ONSCREEN) {
		
		pixel_x = 72 + 20 * print_x;
		pixel_y = 45 + 11 * print_y;
		print_offset = MAX_BYTES_PER_LINE * print_y + print_x;
		gfx_SetTextXY(pixel_x, pixel_y);
		hex_byte = hex(*offset);
		
		if (print_offset >= adj_sel_byte_string_offset && print_offset <= adj_sel_byte_string_offset + editor.num_selected_bytes - 1)
			draw_byte_selector(sel_nibble, pixel_x, pixel_y);
		else
			gfx_SetTextFGColor(BLACK);
		gfx_PrintChar(*hex_byte);
		gfx_PrintChar(*(hex_byte + 1));
		if (print_offset >= adj_sel_byte_string_offset && print_offset <= adj_sel_byte_string_offset + editor.num_selected_bytes - 1) {
			draw_printable_bytes(*offset, true, print_x, print_y);
		} else {
			draw_printable_bytes(*offset, false, print_x, print_y);
		};
		
		if (++print_x == MAX_BYTES_PER_LINE) {
			print_y++;
			print_x = 0;
		};
		
		offset++;
	};
	
	if (editor.edit_type == FILE_EDIT_TYPE && ti_GetSize(editor.file) == 0) {
		gfx_SetTextFGColor(LT_GRAY);
		gfx_PrintStringXY(empty_file_msg, 149 - gfx_GetStringWidth(empty_file_msg) / 2, 45);
	};
	
	return;
}

void update_windows(uint8_t sel_nibble) {
	
	editor.Draw_Left_Window();
	draw_right_two_windows(sel_nibble);
	gfx_BlitRectangle(1, 5, 25, 310, 190);
	return;
}

uint8_t get_keypress(void) {
	
	kb_key_t arrows, function, key_row_two, key_row_three, key_row_four, key_row_five;
	
	do {
		kb_Scan();
	} while (!kb_AnyKey());
	
	function = kb_Data[1];
	if (function & kb_Yequ)			return 21;
	if (function & kb_Window)		return 22;
	if (function & kb_Zoom)			return 23;
	if (function & kb_Trace)		return 24;
	if (function & kb_Graph)		return 25;
	if (function & kb_2nd)			return 26;
	if (function & kb_Del)			return 27;
	
	arrows = kb_Data[7];
	if (arrows & kb_Left)			return 16;
	if (arrows & kb_Right)			return 17;
	if (arrows & kb_Up)			return 18;
	if (arrows & kb_Down)			return 19;
	
	key_row_two = kb_Data[2];
	if (key_row_two & kb_Math)		return 10;
	if (key_row_two & kb_Recip)		return 13;
	
	key_row_three = kb_Data[3];
	if (key_row_three & kb_Apps)		return 11;
	if (key_row_three & kb_Sin)		return 14;
	if (key_row_three & kb_7)		return 7;
	if (key_row_three & kb_4)		return 4;
	if (key_row_three & kb_1)		return 1;
	if (key_row_three & kb_0)		return 0;
	
	key_row_four = kb_Data[4];
	if (key_row_four & kb_Prgm)		return 12;
	if (key_row_four & kb_Cos)		return 15;
	if (key_row_four & kb_8)		return 8;
	if (key_row_four & kb_5)		return 5;
	if (key_row_four & kb_2)		return 2;
	
	key_row_five = kb_Data[5];
	if (key_row_five & kb_9)		return 9;
	if (key_row_five & kb_6)		return 6;
	if (key_row_five & kb_3)		return 3;
	
	if (kb_Data[6] & kb_Clear)		return 20;
	
	return 255;
}

void move_cursor(uint8_t key) {
	
	uint8_t i;
	uint8_t *old_cursor_offset;
	
	if (key == KEY_LEFT && editor.cursor_offset > editor.min_offset)
		editor.cursor_offset--;
	
	if (key == KEY_RIGHT && editor.cursor_offset + 1 < editor.max_offset)
		editor.cursor_offset++;
	
	if (key == KEY_DOWN) {
		old_cursor_offset = editor.cursor_offset;
		while (editor.cursor_offset < old_cursor_offset + MAX_BYTES_PER_LINE && editor.cursor_offset + 1 < editor.max_offset)
			editor.cursor_offset++;
		
		if (kb_Data[2] & kb_Alpha)
			editor.cursor_offset += (MAX_LINES_ONSCREEN - 1) * MAX_BYTES_PER_LINE;
	};
	
	if (key == KEY_UP) {
		old_cursor_offset = editor.cursor_offset;
		while (editor.cursor_offset > old_cursor_offset - MAX_BYTES_PER_LINE && editor.cursor_offset > editor.min_offset)
			editor.cursor_offset--;
		
		if (kb_Data[2] & kb_Alpha) {
			i = (MAX_LINES_ONSCREEN - 1) * MAX_BYTES_PER_LINE;
			while (editor.cursor_offset > cursor.min && i-- > 0)
				editor.cursor_offset--;
		};
	};
	
	/* If one of the cursor bounds is exceeded, set the cursor_offset to the exceeded bound */
	if (editor.cursor_offset > cursor.max)
		editor.cursor_offset = cursor.max;
	if (editor.cursor_offset < cursor.min)
		editor.cursor_offset = cursor.min;
	
	dbg_sprintf(dbgout, "cursor_offset = 0x%6x\n", editor.cursor_offset);
	
	/* Move the window offset if necessary */
	while (editor.cursor_offset < editor.edit_offset)
		editor.edit_offset -= MAX_BYTES_PER_LINE;
	
	while (editor.cursor_offset - editor.edit_offset >= MAX_LINES_ONSCREEN * MAX_BYTES_PER_LINE)
		editor.edit_offset += MAX_BYTES_PER_LINE;
	
	cursor.x = (editor.cursor_offset - editor.edit_offset) % 8;
	cursor.y = (editor.cursor_offset - editor.edit_offset) / 8;
	
	return;
}

static uint8_t save_changes_prompt(void) {
	
	uint8_t selector_y, i;
	const char *options[3] = {"Yes", "No", "Cancel"};
	
	gfx_SetColor(WHITE);
	gfx_FillRectangle_NoClip(92, 88, 135, 63);
	selector_y = 0;
	
	for (;;) {
		
		draw_window("Save changes?", true, 100, 92, 120, 52);
		
		for (i = 0; i < 3; i++){
			gfx_SetTextFGColor(BLACK);
			if (i == selector_y) {
				gfx_SetColor(BLACK);
				gfx_FillRectangle_NoClip(102, 109 + 11 * selector_y, 116, 11);
				gfx_SetTextFGColor(WHITE);
			};
			gfx_PrintStringXY(options[i], 160 - gfx_GetStringWidth(options[i]) / 2, 111 + 11 * i);
		};
		gfx_BlitRectangle(1, 92, 88, 135, 63);
		
		delay(200);
		
		do {
			kb_Scan();
		} while (!kb_AnyKey());
		
		if ((kb_Data[7] & kb_Down) && selector_y < 2)
			selector_y++;
		if ((kb_Data[7] & kb_Up) && selector_y > 0)
			selector_y--;
		
		if (kb_Data[1] & kb_2nd)
			break;
	};
	
	return selector_y;
}

uint8_t run_editor(void) {
	
	uint8_t sel_nibble;
	uint8_t key;
	uint8_t num_bytes;
	uint8_t save_code;
	uint24_t offset;
	bool redraw_bars = true;
	bool file_changed = false;
	bool multi_byte_selection = false;
	
	sel_nibble = 1;
	key = 255;	// 255 is a non-action keycode
	
	editor.cursor_offset = editor.edit_offset;
	editor.min_offset = editor.edit_offset;
	editor.max_offset = get_max_ram_address();
	
	if (editor.edit_type == FILE_EDIT_TYPE)
		editor.max_offset = editor.min_offset + ti_GetSize(editor.file);
	
	editor.undo_activated = false;
	
	cursor.x = 0;
	cursor.y = 0;
	
	for (;;) {
		
		if (multi_byte_selection) {
			/* Terminate multi-byte selection if the cursor is forced behind the first
			   selected byte */
			if ((editor.num_selected_bytes == 1 && key == KEY_LEFT) || (editor.num_selected_bytes < 9 && key == KEY_UP)) {
				multi_byte_selection = false;
				editor.num_selected_bytes = 1;
				redraw_bars = true;
				goto DRAW_BACKGROUND;
			};
			editor.num_selected_bytes = editor.cursor_offset - editor.sel_byte_string_offset + 1;
			draw_alt_tool_bar();
		} else {
			editor.sel_byte_string_offset = editor.cursor_offset;
			editor.num_selected_bytes = 1;
		};
		
		DRAW_BACKGROUND:
		gfx_SetColor(LT_GRAY);
		gfx_FillRectangle_NoClip(0, 20, LCD_WIDTH, 200);
		
		update_windows(sel_nibble);
		
		if (redraw_bars) {
			draw_top_bar(file_changed);
			draw_tool_bar();
			redraw_bars = false;
		};
		
		/* Selective delay to slow down cursor and selection response times for small files */
		if (ti_GetSize(editor.file) < 120)
			delay(100);
		
		key = get_keypress();
		
		easter_egg_one();
		
		if (key == KEY_2ND) {
			if (multi_byte_selection) {
				multi_byte_selection = false;
				editor.cursor_offset = editor.sel_byte_string_offset;
				redraw_bars = true;
			} else {
				multi_byte_selection = true;
			};
		};
		
		if (!multi_byte_selection) {
			if (key == KEY_YEQU) {
				create_goto_undo_action();
				offset = input("Goto:", 6, editor.edit_type);
				if (key != KEY_CLEAR) {
					editor_goto(offset);
					editor.undo_activated = true;
				};
				   
				// Debugging
				dbg_sprintf(dbgout, "[editor.c] [run_editor()] : min_offset = 0x%6x | cursor_offset = 0x%6x\n", editor.min_offset, editor.cursor_offset);
				redraw_bars = true;
			};
			
			if (key == KEY_WINDOW && editor.edit_type == FILE_EDIT_TYPE) {
				num_bytes = input("Insert bytes:", 2, false);
				if (key != KEY_CLEAR && num_bytes > 0) {
					create_file_insert_bytes_undo_action(num_bytes);
					file_insert_bytes(editor.cursor_offset - editor.min_offset, num_bytes);
					file_changed = true;
					editor.undo_activated = true;
				};
				redraw_bars = true;
			};
			
			if (key == KEY_TRACE && editor.undo_activated) {
				undo_action();
				redraw_bars = true;
				file_changed = true;
				editor.undo_activated = false;
			};
			
			if (key < 16) {
				file_changed = true;
				write_nibble(key, sel_nibble);
				if (sel_nibble-- == 0)	// Auto cursor advancing
					key = KEY_RIGHT;
				redraw_bars = true;
				editor.undo_activated = true;
			};
		};
		
		if (key == KEY_ZOOM && editor.edit_type == FILE_EDIT_TYPE && multi_byte_selection) {
			/* Block byte deletion if there are no bytes in the file */
			if (ti_GetSize(editor.file) > 0) {
				create_file_delete_bytes_undo_action();
				file_delete_bytes(editor.sel_byte_string_offset - editor.min_offset, editor.num_selected_bytes);
				file_changed = true;
				editor.undo_activated = true;
			};
			redraw_bars = true;
			multi_byte_selection = false;
		};
		
		if (key == KEY_CLEAR || key == KEY_GRAPH) {
			save_code = 1;
			if (editor.edit_type == FILE_EDIT_TYPE && file_changed)	// Changes to RAM cannot be undone
				save_code = save_changes_prompt();
			if (save_code < 2)
				goto EXIT_EDITOR;
		};
		
		if (key >= KEY_LEFT && key <= KEY_DOWN) {
			
			cursor.min = editor.min_offset;
			cursor.max = editor.max_offset - 1;
			
			if (multi_byte_selection) {
				cursor.min = editor.sel_byte_string_offset;
				if (editor.sel_byte_string_offset + MAX_NUM_SEL_BYTES < editor.max_offset)
					cursor.max = editor.sel_byte_string_offset + MAX_NUM_SEL_BYTES - 1;
			};
			
			if (!(key == KEY_LEFT && sel_nibble == 0))
				move_cursor(key);
			
			sel_nibble = 1;	// Reset the sel_nibble
		};
		
	};
	
	EXIT_EDITOR:
	delay(200);		// Prevent key fall-through
	return save_code;
}

void edit_file(char *file_name, uint8_t editor_file_type) {
	// Open file (ec)
	
	ti_var_t file;
	uint8_t ti_file_type;
	uint8_t save_code;
	
	ti_file_type = find_ti_file_type(editor_file_type);
	
	draw_message_dialog("Copying file...");
	
	if (!copy_file(file_name, ti_file_type))
		return;
	ti_CloseAll();
	file = ti_Open("HXAEDTMP", "r+");

	// Debugging
	dbg_sprintf(dbgout, "Opening file \'%s\' with code \'%d\'\n", file_name, ti_file_type);
	
	if (!file)
		return;
	
	editor.file = file;
	editor.file_name = file_name;
	editor.edit_type = FILE_EDIT_TYPE;
	ti_Rewind(editor.file);
	editor.edit_offset = ti_GetDataPtr(editor.file);
	dbg_sprintf(dbgout, "START OF FILE = 0x%6x\n", editor.edit_offset);
	editor.Draw_Left_Window = &draw_offset_window;
	
	// Open the editor
	save_code = run_editor();
	
	ti_CloseAll();
	if (save_code == 0) {
		draw_message_dialog("Saving file...");
		save_file(file_name, ti_file_type);
	};
	ti_Delete("HXAEDTMP");
	return;
}

void edit_ram(void) {
	
	editor.edit_type = RAM_EDIT_TYPE;
	editor.edit_offset = get_min_ram_address();
	
	// Debugging
	dbg_sprintf(dbgout, "[editor.c] [edit_ram()] : edit_offset = 0x%6x\n", editor.edit_offset);
	
	editor.Draw_Left_Window = &draw_address_window;
	
	run_editor();
	return;
}