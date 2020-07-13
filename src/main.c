
/*-----------------------------------------
 * Program Name: HexaEdit CE
 * Version:      1.2.0 CE
 * Author:       Captain Calc   
 * Description:  A hex editor for the TI-84
 *               Plus CE.
 *-----------------------------------------
*/

#include "defines.h"
#include "easter_eggs.h"
#include "editor.h"
#include "gui.h"
#include "routines.h"

#include <string.h>

#include <tice.h>
#include <graphx.h>
#include <fileioc.h>
#include <keypadc.h>
#include <debug.h>

#define PROGRAM_NAME		"HexaEdit "
#define PROGRAM_VERSION		"1.2.0"
#define RECENT_FILES_APPVAR	"HXAEDRCF"

typedef struct {
	uint8_t asm_prgms;
	uint8_t ti_prgms;
	uint8_t appvars;
	uint8_t recent;
} num_files_per_type_t;

num_files_per_type_t num_files_per_type;

typedef struct {
	char *name;
	uint8_t editor_file_type;
} sel_file_data_t;

sel_file_data_t sel_file_data;


static void create_undo_action_appvar(void) {

	ti_var_t appvar;

	ti_CloseAll();
	if ((appvar = ti_Open(UNDO_APPVAR_NAME, "w+")) == NULL)
		goto ERROR;
	
	if (ti_Resize(104, appvar) <= 0)
		goto ERROR;
	
	ti_CloseAll();
	return;
	
	ERROR:
	draw_message_dialog("Failed to create undo appvar");
	while (!os_GetCSC());
	return;
}

static void draw_title_bar(void) {
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, 0, 180, 20);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
	gfx_PrintStringXY(PROGRAM_NAME, 5, 6);
	gfx_PrintStringXY(PROGRAM_VERSION, 7 + gfx_GetStringWidth(PROGRAM_NAME), 6);
	return;
}

static void draw_menu_bar(uint8_t editor_file_type) {

	uint8_t i;
	uint8_t box_xPos[3] = {0, 67, 140};
	uint8_t box_width[3] = {34, 26, 42};
	const char *editor_file_type_names[3] = {"ASM", "TI", "AppV"};

	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, 220, LCD_WIDTH, 20);
	gfx_SetColor(LT_GRAY);
	gfx_VertLine_NoClip(188, 221, 18);
	gfx_SetColor(BLACK);
	gfx_FillRectangle_NoClip(box_xPos[editor_file_type - 1], 220, box_width[(editor_file_type - 1)], 20);
	gfx_SetColor(LT_GRAY);
	gfx_FillRectangle_NoClip(box_xPos[editor_file_type - 1] + 1, 221, box_width[(editor_file_type - 1)] - 2, 18);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextTransparentColor(DK_GRAY);
	
	for (i = 0; i < 3; i++) {
		gfx_SetTextFGColor(WHITE);
		if (editor_file_type - 1 == i)
			gfx_SetTextFGColor(BLACK);
		gfx_SetTextXY(box_xPos[i] + 5, 226);
		gfx_PrintString(editor_file_type_names[i]);
	};
	
	gfx_SetTextFGColor(WHITE);
	gfx_PrintStringXY("RAM", 230, 226);
	gfx_PrintStringXY("Exit", 286, 226);
	
	return;
}

