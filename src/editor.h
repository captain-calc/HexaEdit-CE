// Name:    Captain Calc
// Date:    July 14, 2021
// File:    editor.h
// Purpose: editor provides the functions for opening editors and the editor
//          control loop functions.


#ifndef EDITOR_H
#define EDITOR_H


#include <fileioc.h>

#include <stdint.h>


// HEADLESS START CONFIGURATION
//
//     The Headless Start feature uses the following data configuration in Ans
// (Note: Always include the general configuration data!):
//
//
// General Configuration (1 byte)
// ===============================
// Color Theme/Editor Type      1
//
//
// Color Theme Override (7 bytes)
// ===============================
// Background Color             1
// Bar Color                    1
// Bar Text Color               1
// Table BG Color               1
// Table Text Color             1
// Selected Table Text Color    1
// Cursor Color                 1
// ===============================
//
//
//
// TODO (July 14, 2021): These addresses should become offsets
//
// RAM Editor/ROM Viewer (6 bytes)
// ===============================
// Cursor Primary Address       3
// Cursor Secondary Address     3
// ===============================
//
//
// File Editor (17 bytes)
// ===============================
// File Name                    10
// File Type                    1
// Cursor Primary Offset        3
// Cursor Secondary Offset      3
// ===============================
//
//
//     Only include the data sections you will need. For example, if you wanted
// to start a file editor and override the default color scheme, you would
// include the following sections:
//
//
// ===========================
// General Configuration Data
// Color Theme Override
// File Editor
// ===========================
//
//
// Configuration Data Notes
// =================================
//
// The Color Theme/Editor Type byte looks like this:
//
//     0000 0000
//     ^      ^
//     |      |
//     |      * The two least significant bytes specify the editor type (File = 0,
//     |        RAM = 1, ROM = 2)
//     |
//     * The most significant byte should be set to specify a color override. It
//       should be set to 0 if you do not want to change the color scheme.
//
//     If you want to override the color scheme and open a file editor, for
// example, the byte would look like: 1000 0010.
//


// FILE DEFINES
// ============================================================================


// Ans flag: If the variable Ans starts with this byte sequence, then HexaEdit
// will consider it as a Headless Start configuration.
#define HS_CONFIG_ANS_FLAG      "HEXAEDIT\x00\x0b"
#define HS_CONFIG_ANS_FLAG_LEN  10
#define COLOR_OVERRIDE_FLAG     ((uint8_t)(1 << 7))


// Common program files
#define EDIT_FILE             "HEXATMP"
#define UNDO_APPVAR           "HEXAUNDO"
#define HEXA_SETTINGS_APPVAR  "HEXACONF"

// The maximum length of any file name, including hidden files and null
// terminator.
#define FILE_NAME_LEN 10

// The editor undo function uses these codes to determine how to process data
// in the undo appvar.
#define UNDO_INSERT_BYTES 0
#define UNDO_DELETE_BYTES 1
#define UNDO_WRITE_NIBBLE 2

// The four kinds of "editors"
#define FILE_EDITOR  0
#define RAM_EDITOR   1
#define ROM_VIEWER   2
#define PORTS_EDITOR 3

// Editor indexing formats
#define OFFSET_INDEXING  0
#define ADDRESS_INDEXING 1

// Return values of can_write_nibble() (see editor.c)
#define WRITE_NIBBLE_MOVE_CURSOR       0
#define NO_WRITE_NIBBLE_MOVE_CURSOR    1
#define NO_WRITE_NIBBLE_NO_MOVE_CURSOR 2

// Start/end address for ROM, RAM, and "Ports"
#define ROM_MIN_ADDRESS   ((uint8_t *)0x000000)
#define ROM_MAX_ADDRESS   ((uint8_t *)0x3fffff)
#define RAM_MIN_ADDRESS   ((uint8_t *)0xd00000)
#define RAM_MAX_ADDRESS   ((uint8_t *)0xd65800)
#define PORTS_MIN_ADDRESS ((uint8_t *)0xe00000)
#define PORTS_MAX_ADDRESS ((uint8_t *)0xffffff)

// Restricted write RAM addresses
#define RAM_READONLY_ADDRESS_ONE ((uint8_t *) 0xd1887c)
#define RAM_READONLY_ADDRESS_TWO ((uint8_t *) 0xd19881)

// Constants for the editor UI
#define ROWS_ONSCREEN   17
#define COLS_ONSCREEN   8
#define ROW_HEIGHT      11
#define HEX_COL_WIDTH   20
#define ASCII_COL_WIDTH 10

