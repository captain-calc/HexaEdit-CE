#include "defines.h"
#include "editor.h"
#include "gui.h"

#include <graphx.h>
#include <keypadc.h>
#include <fileioc.h>
#include <tice.h>
#include <debug.h>

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>

#define UNDO_GOTO	0
#define UNDO_WRITE_NIBBLE	1
#define UNDO_DELETE_BYTES	2
#define UNDO_INSERT_BYTES	3
#define UNDO_DEACTIVATED	255


void close_program(void) {
	
	if (!ti_Delete(UNDO_APPVAR_NAME)) {
		draw_message_dialog("Failed to delete undo appvar");
		while (!os_GetCSC());
	};
	
	gfx_End();
	ti_CloseAll();
	exit(0);
}

uint8_t *get_min_ram_address(void);

uint8_t *get_max_ram_address(void);

// Assembly function for fast data copying (see util.asm)
void copy_data(void *from, void *to, uint24_t amount, uint8_t copy_direction);

uint8_t find_ti_file_type(uint8_t editor_file_type) {
	
	switch(editor_file_type) {
		
		case ASM_PRGM_FILE:
			return TI_PPRGM_TYPE;
			
		case TI_PRGM_FILE:
			return TI_PRGM_TYPE;
			
		case APPVAR_FILE:
			return TI_APPVAR_TYPE;
		
		default:
			return 0;
	};
}

bool copy_file(char *file_name, uint8_t ti_file_type) {

	ti_var_t file, temp_file;
	uint24_t file_size;
	uint8_t byte;
	
	ti_CloseAll();
	if ((file = ti_OpenVar(file_name, "r", ti_file_type)) == NULL) {
		// Debugging
		dbg_sprintf(dbgout, "Failed to open \'%s\' with TI code: \'%d\'\n", file_name, ti_file_type);
		goto ERROR;
	};
	
	file_size = ti_GetSize(file);
	
	if ((temp_file = ti_Open("HXAEDTMP", "w")) == NULL) {
		// Debugging
		dbg_sprintf(dbgout, "Failed to open \'HXAEDTMP\'\n");
		goto ERROR;
	};
	
	/* If the file size is zero, ti_Resize() will fail */
	if (file_size > 0) {
		if (ti_Resize(file_size, temp_file) <= 0) {
			// Debugging
			dbg_sprintf(dbgout, "Failed to resize \'%s\'\n", file_name);
			goto ERROR;
		};
		
		copy_data(ti_GetDataPtr(file), ti_GetDataPtr(temp_file), file_size, 1);
	};
	
	// Debugging
	dbg_sprintf(dbgout, "Copied file\n");

	ti_CloseAll();
	return true;
	
	ERROR:
	ti_CloseAll();
	draw_message_dialog("Failed to copy file");
	while (!os_GetCSC());
	return false;
}

bool save_file(char *file_name, uint8_t ti_file_type) {

	ti_var_t original_file, new_file, temp_file;
	uint24_t temp_file_size;
	bool is_archived;

	// Open the original file to determine its archive status
	ti_CloseAll();

	if ((original_file = ti_OpenVar(file_name, "r", ti_file_type)) == NULL)
		goto ERROR_FILE;

	is_archived = ti_IsArchived(original_file);

	ti_CloseAll();

	/* Open the editor's primary temporary file and another file that will become
	   the new changed file */
	if ((temp_file = ti_Open("HXAEDTMP", "r")) == NULL)
		goto ERROR_FILE;
	
	temp_file_size = ti_GetSize(temp_file);

	if ((new_file = ti_OpenVar("HXAEDTP2", "w", ti_file_type)) == NULL)
		goto ERROR_FILE;

	// Make the new file as big as the temporary file
	if (temp_file_size > 0) {
		if (ti_Resize(temp_file_size, new_file) <= 0)
			goto ERROR_FILE;
		
		// Debugging
		dbg_sprintf(dbgout, "Resized new file\n");

		// Copy the contents of the temporary file into the new file
		copy_data(ti_GetDataPtr(temp_file), ti_GetDataPtr(new_file), temp_file_size, 1);
	};

	ti_CloseAll();

	// Delete the temporary file. If this action fails, delete the new file
	if (!ti_Delete("HXAEDTMP")) {
		ti_DeleteVar("HXAEDTP2", ti_file_type);
		goto ERROR_FILE;
	};

	// Delete the original file
	if (!ti_DeleteVar(file_name, ti_file_type))
		goto ERROR_FILE;

	// If the original file was archived, archive the new file
	if ((new_file = ti_OpenVar("HXAEDTP2", "r", ti_file_type)) == NULL)
		goto ERROR_FILE;

	if (is_archived)
		if (!ti_SetArchiveStatus(1, new_file))
			goto ERROR_MEM;

	ti_CloseAll();

	// Finally, rename the new file as the original file
	if (ti_RenameVar("HXAEDTP2", file_name, ti_file_type) != 0)
		goto ERROR_FILE;

	return true;

	ERROR_FILE:
	ti_CloseAll();
	draw_message_dialog("Failed to save file");
	while (!os_GetCSC());
	return false;

	ERROR_MEM:
	ti_CloseAll();
	draw_message_dialog("Insufficient ROM to save file");
	while (!os_GetCSC());
	return false;
}