static void open_context_menu(char *file_name, uint8_t editor_file_type, uint24_t xPos, uint8_t yPos) {

	ti_var_t file;
	uint8_t ti_file_type;
	uint8_t i;
	
	char hex[9] = {'\0'};
	
	uint24_t width = 170;
	uint8_t height = 70;
	
	ti_file_type = find_ti_file_type(editor_file_type);

	// Open file
	ti_CloseAll();
	if ((file = ti_OpenVar(file_name, "r", ti_file_type)) == NULL)
		return;

	// Draw window
	gfx_SetColor(WHITE);
	gfx_HorizLine(xPos, yPos + 14, width);
	draw_window(file_name, true, xPos, yPos, width, height);
	
	gfx_SetTextBGColor(WHITE);
	gfx_SetTextFGColor(BLACK);
	gfx_SetTextTransparentColor(WHITE);

	// Get its size
	gfx_PrintStringXY("Size:", xPos + 3, yPos + 18);
	gfx_SetTextXY(xPos + 100, yPos + 18);
	gfx_PrintUInt(ti_GetSize(file), 5);
	// Determine its archive status
	gfx_PrintStringXY("Archived:", xPos + 3, yPos + 27);
	gfx_SetTextXY(xPos + 100, yPos + 27);
	gfx_PrintUInt((uint8_t)ti_IsArchived(file), 1);
	// Retrieve its VAT pointer
	gfx_PrintStringXY("VAT Ptr:", xPos + 3, yPos + 36);
	
	sprintf(hex, "0x%6x", (uint8_t *)ti_GetVATPtr(file));
	i = 2;
	while (*(hex + i) == ' ')
		*(hex + i++) = '0';
	
	gfx_PrintStringXY(hex, xPos + 100, yPos + 36);
	// Get its data pointer and convert it to its memory pointer
	gfx_PrintStringXY("Address:", xPos + 3, yPos + 45);
	
	sprintf(hex, "0x%6x", (uint8_t *)ti_GetDataPtr(file) - 2);
	i = 2;
	while (*(hex + i) == ' ')
		*(hex + i++) = '0';
	
	gfx_PrintStringXY(hex, xPos + 100, yPos + 45);

	ti_CloseAll();
	
	// Wait for keypress
	gfx_BlitRectangle(1, xPos, yPos, width, height);
	delay(200);
	while (!os_GetCSC());
	return;
}

/* Search routine for quickly finding files.
   Automatically sets the list_offset and sel_file_left_window variables to the desired file, if found */
