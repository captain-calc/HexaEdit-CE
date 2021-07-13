#ifndef EDITOR_H
#define EDITOR_H

#include <fileioc.h>

#include <stdint.h>

/* HEADLESS START CONFIGURATION
 *
 *    The Headless Start feature uses the following data configuration in the
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
 *     Only include the data sections you will need. For example, if you wanted
 * to start a file editor and override the default color scheme, you would
 * include the following sections:
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
 *    0000 0000
 *    ^      ^
 *    |      |
 *    |      * The two least significant bytes specify the editor type (File = 0,
 *    |        RAM = 1, ROM = 2)
 *    |
 *    * The most significant byte should be set to specify a color override. It
 *      should be set to 0 if you do not want to change the color scheme.
 *
 *     If you want to override the color scheme and open a file editor, for example,
 * the byte would look like: 1000 0010.
 *
 *
 * You may notice that the values for the window and cursor pointers for the file editor are OFFSETS
 * instead of memory pointers. This is because HexaEdit does not edit the specified file directly but,
 * rather, a copy of it. HexaEdit does not create this copy until after it reads out the configuration
 * data and creates the necessary memory pointers out of the file offsets.
*/


/*
 * Ans flag: If the variable Ans starts with this byte sequence, then HexaEdit
 * will consider it as a Headless Start configuration.
 */
#define HS_CONFIG_ANS_FLAG      "HEXAEDIT\x00\x0b"
#define HS_CONFIG_ANS_FLAG_LEN  10
#define COLOR_OVERRIDE_FLAG     ((uint8_t)(1 << 7))


/* Common program files */
#define EDIT_FILE             "HEXATMP"
#define UNDO_APPVAR           "HEXAUNDO"
/* The configuration appvar for the Headless Start */
#define HS_CONFIG_APPVAR      "HEXAHSCA"
#define HEXA_SETTINGS_APPVAR  "HEXACONF"

/*
 * The maximum length of any file name, including hidden files and null
 * terminator.
*/
#define FILE_NAME_LEN 10

/*
 * The editor undo function uses these codes to determine how to process data in
 * the undo appvar.
*/
#define UNDO_INSERT_BYTES 0
#define UNDO_DELETE_BYTES 1
#define UNDO_WRITE_NIBBLE 2

/* The three kinds of "editors" */
#define FILE_EDITOR 0
#define RAM_EDITOR  1
#define ROM_VIEWER  2

/* Editor indexing formats */
#define OFFSET_INDEXING  0
#define ADDRESS_INDEXING 1

/* Return values of can_write_nibble() (see editor.c) */
#define WRITE_NIBBLE_MOVE_CURSOR       0
#define NO_WRITE_NIBBLE_MOVE_CURSOR    1
#define NO_WRITE_NIBBLE_NO_MOVE_CURSOR 2

#define ROM_MIN_ADDRESS   ((uint8_t *)0x000000)
#define ROM_MAX_ADDRESS   ((uint8_t *)0x3fffff)
#define RAM_MIN_ADDRESS   ((uint8_t *)0xd00000)
#define RAM_MAX_ADDRESS   ((uint8_t *)0xd65800)
#define PORTS_MIN_ADDRESS ((uint8_t *)0xe00000)
#define PORTS_MAX_ADDRESS ((uint8_t *)0xffffff)

/* Restricted write RAM addresses */
#define RAM_READONLY_ADDRESS_ONE ((uint8_t *) 0xd1887c)
#define RAM_READONLY_ADDRESS_TWO ((uint8_t *) 0xd19881)

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

#define JUMP_LEN_ACCEL_LEFT_RIGHT ((uint24_t)0x010000)
#define JUMP_LEN_ACCEL_UP_DOWN    ((uint24_t)((ROWS_ONSCREEN - 1) * COLS_ONSCREEN))

/* The maximum length for the editor name (the name that appears in the top bar)
 * This should be length of the file name or editor name + 2 for the "* " that
 * appears before the name when the user makes a modification to the memory.
*/
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
  bool su_mode;       // Superuser mode; allows user to write to RAM_READONLY_ADDRESSes
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

bool editor_FileEditor(const char *name, uint8_t type, uint24_t primary_cursor_offset, uint24_t secondary_cursor_offset);
void editor_RAMEditor(uint24_t primary_cursor_offset, uint24_t secondary_cursor_offset);
void editor_PortsEditor(uint24_t primary_cursor_offset, uint24_t secondary_cursor_offset);
void editor_MemEditor(
  const char *name,
  uint8_t *min_address,
  uint8_t *max_address,
  uint24_t primary_cursor_offset,
  uint24_t secondary_cursor_offset
);
void editor_ROMViewer(uint24_t primary_cursor_offset, uint24_t secondary_cursor_offset);
void editor_HeadlessStart(void);

#endif