void create_goto_undo_action(void) {
	
	ti_var_t undo_appvar;
	uint8_t undo_code = UNDO_GOTO;
	
	if ((undo_appvar = ti_Open(UNDO_APPVAR_NAME, "r+")) == NULL) {
		draw_message_dialog("Failed to create undo action");
		while (!os_GetCSC());
		return;
	};
	ti_Write(&undo_code, 1, 1, undo_appvar);
	ti_Write(&editor.cursor_offset, 3, 1, undo_appvar);
	ti_Close(undo_appvar);
	
	return;
}

void editor_goto(uint24_t offset) {
	
	uint8_t *new_offset;
	
	new_offset = editor.min_offset;
	
	if (editor.edit_type == FILE_EDIT_TYPE) {
		new_offset += offset;
	} else {
		new_offset = (uint8_t *)offset;
	};
	
	// Debugging
	dbg_sprintf(dbgout, "[routines.c] [Setting new offset] : max_offset = 0x%6x\n", editor.max_offset);
	
	if (new_offset >= editor.min_offset && new_offset < editor.max_offset) {
		
		editor.cursor_offset = new_offset;
		
		if (editor.cursor_offset > editor.edit_offset + (MAX_LINES_ONSCREEN - 1) * MAX_BYTES_PER_LINE) {
			// Truncate quotient and multiply by 8 to get a multiple of 8
			editor.edit_offset = (uint8_t *)(((uint24_t)editor.cursor_offset / 8) * 8);
			
			/* Decrement the window offset (edit_offset) as far as possible to show the desired offset's context */
			while (editor.edit_offset > editor.min_offset && (editor.cursor_offset - editor.edit_offset) < (MAX_LINES_ONSCREEN - 1) * MAX_BYTES_PER_LINE)
				editor.edit_offset -= MAX_BYTES_PER_LINE;
		};

		if (editor.cursor_offset < editor.edit_offset)
			editor.edit_offset = editor.min_offset + (((editor.cursor_offset - editor.min_offset) / 8) * 8);
		
		// Debugging
		dbg_sprintf(dbgout, "[routines.c] [editor_goto()] : edit_offset = 0x%6x | cursor_offset = 0x%6x\n", editor.edit_offset, editor.cursor_offset);
	};
	
	return;
}

void write_nibble(uint8_t nibble_byte, uint8_t sel_nibble) {
	
	ti_var_t undo_appvar;
	uint8_t undo_code = UNDO_WRITE_NIBBLE;
	uint8_t i;
	
	// Store byte to undo appvar if the more significant nibble is selected
	if (sel_nibble == 1) {
		if ((undo_appvar = ti_Open(UNDO_APPVAR_NAME, "r+")) == NULL) {
			draw_message_dialog("Failed to create undo action");
			while (!os_GetCSC());
			return;
		};
		ti_Write(&undo_code, 1, 1, undo_appvar);
		ti_Write(&editor.cursor_offset, 3, 1, undo_appvar);
		ti_Write(editor.cursor_offset, 1, 1, undo_appvar);
		ti_Close(undo_appvar);
	};
	
	if (sel_nibble) nibble_byte *= 16;

	for (i = 0; i < 4; i++) {
		*editor.cursor_offset &= ~(1 << i + (4 * sel_nibble));
	};

	*editor.cursor_offset |= nibble_byte;
	
	return;
}