static void search_files(uint8_t editor_file_type, uint8_t *list_offset, uint8_t *sel_file_left_window) {
	
	uint8_t input_x;
	uint8_t input_y;
	uint8_t input_w;
	uint8_t input_h;
	
	uint8_t local_list_offset = 0;
	uint8_t local_sel_file = 0;
	
	uint8_t key, i = 0;
	uint8_t buffer_width = 0;
	uint8_t *detect_str = NULL;
	uint8_t ti_file_type;
	
	const char *uppercase_letters = "\0\0\0\0\0\0\0\0\0\0\0WRMH\0\0\0[VQLG\0\0\0ZUPKFC\0\0YTOJEB\0\0XSNIDA\0\0\0\0\0\0\0\0";
	const char *lowercase_letters = "\0\0\0\0\0\0\0\0\0\0\0wrmh\0\0\0[vqlg\0\0\0zupkfc\0\0ytojeb\0\0xsnida\0\0\0\0\0\0\0\0";
	const char *numbers = "\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x33\x36\x39\0\0\0\0\0\x32\x35\x38\0\0\0\0\x30\x31\x34\x37\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0";
	char *keymap = uppercase_letters;
	char buffer[9] = '\0';
	char *file_name;
	
	
	gfx_SetColor(WHITE);
	gfx_HorizLine(75, 99, 170);
	draw_window("Search Files", true, 75, 85, 170, 70);
	
	gfx_SetTextBGColor(WHITE);
	gfx_SetTextFGColor(BLACK);
	gfx_SetTextTransparentColor(WHITE);
	gfx_PrintStringXY("File name:", 80, 116);
	
	gfx_SetColor(BLACK);
	gfx_FillRectangle_NoClip(80, 125, 160, 13);
	input_x = 81;
	input_y = 126;
	input_w = 158;
	input_h = 11;
	
	for (;;) {
		
		gfx_SetColor(WHITE);
		gfx_FillRectangle_NoClip(input_x, input_y, input_w, input_h);
		gfx_SetTextXY(input_x + 1, input_y + 2);
		gfx_SetTextBGColor(WHITE);
		gfx_SetTextFGColor(BLACK);
		gfx_SetTextTransparentColor(WHITE);
		
		gfx_SetColor(BLACK);
		print_file_name(buffer);
		
		gfx_SetColor(BLACK);
		gfx_FillRectangle_NoClip(gfx_GetTextX(), input_y + 1, 9, 9);
		
		gfx_SetTextBGColor(BLACK);
		gfx_SetTextFGColor(WHITE);
		gfx_SetTextTransparentColor(BLACK);
		gfx_SetTextXY(gfx_GetTextX() + 1, gfx_GetTextY());
		
		if (keymap == uppercase_letters) {
			gfx_PrintChar('A');
		} else if (keymap == lowercase_letters) {
			gfx_PrintChar('a');
		} else {
			gfx_PrintChar('0');
		};
		
		gfx_SetTextXY(gfx_GetTextX() - 1, gfx_GetTextY());
			
		gfx_BlitRectangle(1, 75, 85, 170, 70);
		while (!(key = os_GetCSC()));
		
		if (keymap[key] && i < 8) {
			buffer[i] = keymap[key];
			if (*(buffer + i) != '[') {
				buffer_width += gfx_GetCharWidth(buffer[i]);
			} else {
				buffer_width += gfx_GetCharWidth('O');
			};
			i++;
		};
		
		if (key == sk_Del && i > 0) {
			buffer_width -= gfx_GetCharWidth(buffer[--i]);
			*(buffer + i) = '\0';
		};
		
		if (key == sk_Alpha) {
			if (keymap == uppercase_letters) {
				keymap = lowercase_letters;
			} else if (keymap == lowercase_letters) {
				keymap = numbers;
			} else {
				keymap = uppercase_letters;
			};
		};
		
		if (key == sk_Clear)
			return;
		
		if (key == sk_2nd)
			break;
	};
	
	ti_file_type = find_ti_file_type(editor_file_type);
	
	while (((file_name = ti_DetectVar(&detect_str, NULL, ti_file_type)) != NULL)) {
		if (!strcmp(file_name, buffer)) {
			*list_offset = local_list_offset;
			*sel_file_left_window = local_sel_file;
			return;
		};
		
		local_sel_file++;
		if (local_sel_file == MAX_LINES_ONSCREEN) {
			local_sel_file--;
			local_list_offset++;
		};
	};
	
	gfx_SetTextFGColor(RED);
	gfx_PrintStringXY("File not found!", 110, 143);
	gfx_BlitRectangle(1, 75, 85, 170, 70);
	while (!os_GetCSC());
	return;
}

static void get_num_asm_prgms(void) {
	
	uint8_t *offset_ptr = NULL;
	uint8_t num_asm_prgms = 0;
	
	while (ti_DetectVar(&offset_ptr, NULL, TI_PPRGM_TYPE) != NULL)
		num_asm_prgms++;
	
	num_files_per_type.asm_prgms = num_asm_prgms;
	return;
}

static void get_num_ti_prgms(void) {
	
	uint8_t *offset_ptr = NULL;
	uint8_t num_ti_prgms = 0;
	char *file_name;
	
	while ((file_name = ti_DetectVar(&offset_ptr, NULL, TI_PRGM_TYPE)) != NULL)
		if (*file_name != '!' && *file_name != '#')
			num_ti_prgms++;
	
	num_files_per_type.ti_prgms = num_ti_prgms;
	return;
}

static void get_num_appvars(void) {
	
	uint8_t *offset_ptr = NULL;
	uint8_t num_appvars = 0;
	
	while (ti_Detect(&offset_ptr, NULL) != NULL)
		num_appvars++;
	
	num_files_per_type.appvars = num_appvars;
	return;
}

static void print_file_size(char *file_name, uint8_t ti_file_type, uint8_t print_y) {
	
	ti_var_t file;
	uint16_t file_size;
	
	ti_CloseAll();
	file = ti_OpenVar(file_name, "r", ti_file_type);
	file_size = ti_GetSize(file);
	gfx_SetTextXY(140, print_y);
	gfx_PrintUInt(file_size, 5);
	ti_CloseAll();
	return;
}

