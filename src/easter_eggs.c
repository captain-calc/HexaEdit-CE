#include "defines.h"
#include "editor.h"
#include "easter_eggs.h"
#include "routines.h"

#include <graphx.h>
#include <keypadc.h>
#include <tice.h>
#include <debug.h>

/* This draws a 16x16 icon using the data found at the current cursor offset */
void easter_egg_one(void) {
	
	uint8_t icon_data[256];
	uint24_t xPos = 200;
	uint8_t yPos = 50;
	uint8_t i, j, k;

	if ((editor.max_offset - editor.cursor_offset > 256) && (kb_Data[4] & kb_Stat)) {
		
		// Read out and store the 256 bytes
		copy_data(editor.cursor_offset, icon_data, 256, 1);
		
		// Draw rectangle border
		gfx_SetColor(WHITE);
		gfx_FillRectangle_NoClip(xPos, yPos, 42, 42);
		gfx_SetColor(BLACK);
		gfx_FillRectangle_NoClip(xPos + 2, yPos + 2, 38, 38);
		gfx_SetColor(WHITE);
		gfx_FillRectangle_NoClip(xPos + 3, yPos + 3, 36, 36);

		for (i = 0; i < 16; i++) {
			for (j = 0; j < 2; j++) {
				for (k = 0; k < 16; k++) {
					gfx_SetColor(icon_data[16 * i + k]);
					gfx_SetPixel(xPos + 5 + 2 * k, yPos + 5);
					gfx_SetPixel(xPos + 6 + 2 * k, yPos + 5);
				};
				yPos++;
			};
		};

		gfx_BlitRectangle(1, xPos, 50, 42, 42);
		delay(200);
		while (!os_GetCSC());
		return;
	};
}

/* A ROM viewer */
bool easter_egg_two(void) {
	
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
	
	editor.edit_type = ROM_EDIT_TYPE;
	editor.edit_offset = (uint8_t *)0x000000;
	editor.Draw_Left_Window = &draw_address_window;
	
	editor.cursor_offset = editor.edit_offset;
	editor.min_offset = editor.edit_offset;
	editor.max_offset = (uint8_t *)0x400000;
	
	editor.undo_activated = false;
	
	cursor.x = 0;
	cursor.y = 0;
	
	if (kb_Data[6] & kb_Add) {
		
		dbg_sprintf(dbgout, "Running ROM viewer\n");
		
		for (;;) {
		
			if (multi_byte_selection) {
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
					redraw_bars = true;
				};
			};
			
			if (key == KEY_CLEAR || key == KEY_GRAPH) {
				save_code = 1;
				goto EXIT_VIEWER;
			};
			
			if (key >= KEY_LEFT && key <= KEY_DOWN) {
				sel_nibble = 1;	// Reset the sel_nibble
				
				cursor.min = editor.min_offset;
				cursor.max = editor.max_offset - 1;
				dbg_sprintf(dbgout, "cursor.min = 0x%6x\n", cursor.min);
				
				if (multi_byte_selection) {
					cursor.min = editor.sel_byte_string_offset;
					if (editor.sel_byte_string_offset + MAX_NUM_SEL_BYTES < editor.max_offset)
						cursor.max = editor.sel_byte_string_offset + MAX_NUM_SEL_BYTES;
				};
				
				move_cursor(key);
			};
			
			
		};
	};
	
	EXIT_VIEWER:
	delay(200);
	return true;
}

