#include "defines.h"
#include "editor.h"
#include "easter_eggs.h"
#include "routines.h"

#include <graphx.h>
#include <keypadc.h>
#include <tice.h>
#include <debug.h>

/* This draws a 16x16 icon using the data found at the current cursor offset */
void easter_egg_one(bool file_changed, uint8_t sel_nibble) {
	
	uint8_t *sprite_data;
	uint8_t sprite_width, sprite_height;
	uint24_t sprite_size;
	uint24_t xPos = 0;
	uint8_t yPos = 0;
	uint8_t temp_yPos;
	uint8_t scale = 2;
	uint8_t i, j, k;

	if (!(kb_Data[4] & kb_Stat) || (editor.max_offset - editor.cursor_offset < 2))
		return;
		
	// Get the sprite's size
	sprite_width = *editor.cursor_offset;
	sprite_height = *(editor.cursor_offset + 1);
	sprite_size = sprite_width * sprite_height;
	
	if (sprite_size == 0 || sprite_height > 245)
		return;
	
	if ((uint24_t)(editor.max_offset - editor.cursor_offset) < sprite_size)
		return;
	
	sprite_data = malloc(sprite_size);
	if (sprite_data == NULL)
		return;
	
	// Set the scale to one if the sprite is very large
	if (sprite_width > 155 || sprite_height > 115)
		scale = 1;
	
	// Read out and store the sprite data
	copy_data(editor.cursor_offset + 2, sprite_data, sprite_size, 1);
	
	// Draw rectangle border
	gfx_SetColor(WHITE);
	gfx_FillRectangle_NoClip(xPos, yPos, sprite_width * scale + 10, sprite_height * scale + 10);
	gfx_SetColor(BLACK);
	gfx_FillRectangle_NoClip(xPos + 2, yPos + 2, sprite_width * scale + 6, sprite_height * scale + 6);
	gfx_SetColor(WHITE);
	gfx_FillRectangle_NoClip(xPos + 3, yPos + 3, sprite_width * scale + 4, sprite_height * scale + 4);

	temp_yPos = yPos + 5;
	for (i = 0; i < sprite_height; i++) {
		for (j = 0; j < scale; j++) {
			for (k = 0; k < sprite_width; k++) {
				gfx_SetColor(*(sprite_data + (sprite_width * i + k)));
				gfx_SetPixel(xPos + 5 + scale * k, temp_yPos);
				if (scale > 1)
					gfx_SetPixel(xPos + 6 + scale * k, temp_yPos);
			};
			temp_yPos++;
		};
	};

	gfx_BlitRectangle(1, xPos, yPos, sprite_width * scale + 10, sprite_height * scale + 10);
	free(sprite_data);
	
	delay(200);
	while (!os_GetCSC());
	
	// Redraw everything
	gfx_FillScreen(LT_GRAY);
	update_windows(sel_nibble);
	draw_top_bar(file_changed);
	draw_tool_bar();
	
	return;
}

/* A ROM viewer */
bool easter_egg_two(void) {
	
	uint8_t sel_nibble;
	uint8_t key;
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
	
	if (!(kb_Data[6] & kb_Add))
		return true;
		
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
		
		easter_egg_one(file_changed, sel_nibble);
		
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

	EXIT_VIEWER:
	delay(200);
	return true;
}