static void print_file_entry(char *file_name, uint8_t ti_file_type, uint8_t num_files_printed) {
	
	uint8_t print_y;
	
	print_y = 44 + 11 * num_files_printed;
	gfx_SetTextXY(8, print_y);
	print_file_name(file_name);
	print_file_size(file_name, ti_file_type, print_y);
	return;
}

static void print_files(uint8_t sel_window, uint8_t list_offset, uint8_t sel_file, uint8_t ti_file_type) {
	
	const char *empty_file_list = "-- No files found --";
	uint8_t *detect_str = NULL;
	uint8_t num_files_retrieved = 0, num_files_printed = 0;
	
	char *file_name;
	static char sel_file_name[9] = {'\0'};
	
	gfx_SetTextBGColor(LT_GRAY);
	gfx_SetTextTransparentColor(LT_GRAY);
	
	while (((file_name = ti_DetectVar(&detect_str, NULL, ti_file_type)) != NULL) && num_files_printed < MAX_LINES_ONSCREEN) {
		
		if (++num_files_retrieved > list_offset) {
			
			if (*file_name != '!' && *file_name != '#') {
				
				gfx_SetTextFGColor(BLACK);
				gfx_SetColor(BLACK);
				if (num_files_printed == sel_file) {
					gfx_SetTextFGColor(WHITE);
					gfx_FillRectangle_NoClip(7, 42 + 11 * sel_file, 176, 11);
					gfx_SetColor(WHITE);
					strcpy(sel_file_name, file_name);
					sel_file_data.name = sel_file_name;
				};
				
				print_file_entry(file_name, ti_file_type, num_files_printed++);
			};
		};
	};
	
	if (num_files_printed == 0) {
		gfx_SetTextBGColor(WHITE);
		gfx_SetTextFGColor(LT_GRAY);
		gfx_SetTextTransparentColor(WHITE);
		gfx_PrintStringXY(empty_file_list, 95 - gfx_GetStringWidth(empty_file_list) / 2, 44);
	};
	
	return;
}

static void draw_left_window(uint8_t sel_window, uint8_t editor_file_type, uint8_t list_offset, uint8_t sel_file) {
	
	uint8_t ti_file_type;
	const char *editor_file_type_headers[3] = {"Assembly Programs", "BASIC Programs", "AppVars"};
	
	if (editor_file_type == ASM_PRGM_FILE) {
		ti_file_type = TI_PPRGM_TYPE;
	} else if (editor_file_type == TI_PRGM_FILE) {
		ti_file_type = TI_PRGM_TYPE;
	} else {
		ti_file_type = TI_APPVAR_TYPE;
	};
	
	if (sel_window == 2)
		sel_window = 0;
	
	draw_window(editor_file_type_headers[editor_file_type - 1], (bool)sel_window, 5, 25, 180, 190);
	print_files(sel_window, list_offset, sel_file, ti_file_type);
	
	// This function updates the selected file info by default
	sel_file_data.editor_file_type = editor_file_type;
	return;
}

