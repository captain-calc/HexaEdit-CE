#ifndef DEFINES_H
#define DEFINES_H

#define MAX_LINES_ONSCREEN	15
#define MAX_BYTES_PER_LINE	8


#define DK_GRAY	0x6B
#define LT_GRAY	0xB5
#define BLACK	0x00
#define WHITE	0xFF
#define GREEN	0x06
#define BLUE	0x1C
#define RED	0xE0

/* Editor file types
   These are different from the TI-OS file type codes */
#define ASM_PRGM_FILE	1
#define TI_PRGM_FILE	2
#define APPVAR_FILE		3

// Editor edit types
#define FILE_EDIT_TYPE	0
#define RAM_EDIT_TYPE	1
#define ROM_EDIT_TYPE	2

#define UNDO_APPVAR_NAME	"HXAEDUND"

#define KEY_LEFT	16
#define KEY_RIGHT	17
#define KEY_UP	18
#define KEY_DOWN	19
#define KEY_CLEAR	20
#define KEY_YEQU	21
#define KEY_WINDOW	22
#define KEY_ZOOM	23
#define KEY_TRACE	24
#define KEY_GRAPH	25
#define KEY_2ND	26
#define KEY_DEL	27

#define MAX_NUM_SEL_BYTES 99

#endif