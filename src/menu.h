#ifndef MENU_H
#define MENU_H

/**
 * This file contains all of the data for HexaEdit's main menu.
*/

#include <fileioc.h>

#include <stdbool.h>
#include <stdint.h>


/* General program data. */
#define PROGRAM_NAME		"HexaEdit"
#define PROGRAM_VERSION		"2.0.1"
#define RECENT_FILES_APPVAR	"HEXARCF"


/* File table data. */
#define NUM_FILES_ONSCREEN	15


#define HEXAEDIT_RECENTS_TYPE		0
#define HEXAEDIT_APPVAR_TYPE		1
#define HEXAEDIT_BASIC_PRGM_TYPE	2
#define HEXAEDIT_ASM_PRGM_TYPE		3

#define NUM_TABLES		4
#define RECENTS_TABLE_NUM	0
#define APPVAR_TABLE_NUM	1
#define ASM_PRGM_TABLE_NUM	2
#define BASIC_PRGM_TABLE_NUM	3


#define MAX_NUM_APPVARS		400
#define MAX_NUM_ASM_PROGRAMS	400
#define MAX_NUM_BASIC_PROGRAMS	400
#define MAX_NUM_RECENT_FILES	(15)

#define ASM_PROGRAM_FLAG	31727	// 61307 \xef\x7b
#define FILE_NAME_LEN		10

/* This structure handles multiple file types.
Although appvars cannot be "locked" and the unused boolean variable does
cause some wasted space, the convenience and efficiency of having only one
file data structure far outweighs this overhead. */
typedef struct {
	char name[FILE_NAME_LEN];
	uint8_t hexaedit_type;
	bool is_archived;
	bool is_protected;
	uint8_t *vat_ptr;
	uint8_t *data_ptr;
	uint16_t size;
} file_data_t;

typedef struct {
	file_data_t *appvars[MAX_NUM_APPVARS];
	file_data_t *asm_programs[MAX_NUM_ASM_PROGRAMS];
	file_data_t *basic_programs[MAX_NUM_BASIC_PROGRAMS];
	uint16_t num_appvars;
	uint16_t num_asm_programs;
	uint16_t num_basic_programs;
} file_list_t;

typedef struct {
	file_data_t *files[MAX_NUM_RECENT_FILES];
	uint8_t num_files;
} recent_file_list_t;

void menu_DrawTitleBar();
void main_menu(void);

#endif