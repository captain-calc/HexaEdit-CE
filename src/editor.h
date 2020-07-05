#ifndef EDITOR_H
#define EDITOR_H

#include <fileioc.h>
#include <stdbool.h>


typedef struct {
	uint8_t edit_type;
	ti_var_t file;
	char *file_name;
	uint8_t *min_offset, *max_offset;
	uint8_t *edit_offset;
	uint8_t *cursor_offset;
	uint8_t cursor_x, cursor_y;
	uint8_t *sel_byte_string_offset;
	uint8_t num_selected_bytes;
	bool undo_activated;
	void (*Draw_Left_Window)(void);
} editor_t;

extern editor_t editor;

typedef struct {
	uint8_t x, y;
	uint8_t *min, *max;
} cursor_t;

extern cursor_t cursor;

// GUI routines
void draw_file_size(void);
void draw_top_bar(bool file_changed);
uint24_t input(const char *prompt, uint8_t char_limit, bool hex_flag);
void draw_alt_tool_bar(void);
void draw_tool_bar(void);
void draw_address_window(void);
static void draw_offset_window(void);
char *hex(uint8_t byte);
void draw_byte_selector(uint8_t sel_nibble, uint8_t pixel_x, uint8_t pixel_y);
void draw_printable_bytes(uint8_t byte, bool highlight, uint24_t print_x, uint8_t print_y);
void draw_right_two_windows(uint8_t sel_nibble);
void update_windows(uint8_t sel_nibble);
void print_alt_byte_values(void);

// Actual editor
uint8_t get_keypress(void);
void move_cursor(uint8_t key);
static uint8_t save_changes_prompt(void);
static uint8_t run_editor(void);

void edit_file(char *file_name, uint8_t editor_file_type);
void edit_ram(void);

#endif