#define FONT_HEIGHT  7
#define CURSOR_COLOR BLUE

// These two keymaps are only used in editor.c
// The first one maps hex values to the corresponding keys on the calculator
// keypad. The second one maps ASCII representations of hex characters to the
// corresponding keys on the calculator keypad.
#define HEX_VAL_KEYMAP "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x03\x06\x09\0\0" \
                       "\0\0\0\x02\x05\x08\0\x0F\x0C\0\x00\x01\x04\x07\0\x0E" \
                       "\x0B\0\0\0\0\0\0\x0D\x0A\0\0\0\0\0\0\0\0"

#define ASCII_HEX_KEYMAP "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x33\x36\x39\0" \
                         "\0\0\0\0\x32\x35\x38\0fc\0\x30\x31\x34\x37\0eb\0\0" \
                         "\0\0\0\0da\0\0\0\0\0\0\0\0"

// Key codes for move_cursor
#define CURSOR_DOWN  0
#define CURSOR_LEFT  1
#define CURSOR_RIGHT 2
#define CURSOR_UP    3

// Cursor jump distances for alpha scrolling
#define JUMP_LEN_ACCEL_LEFT_RIGHT ((uint24_t)0x010000)
#define JUMP_LEN_ACCEL_UP_DOWN    ( \
  (uint24_t)((ROWS_ONSCREEN - 1) * COLS_ONSCREEN) \
)

// The maximum length for the editor name (the name that appears in the top
// bar). This should be length of the file name or editor name + 2 for the "* "
// that appears before the name when the user makes a modification to the
// memory.
#define EDITOR_NAME_LEN 15


// GLOBAL EDITOR/CURSOR DATA
// ============================================================================


typedef struct
{
	char name[EDITOR_NAME_LEN];
  
  // The type of editor/viewer (File/RAM/"Ports"/ROM)
	uint8_t type;
  
  // min_address points to the start of the memory being edited. When HexaEdit
  // edits files, it copies the file into EDIT_FILE, an appvar. When the
  // changes are saved, HexaEdit converts EDIT_FILE into whatever format the
  // original file was and gives it the orignal file's name. Unless it is
  // inserting or deleting bytes from a file, HexaEdit keeps the EDIT_FILE
  // closed and modifies it via its location in RAM. Every time the EDIT_FILE
  // is resized, its location in RAM changes, invalidating min_address. Thus,
  // each time EDIT_FILE is opened (and every time any other file is opened),
  // min_address should be reassigned. min_address should be updated via the
  // VAT entry for EDIT_FILE because this will be faster to parse than a
  // ti_Open()/ti_GetDataPtr() and because I am not sure if that ti_Open() will
  // shift the file location again. Every time EDIT_FILE is resized, mem_size
  // should be updated.
	uint8_t *min_address;
	uint24_t mem_size;
  
  // The offset relative to min_address of the first byte displayed in the
  // window
	uint24_t window_offset;
	uint24_t num_changes;
	
  // file_type is only used for files.
	uint8_t file_type;
	bool is_file_empty;
  
  // Superuser mode; allows user to write to RAM_READONLY_ADDRESSes
  bool su_mode;
} editor_t;


typedef struct
{
  // The cursor offset, updated every time the cursor moves.
	uint24_t primary;
	
	// The offset to the first byte in a multi-byte selection. Otherwise, it is
	// the same as primary.
	uint24_t secondary;
	bool high_nibble;
	bool multibyte_selection;
} cursor_t;


// PUBLIC FUNCTION DECLARATIONS
// ============================================================================


// Description: 
// Pre:         
// Post:        
bool editor_FileEditor(
  const char *name,
  uint8_t type,
  uint24_t primary_cursor_offset,
  uint24_t secondary_cursor_offset
);


// Description: 
// Pre:         
// Post:        
void editor_RAMEditor(
  uint24_t primary_cursor_offset, uint24_t secondary_cursor_offset
);


// Description: 
// Pre:         
// Post:        
void editor_PortsEditor(
  uint24_t primary_cursor_offset, uint24_t secondary_cursor_offset
);


// Description: 
// Pre:         
// Post:        
void editor_MemEditor(
  const char *name,
  uint8_t type,
  uint8_t *min_address,
  uint8_t *max_address,
  uint24_t primary_cursor_offset,
  uint24_t secondary_cursor_offset
);


// Description: 
// Pre:         
// Post:        
void editor_ROMViewer(
  uint24_t primary_cursor_offset, uint24_t secondary_cursor_offset
);


// Description: 
// Pre:         
// Post:        
void editor_HeadlessStart(void);

#endif
