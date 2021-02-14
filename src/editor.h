#ifndef EDITOR_H
#define EDITOR_H

#include <fileioc.h>

#include <stdint.h>

#define EDIT_FILE		"HEXATMP"
#define UNDO_APPVAR		"HEXAUNDO"
#define HS_CONFIG_APPVAR	"HEXAHSCA"
#define HEXAEDIT_CONFIG_APPVAR	"HEXACONF"

/* The maximum length of any file name including hidden files and null terminator. */
#define FILE_NAME_LEN		10

/*
 * The editor undo function uses these codes to determine how to process data in
 * the undo appvar.
*/
#define UNDO_INSERT_BYTES	0
#define UNDO_DELETE_BYTES	1
#define UNDO_WRITE_NIBBLE	2

/* The three kinds of "editors" */
#define FILE_EDITOR 0
#define RAM_EDITOR  1
#define ROM_VIEWER  2

/* Editor indexing formats */
#define OFFSET_INDEXING  0
#define ADDRESS_INDEXING 1

/* For editact_FindPhraseOccurances */
#define MAX_NUM_PHRASE_OCCURANCES 255
#define QUICK_SEARCH              ((uint24_t)500000)
#define MODERATE_SEARCH           ((uint24_t)1000000)
#define THOROUGH_SEARCH           ((uint24_t)5000000)

#define ROM_MIN_ADDRESS ((uint8_t *)0x000000)
#define ROM_MAX_ADDRESS ((uint8_t *)0x3fffff)
#define RAM_MIN_ADDRESS ((uint8_t *)0xd00000)
#define RAM_MAX_ADDRESS ((uint8_t *)0xd65800)

/* Constants for the editor UI */
#define ROWS_ONSCREEN   17
#define COLS_ONSCREEN   8
#define ROW_HEIGHT      11
#define HEX_COL_WIDTH   20
#define ASCII_COL_WIDTH 10

#define FONT_HEIGHT  7
#define CURSOR_COLOR BLUE

/* These two keymaps are only used in editor.c
 * The first one maps hex values to the corresponding keys on the calculator keypad.
 * The second one maps ASCII representations of hex characters to the corresponding keys
 * on the calculator keypad.
*/

#define HEX_VAL_KEYMAP "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\x06\x09\0\0\0\0\0" \
                       "\x02\x05\x08\0\x0F\x0C\0\x00\x01\x04\x07\0\x0E\x0B\0\0\0\0" \
                       "\0\0\x0D\x0A\0\0\0\0\0\0\0\0"

#define ASCII_HEX_KEYMAP "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x33\x36\x39\0\0\0\0\0" \
                         "\x32\x35\x38\0fc\0\x30\x31\x34\x37\0eb\0\0\0\0" \
                         "\0\0da\0\0\0\0\0\0\0\0"

/* Key codes for move_cursor */
#define CURSOR_DOWN  0
#define CURSOR_LEFT  1
#define CURSOR_RIGHT 2
#define CURSOR_UP    3

/* The maximum length for the editor name (the name that appears in the top bar) */
#define EDITOR_NAME_LEN 15

typedef struct
{
	char name[EDITOR_NAME_LEN];
	uint8_t type;
	uint8_t *min_address;
	uint8_t *max_address;
	uint8_t	*window_address;
	uint24_t num_changes;
	
	uint8_t file_type;	// Only used for files
	bool is_file_empty;
} editor_t;

typedef struct
{
	uint8_t *primary;	// This always points to the current byte selection
	
	/* 
	 * This points to the first byte in a multi-byte selection. Otherwise, it is
	 * the same as primary.
	*/
	uint8_t *secondary;
	bool high_nibble;
	bool multibyte_selection;
} cursor_t;

/* These structures are for the Headless Start. */
/*
typedef struct {
	char headless_start_flag[3];
	uint8_t editor_config;
} header_config_t;

typedef struct {
	uint8_t *window_address;
	uint8_t *cursor_primary;
	uint8_t *cursor_secondary;
} mem_editor_config_t;

typedef struct {
	char file_name[10];
	uint8_t file_type;
	uint24_t window_offset;
	uint24_t cursor_primary_offset;
	uint24_t cursor_secondary_offset;
} file_editor_config_t;
*/
bool editor_FileEditor(const char *name, uint8_t type, uint24_t primary_cursor_offset, uint24_t secondary_cursor_offset);
void editor_RAMEditor(uint24_t primary_cursor_offset, uint24_t secondary_cursor_offset);
void editor_ROMViewer(uint24_t primary_cursor_offset, uint24_t secondary_cursor_offset);
void editor_HeadlessStart(void);

#endif