static void draw_recent_files (uint8_t sel_window, uint8_t sel_file_right_window) {
	
	uint8_t print_y = 0;
	uint8_t editor_file_type;
	ti_var_t file;
	const char *title = "Recent Files";
	char file_name[9] = {'\0'};
	static char sel_file_name_rw[9] = {'\0'};
	
	if (sel_window == 1) sel_window = 0;
	draw_window("Recent Files", (bool)sel_window, 190, 25, 125, 190);
	
	ti_CloseAll();
	if ((file = ti_Open(RECENT_FILES_APPVAR, "r")) == NULL) {
		draw_message_dialog("Error opening recents appvar. Close HexaEdit");
		while (!os_GetCSC());
		return;
	};
	
	while (ti_Read(file_name, 8, 1, file) != NULL && print_y < MAX_LINES_ONSCREEN) {
		
		gfx_SetTextFGColor(BLACK);
		gfx_SetColor(BLACK);
		if (sel_file_right_window == print_y) {
			gfx_FillRectangle_NoClip(192, 42 + 11 * sel_file_right_window, 121, 11);
			gfx_SetTextFGColor(WHITE);
			gfx_SetColor(WHITE);
			
			// Update the selected file info only if the "Recent Files" window is selected
			if (sel_window == 2) {
				strcpy(sel_file_name_rw, file_name);
				sel_file_data.name = sel_file_name_rw;
			};
		};
		editor_file_type = (uint8_t)ti_GetC(file);
		// Debugging
		dbg_sprintf(dbgout, "[main.c] [draw_recent_files()]\t: file offset = %d | file type = %d\n", ti_Tell(file), editor_file_type);
		if (sel_window == 2 && sel_file_right_window == print_y)
			sel_file_data.editor_file_type = editor_file_type;
		
		gfx_SetTextXY(193, 44 + 11 * print_y++);
		print_file_name(file_name);
	};
	
	num_files_per_type.recent = print_y - 1;
	
	ti_CloseAll();
	
	// Debugging
	dbg_sprintf(dbgout, "[main.c] [draw_recent_files()] : sel_file_name_rw = %s\n", sel_file_name_rw);
	return;
}

static bool create_recents_appvar(void) {
	
	ti_var_t appvar;
	uint8_t i;
	
	ti_CloseAll();
	appvar = ti_Open(RECENT_FILES_APPVAR, "w");
	if (!appvar) {
		// Debugging
		dbg_sprintf(dbgout, "[main.c] [create_recents_appvar()] : Failed to create appvar\n");
		return false;
	};
	
	num_files_per_type.recent = 0;
	
	ti_CloseAll();
	return true;
}

static void add_recent_file(char *file_name, uint8_t editor_file_type) {
	
	ti_var_t rf_appvar, new_rf_appvar;
	char next_file_name[9] = {'\0'};
	uint8_t num_files_copied;

	rf_appvar = ti_Open(RECENT_FILES_APPVAR, "r");
	new_rf_appvar = ti_Open("HXAEDRF2", "w+");
	if (!new_rf_appvar)
		return;

	ti_Write(file_name, 8, 1, new_rf_appvar);
	ti_PutC((char)editor_file_type, new_rf_appvar);
	num_files_copied = 1;

	while (num_files_copied++ < MAX_LINES_ONSCREEN) {
		if (ti_Read(next_file_name, 8, 1, rf_appvar) == NULL)
			break;
		editor_file_type = ti_GetC(rf_appvar);
		// Debugging
		// dbg_sprintf(dbgout, "[main.c] [add_recent_file()] : next_file_name = %s\n", next_file_name);
		if (strcmp(file_name, next_file_name)) {
			ti_Write(next_file_name, 8, 1, new_rf_appvar);
			ti_PutC((uint8_t)editor_file_type, new_rf_appvar);
		};
	};

	ti_CloseAll();
	
	if (ti_Delete(RECENT_FILES_APPVAR))
		ti_Rename("HXAEDRF2", RECENT_FILES_APPVAR);
	
	num_files_per_type.recent = num_files_copied - 1;
	
	return;
}