void create_file_insert_bytes_undo_action(uint8_t num_bytes) {
	
	ti_var_t undo_appvar;
	uint8_t undo_code = UNDO_INSERT_BYTES;
	
	if ((undo_appvar = ti_Open(UNDO_APPVAR_NAME, "r+")) == NULL) {
		draw_message_dialog("Failed to create undo action");
		while (!os_GetCSC());
		return;
	};
	ti_Write(&undo_code, 1, 1, undo_appvar);
	ti_Write(&editor.sel_byte_string_offset, 3, 1, undo_appvar);
	ti_Write(&num_bytes, 1, 1, undo_appvar);
	ti_Close(undo_appvar);
	
	return;
}

/* offset is the number of bytes from the start of the file */
bool file_insert_bytes(uint24_t offset, uint8_t num_bytes) {
	
	uint8_t i;
	uint24_t num_bytes_shift = offset;
	
	// Debugging
	dbg_sprintf(dbgout, "[routines.c] [file_insert_bytes()]: offset = %d | num_bytes = %d\n", offset, num_bytes);
	
	if (!ti_Resize(ti_GetSize(editor.file) + num_bytes, editor.file))
		goto ERROR;
	
	if (ti_Rewind(editor.file) == 'EOF')
		goto ERROR;
	
	editor.cursor_offset = editor.min_offset + num_bytes_shift;
	editor.max_offset = editor.min_offset + ti_GetSize(editor.file);
	
	// Debugging
	dbg_sprintf(dbgout, "[routines.c] [file_insert_bytes()]: from = 0x%6x | to = 0x%6x\n", editor.min_offset + num_bytes, editor.min_offset);
	if (num_bytes_shift > 0)
		copy_data(editor.min_offset + num_bytes, editor.min_offset, num_bytes_shift, 1);
	for (i = 0; i < num_bytes; i++)
		*(editor.min_offset + offset++) = '\0';
	return true;
	
	ERROR:
	draw_message_dialog("Byte insertion failed");
	while (!os_GetCSC());
	return false;
}

void create_file_delete_bytes_undo_action(void) {
	
	ti_var_t undo_appvar;
	uint8_t undo_code = UNDO_DELETE_BYTES;
	
	if ((undo_appvar = ti_Open(UNDO_APPVAR_NAME, "r+")) != NULL) {
	
		ti_Write(&undo_code, 1, 1, undo_appvar);
		ti_Write(&editor.sel_byte_string_offset, 3, 1, undo_appvar);
		ti_Write(&editor.num_selected_bytes, 1, 1, undo_appvar);
		ti_Seek(5, SEEK_SET, undo_appvar);
		copy_data(editor.sel_byte_string_offset, ti_GetDataPtr(undo_appvar), editor.num_selected_bytes, 1);
		ti_Close(undo_appvar);
		return;
	};
	
	draw_message_dialog("Failed to create undo action");
	while (!os_GetCSC());
	return;
}