static void delete_recent_file(char *file_name, uint8_t *sel_file_right_window) {
	
	ti_var_t rf_appvar, new_rf_appvar;
	char next_file_name[9] = {'\0'};
	uint8_t num_files_copied;
	uint8_t editor_file_type;

	rf_appvar = ti_Open(RECENT_FILES_APPVAR, "r");
	new_rf_appvar = ti_Open("HXAEDRF2", "w+");
	if (!new_rf_appvar)
		return;

	num_files_copied = 0;

	while (num_files_copied++ < MAX_LINES_ONSCREEN) {
		if (ti_Read(next_file_name, 8, 1, rf_appvar) == NULL)
			break;
		editor_file_type = ti_GetC(rf_appvar);
		if (strcmp(file_name, next_file_name)) {
			ti_Write(next_file_name, 8, 1, new_rf_appvar);
			ti_PutC((uint8_t)editor_file_type, new_rf_appvar);
		};
	};

	ti_CloseAll();
	
	if (ti_Delete(RECENT_FILES_APPVAR))
		ti_Rename("HXAEDRF2", RECENT_FILES_APPVAR);
	
	num_files_per_type.recent = num_files_copied - 1;
	
	// Debugging
	dbg_sprintf(dbgout, "[main.c] [delete_recent_file()]\t: selected file = %d\n", *sel_file_right_window);
	
	if (*sel_file_right_window > 0)
		(*sel_file_right_window)--;
	
	// Debugging
	dbg_sprintf(dbgout, "[main.c] [delete_recent_file()]\t: selected file = %d\n", *sel_file_right_window);
	
	return;
}