bool file_delete_bytes(uint24_t offset, uint8_t num_bytes) {

	/* When the TI-OS resizes a file, any pointers to offsets within it
	   become inaccurate. It also does not add or remove bytes
	   from the end of the file, but from the start of the file */
	   
	/* The above means that the edit_offset and sel_byte_string_offset will be inaccurate */
	   
	/* If the cursor is at the end of the file and the number of bytes requested to be deleted
	   includes the byte the cursor is on, move the cursor to the last byte of the file */

	uint24_t num_bytes_shift = offset;

	// Debugging
	dbg_sprintf(dbgout, "[Start file_delete_bytes()]\t: min_offset = 0x%6x\t| max_offset = 0x%6x\n", editor.min_offset, editor.max_offset);
	dbg_sprintf(dbgout, "[Start file_delete_bytes()]\t: offset = 0x%6x\t| cursor_offset = 0x%6x\n", offset, editor.cursor_offset);
	dbg_sprintf(dbgout, "[Start file_delete_bytes()]\t: sel_byte_string_offset = 0x%6x\t| num_bytes = %d\n", editor.sel_byte_string_offset, num_bytes);

	if (num_bytes_shift > 0) {
		copy_data(editor.sel_byte_string_offset - 1, editor.cursor_offset, num_bytes_shift, 0);
	};

	if (ti_Resize(ti_GetSize(editor.file) - num_bytes, editor.file)) {
		editor.cursor_offset = editor.min_offset + num_bytes_shift;
		editor.max_offset = editor.min_offset + ti_GetSize(editor.file);
		if (editor.cursor_offset > editor.max_offset - 1)
			editor.cursor_offset = editor.max_offset - 1;
	} else {
		draw_message_dialog("Byte deletion failed");
		while (!os_GetCSC());
		return false;
	};

	// Debugging
	dbg_sprintf(dbgout, "[Exit file_delete_bytes()]\t: min_offset = 0x%6x\t| max_offset 0x%6x\n", editor.min_offset, editor.max_offset);
	dbg_sprintf(dbgout, "[Exit file_delete_bytes()]\t: sel_byte_string_offset = 0x%6x\t| cursor_offset = 0x%6x\n", editor.sel_byte_string_offset, editor.cursor_offset);

	return true;
}

//------------------------------------------------------------------------------------

void undo_action(void) {
	
	ti_var_t undo_appvar;
	uint8_t undo_code = 255, byte;
	uint8_t num_bytes;
	
	if ((undo_appvar = ti_Open(UNDO_APPVAR_NAME, "r")) == NULL) {
		draw_message_dialog("Failed to open undo appvar");
		while (!os_GetCSC());
		return;
	};
	
	ti_Read(&undo_code, 1, 1, undo_appvar);
	
	switch(undo_code) {
		
		case UNDO_GOTO:
			ti_Read(&editor.cursor_offset, 3, 1, undo_appvar);
			editor_goto((uint24_t)editor.cursor_offset);
			break;
		
		case UNDO_WRITE_NIBBLE:
			ti_Read(&editor.cursor_offset, 3, 1, undo_appvar);
			ti_Read(&byte, 1, 1, undo_appvar);
			editor_goto((uint24_t)editor.cursor_offset);
			*editor.cursor_offset = byte;
			break;
		
		case UNDO_INSERT_BYTES:
			ti_Read(&editor.sel_byte_string_offset, 3, 1, undo_appvar);
			ti_Read(&num_bytes, 1, 1, undo_appvar);
			editor.cursor_offset = editor.sel_byte_string_offset + num_bytes;
			editor_goto((uint24_t)editor.cursor_offset);
			file_delete_bytes(editor.sel_byte_string_offset - editor.min_offset, num_bytes);
			break;
			
		case UNDO_DELETE_BYTES:
			ti_Read(&editor.cursor_offset, 3, 1, undo_appvar);
			ti_Read(&num_bytes, 1, 1, undo_appvar);
			editor_goto((uint24_t)editor.cursor_offset);
			file_insert_bytes(editor.cursor_offset - editor.min_offset, num_bytes);
			
			// Debugging
			dbg_sprintf(dbgout, "[routines.c] [undo_action()]: num_bytes = %d; 0x%6x | from = 0x%6x | to = 0x%6x\n", num_bytes, num_bytes,ti_GetDataPtr(undo_appvar), editor.cursor_offset);
			
			ti_Seek(5, SEEK_SET, undo_appvar);
			
			// Debugging
			dbg_Debugger();
			
			copy_data(ti_GetDataPtr(undo_appvar), editor.cursor_offset, num_bytes, 1);
			break;
		
		default:
			draw_message_dialog("Unknown undo action request");
			while (!os_GetCSC());
			break;
	};
	
	// Disable the "Undo" function
	editor.undo_activated = false;
	
	ti_Close(undo_appvar);
	return;
}