void main(void) {
	
	// General-purpose loop variable
	uint8_t i;
	
	bool redraw_background;
	
	uint8_t editor_file_type;
	uint8_t list_offset;
	uint8_t sel_window;
	uint8_t sel_file_left_window, sel_file_right_window;
	uint8_t num_files_curr_type;
		
	kb_key_t arrows, function;
	bool key_Left, key_Right, key_Up, key_Down;
	bool key_Yequ, key_Window, key_Zoom, key_Trace, key_Graph;
	bool key_2nd, key_Alpha, key_Mode, key_GraphVar, key_Del, key_Clear;
	
	char *sel_file_name;
	
	redraw_background = true;
	editor_file_type = ASM_PRGM_FILE;
	list_offset = 0;
	sel_window = 1;
	sel_file_left_window = 0;
	sel_file_right_window = 0;
	
	gfx_Begin();
	gfx_SetDrawBuffer();
	
	draw_message_dialog("Loading files...");
	
	create_undo_action_appvar();
	
	if (ti_Open(RECENT_FILES_APPVAR, "r") == NULL) {
		if (!create_recents_appvar()) {
			draw_message_dialog("Failed to create appvar. Program will exit");
			while (!os_GetCSC());
			close_program();
		};
	};
	
	get_num_asm_prgms();
	get_num_ti_prgms();
	get_num_appvars();
	num_files_curr_type = 0;

	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, 0, LCD_WIDTH, 20);
	draw_battery_status();

	for (;;) {
		
		switch (editor_file_type) {
			
			case ASM_PRGM_FILE:
				num_files_curr_type = num_files_per_type.asm_prgms;
				break;
				
			case TI_PRGM_FILE:
				num_files_curr_type = num_files_per_type.ti_prgms;
				break;
				
			case APPVAR_FILE:
				num_files_curr_type = num_files_per_type.appvars;
				break;
		};
		
		if (redraw_background) {
			gfx_SetColor(LT_GRAY);
			gfx_FillRectangle_NoClip(0, 20, LCD_WIDTH, 220);
			redraw_background = false;
		};
		draw_title_bar();
		draw_left_window(sel_window, editor_file_type, list_offset, sel_file_left_window);
		draw_recent_files(sel_window, sel_file_right_window);
		draw_menu_bar(editor_file_type);
		
		// Debugging
		dbg_sprintf(dbgout, "[main.c] [Drawing complete]\t: name = %s | editor_file_type = %d\n", sel_file_data.name, sel_file_data.editor_file_type);
		
		do {
			draw_time(180);
			gfx_BlitBuffer();
			kb_Scan();
		} while (!kb_AnyKey());
		
		arrows = kb_Data[7];
		key_Left = arrows & kb_Left;
		key_Right = arrows & kb_Right;
		key_Up = arrows & kb_Up;
		key_Down = arrows & kb_Down;
		
		function = kb_Data[1];
		key_Yequ = function & kb_Yequ;
		key_Window = function & kb_Window;
		key_Zoom = function & kb_Zoom;
		key_Trace = function & kb_Trace;
		key_Graph = function & kb_Graph;
		key_2nd = function & kb_2nd;
		key_Mode = function & kb_Mode;
		key_Del = function & kb_Del;
		
		key_Alpha = kb_Data[2] & kb_Alpha;
		key_GraphVar = kb_Data[3] & kb_GraphVar;
		key_Clear = kb_Data[6] & kb_Clear;
		
		redraw_background = easter_egg_two();
		
		if (key_Clear || key_Graph)
			close_program();
	
		if (key_Yequ || key_Window || key_Zoom) {
			list_offset = 0;
			sel_file_left_window = 0;
			if (key_Yequ) editor_file_type = ASM_PRGM_FILE;
			if (key_Window) editor_file_type = TI_PRGM_FILE;
			if (key_Zoom) editor_file_type = APPVAR_FILE;
		};
		
		if (key_Trace) {
			edit_ram();
			redraw_background = true;
		};
		
		if (key_Up) {
			if (sel_window == 1) {
				
				// Alpha accelerated scrolling
				if (key_Alpha) {
					if (list_offset == 0)
						sel_file_left_window = 0;
					
					i = MAX_LINES_ONSCREEN;
					while (i-- > 0 && list_offset > 0)
						list_offset--;
				} else {
					if (sel_file_left_window > 0)
						sel_file_left_window--;
					else if (list_offset > 0)
						list_offset--;
				};
				
			} else {
				if (sel_file_right_window > 0)
					sel_file_right_window--;
			};
		};
		
		if (key_Down) {
			if (sel_window == 1) {
				
				// Alpha accelerated scrolling
				if (key_Alpha) {
					if (list_offset + MAX_LINES_ONSCREEN >= num_files_curr_type)
						sel_file_left_window = num_files_curr_type - list_offset - 1;
					
					i = 0;
					while (i++ < MAX_LINES_ONSCREEN && (list_offset + MAX_LINES_ONSCREEN < num_files_curr_type))
						list_offset++;
				} else {
					if (sel_file_left_window == (MAX_LINES_ONSCREEN - 1) && (list_offset + sel_file_left_window + 1) < num_files_curr_type)
						list_offset++;
					if (sel_file_left_window + 1 < num_files_curr_type && sel_file_left_window < MAX_LINES_ONSCREEN - 1)
						sel_file_left_window++;
				};
				
			} else {
				if (sel_file_right_window < num_files_per_type.recent)
					sel_file_right_window++;
			};
		};
		
		if (key_Left && sel_window > 1)
			sel_window--;
		
		if (key_Right && sel_window < 2) {
			sel_file_right_window = 0;
			sel_window++;
		};
		
		if (key_2nd) {
			// Debugging
			//dbg_sprintf(dbgout, "[main.c] [Starting add_recent_file()] : sel_file_name = %s | ", sel_file_data.name);
			//for (i = 0; i < 9; i++)
			//	dbg_sprintf(dbgout, "%2x ", sel_file_data.name[i]);
			//dbg_sprintf(dbgout, "\n");
			
			add_recent_file(sel_file_data.name, sel_file_data.editor_file_type);
			
			// Debugging
			// dbg_sprintf(dbgout, "[main.c] [Starting edit_file()] : sel_file_name = %s\n", sel_file_name);
			edit_file(sel_file_data.name, sel_file_data.editor_file_type);
			redraw_background = true;
			delay(200);
		};
		
		if (key_Mode) {
			search_files(editor_file_type, &list_offset, &sel_file_left_window);
			redraw_background = true;
			delay(200);
		};
		
		if (key_Del && sel_window == 2) {
			delete_recent_file(sel_file_data.name, &sel_file_right_window);
		};
		
		if (key_GraphVar) {
			open_context_menu(sel_file_data.name, sel_file_data.editor_file_type, 75, 85);
			redraw_background = true;
			delay(200);
		};
	};
	
	close_program();
}
