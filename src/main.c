
/*-----------------------------------------
 * Program Name: HexaEdit CE
 * Version:      2.0.0 CE
 * Author:       Captain Calc   
 * Description:  A hex editor for the TI-84
 *               Plus CE.
 *-----------------------------------------
*/

#include "asmutil.h"
#include "colors.h"
#include "editor.h"
#include "gui.h"

#include <string.h>

#include <tice.h>
#include <graphx.h>
#include <fileioc.h>
#include <keypadc.h>

// Debugging
#include "debug.h"

#define PROGRAM_NAME		"HexaEdit"
#define PROGRAM_VERSION		"2.0.0"
#define RECENT_FILES_APPVAR	"HEXARCF"

#define NUM_FILES_ONSCREEN	15

/* This is not a regular TI file type, but it is treated like one for menu purposes. */
#define RECENTS_TYPE	7

#define NUM_TABLES		4
#define RECENTS_TABLE_NUM	0
#define APPVAR_TABLE_NUM	1
#define PROT_PRGM_TABLE_NUM	2
#define PGRM_TABLE_NUM		3

uint8_t num_files[NUM_TABLES];
uint8_t table_order[] = {RECENTS_TYPE, TI_APPVAR_TYPE, TI_PPRGM_TYPE, TI_PRGM_TYPE};
char *table_names[] = {"Recents", "Appvars", "Prot. Prgms", "Prgms"};

const char *column_headers[] = {"Name", "Arc", "VAT Ptr", "Data Ptr", "Size"};
uint8_t col_widths[] = {69, 30, 75, 70, 64};

#define COL_1_X		TABLE_LEFT_MARGIN
#define COL_2_X		COL_1_X + col_widths[0]
#define COL_3_X		COL_2_X + col_widths[1]
#define COL_4_X		COL_3_X + col_widths[2]
#define COL_5_X		COL_4_X + col_widths[3]

#define TABLE_LEFT_MARGIN	6
#define TABLE_RIGHT_MARGIN	314
#define TABLE_WIDTH		(TABLE_RIGHT_MARGIN - TABLE_LEFT_MARGIN)
#define TABLE_FIRST_ROW_Y	55
#define TABLE_ROW_HEIGHT	11
#define TABLE_NUM_COLS		5

color_theme_config_t color_theme;


static void load_default_color_theme(void)
{
	color_theme.background_color = LT_GRAY;
	color_theme.bar_color = DK_GRAY;
	color_theme.bar_text_color = WHITE;
	color_theme.table_bg_color = WHITE;
	color_theme.table_text_color = BLACK;
	color_theme.selected_table_text_color = WHITE;
	color_theme.cursor_color = BLUE;
	return;
}

static bool create_recents_appvar(void)
{
	ti_var_t appvar;
	
	ti_CloseAll();
	appvar = ti_Open(RECENT_FILES_APPVAR, "w");
	if (!appvar) {
		return false;
	};
	
	num_files[RECENTS_TABLE_NUM] = 0;
	
	ti_Close(appvar);
	return true;
}

static void add_recent_file(char *name, uint8_t type)
{
	ti_var_t rf_appvar, new_rf_appvar;
	char next_file_name[9] = {'\0'};
	uint8_t num_files_copied;

	rf_appvar = ti_Open(RECENT_FILES_APPVAR, "r");
	new_rf_appvar = ti_Open("HXAEDRF2", "w+");
	if (!new_rf_appvar)
		return;

	ti_Write(name, 8, 1, new_rf_appvar);
	ti_PutC((char)type, new_rf_appvar);
	num_files_copied = 1;

	while (num_files_copied++ < NUM_FILES_ONSCREEN) {
		if (ti_Read(next_file_name, 8, 1, rf_appvar) == 0)
			break;
		type = ti_GetC(rf_appvar);
		// Debugging
		// dbg_sprintf(dbgout, "[main.c] [add_recent_file()] : next_file_name = %s\n", next_file_name);
		if (strcmp(name, next_file_name)) {
			ti_Write(next_file_name, 8, 1, new_rf_appvar);
			ti_PutC((uint8_t)type, new_rf_appvar);
		};
	};

	ti_CloseAll();
	
	if (ti_Delete(RECENT_FILES_APPVAR))
		ti_Rename("HXAEDRF2", RECENT_FILES_APPVAR);
	
	num_files[RECENTS_TABLE_NUM] = num_files_copied - 1;
	
	return;
}

static void delete_recent_file(char *file_name)
{
	ti_var_t rf_appvar;
	char rf_file_name[9] = {'\0'};

	rf_appvar = ti_Open(RECENT_FILES_APPVAR, "r+");
	if (!rf_appvar)
		return;

	while (ti_Read(rf_file_name, 8, 1, rf_appvar) != 0) {
		
		ti_GetC(rf_appvar);
		if (!strcmp(file_name, rf_file_name)) {
			if (ti_Tell(rf_appvar) >= 10)
				asm_CopyData(ti_GetDataPtr(rf_appvar) - 10, ti_GetDataPtr(rf_appvar) - 1, ti_Tell(rf_appvar) - 9, 0);
			ti_Resize(ti_GetSize(rf_appvar) - 9, rf_appvar);
			
			ti_Close(rf_appvar);
			num_files[RECENTS_TABLE_NUM]--;
			return;
		};
	}

	ti_Close(rf_appvar);
	return;
}

static void update_recent_files_list(void)
{
	ti_var_t rf_appvar, file;
	char file_name[9] = {'\0'};
	uint8_t file_type;

	ti_CloseAll();
	if ((rf_appvar = ti_Open(RECENT_FILES_APPVAR, "r")) == 0) {
		gui_DrawMessageDialog_Blocking("Could not update Recent Files list");
		return;
	};

	while (ti_Read(file_name, 8, 1, rf_appvar) != 0) {
		
		file_type = (uint8_t)ti_GetC(rf_appvar);
		if ((file = ti_OpenVar(file_name, "r", file_type)) == 0) {
			delete_recent_file(file_name);
			// Seek back one entry since the appvar now contains one less entry
			ti_Seek(-9, SEEK_CUR, rf_appvar);
		} else {
			ti_Close(file);
		};
	};

	ti_Close(rf_appvar);
	return;
}

static uint8_t get_recent_file_type(char *file_name)
{
	ti_var_t rf_appvar;
	uint8_t file_type;
	char curr_file_name[9] = {'\0'};
	
	if ((rf_appvar = ti_Open(RECENT_FILES_APPVAR, "r")) == 0)
	{
		return 0;
	};
	
	while (ti_Read(&curr_file_name, 8, 1, rf_appvar) != 0)
	{
		file_type = (uint8_t)ti_GetC(rf_appvar);
		if (!strcmp(file_name, curr_file_name))
		{
			ti_Close(rf_appvar);
			return file_type;
		};
	};
	
	ti_Close(rf_appvar);
	return 0;
}

static char *get_recent_file(uint24_t num)
{
	ti_var_t rf_appvar;
	static char file_name[9] = {'\0'};
	
	if ((rf_appvar = ti_Open(RECENT_FILES_APPVAR, "r")) == 0)
	{
		return NULL;
	};
	
	if (ti_GetSize(rf_appvar) == 0)
	{
		return NULL;
	};
	
	if (ti_Seek(num * 9, SEEK_SET, rf_appvar) == EOF)
	{
		return NULL;
	};
	
	if (ti_Tell(rf_appvar) == ti_GetSize(rf_appvar))
	{
		return NULL;
	};
	
	//dbg_sprintf(dbgout, "offset = %d\n", num * 9);
	
	/* Seek past the file type. */
	ti_Read(&file_name, 8, 1, rf_appvar);
	return file_name;
}

static void draw_title_bar(void)
{
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(0, 0, LCD_WIDTH, 20);
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	gfx_PrintStringXY(PROGRAM_NAME, 5, 6);
	gfx_PrintStringXY(PROGRAM_VERSION, 9 + gfx_GetStringWidth(PROGRAM_NAME), 6);
	gui_DrawBatteryStatus();
	return;
}

static void draw_menu_bar(void)
{
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(0, 220, LCD_WIDTH, 20);
	gfx_SetColor(color_theme.background_color);
	gfx_VertLine_NoClip(281, 221, 18);
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_PrintStringXY("RAM", 5, 226);
	gfx_PrintStringXY("ROM", 70, 226);
	gfx_PrintStringXY("Search", 130, 226);
	gfx_PrintStringXY("Exit", 286, 226);
	
	return;
}

static void get_num_recent_files(void)
{
	ti_var_t rf_appvar;
	
	if ((rf_appvar = ti_Open(RECENT_FILES_APPVAR, "r")) == 0)
	{
		num_files[RECENTS_TABLE_NUM] = 0;
		return;
	};
	
	num_files[RECENTS_TABLE_NUM] = ti_GetSize(rf_appvar) / 9;
	return;
}

static void get_num_protected_prgms(void)
{
	void *offset_ptr = NULL;
	uint8_t num_prot_prgms = 0;
	
	while (ti_DetectVar(&offset_ptr, NULL, TI_PPRGM_TYPE) != 0)
		num_prot_prgms++;
	
	num_files[PROT_PRGM_TABLE_NUM] = num_prot_prgms;
	return;
}

static void get_num_prgms(void)
{
	void *offset_ptr = NULL;
	uint8_t num_prgms = 0;
	char *file_name;
	
	while ((file_name = ti_DetectVar(&offset_ptr, NULL, TI_PRGM_TYPE)) != 0)
		if (*file_name != '!' && *file_name != '#')
			num_prgms++;
	
	num_files[PGRM_TABLE_NUM] = num_prgms;
	return;
}

static void get_num_appvars(void)
{
	void *offset_ptr = NULL;
	uint8_t num_appvars = 0;
	
	while (ti_Detect(&offset_ptr, NULL) != 0)
		num_appvars++;
	
	num_files[APPVAR_TABLE_NUM] = num_appvars;
	return;
}

static void draw_file_list_header(uint8_t y, uint8_t table_num)
{
	//uint24_t x = (LCD_WIDTH / 2) - gfx_GetStringWidth(table_names[table_num]) / 2;
	uint24_t x = 5;
	uint8_t i;
	
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(0, y, LCD_WIDTH, 12);
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	gfx_SetColor(LT_GRAY);
	
	for (i = 0; i < NUM_TABLES; i++)
	{
		gfx_SetTextFGColor(color_theme.selected_table_text_color);
		if (table_num == i)
		{
			gfx_FillRectangle_NoClip(x - 3, y, gfx_GetStringWidth(table_names[i]) + 4, 12);
			gfx_SetTextFGColor(color_theme.table_selector_color);
		};
		gfx_PrintStringXY(table_names[i], x, y + 2);
		x += gfx_GetStringWidth(table_names[i]) + 15;
	};
	return;
}

static void draw_table_header(uint8_t y)
{
	uint8_t x = TABLE_LEFT_MARGIN + 1;
	uint8_t col;
	
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(TABLE_LEFT_MARGIN - 2, y, TABLE_WIDTH + 4, 16);
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	
	y += 4;
	
	for (col = 0; col < TABLE_NUM_COLS; col++)
	{
		gfx_PrintStringXY(column_headers[col], x, y);
		x += col_widths[col];
	};
	return;
}

static void sprintf_ptr(char *buffer, unsigned int pointer)
{
	char *c;
	
	sprintf(buffer, "0x%6x", pointer);
	c = buffer;
	
	while (*c != '\0')
	{
		if (*c == ' ')
			*c = '0';
		c++;
	};
	return;
}

static void print_file_entry(char *name, uint8_t type, uint8_t y)
{
	ti_var_t file;
	char buffer[9] = {'\0'};
	
	if (type == RECENTS_TYPE)
	{
		type = get_recent_file_type(name);
	};
	
	gfx_SetTextXY(COL_1_X + 1, y);
	gui_PrintFileName(name);
	
	ti_CloseAll();
	if ((file = ti_OpenVar(name, "r", type)) == 0)
	{
		gfx_PrintStringXY("-- Error --", LCD_WIDTH / 2 - 50, y);
		return;
	};
	
	// Print file archive status.
	if (ti_IsArchived(file))
	{
		gfx_PrintStringXY("*", COL_2_X, y);
	};
	
	// Print file size.
	sprintf(buffer, "%5d", ti_GetSize(file));
	gfx_PrintStringXY(buffer, COL_5_X + (col_widths[4] - gfx_GetStringWidth(buffer)), y);
	
	// Print file VAT pointer.
	sprintf_ptr(buffer, (unsigned int)ti_GetVATPtr(file));
	gfx_PrintStringXY(buffer, COL_3_X, y);
	
	// Print file data pointer.
	sprintf_ptr(buffer, (unsigned int)ti_GetDataPtr(file));
	gfx_PrintStringXY(buffer, COL_4_X, y);
	
	ti_Close(file);
	return;
}

static char *print_file_table(char selected_file_name[], uint8_t type, uint24_t list_offset, uint8_t selected_file_offset)
{
	const char *empty_file_list = "-- No files found --";
	void *detect_str = 0;
	uint24_t num_files_retrieved = 0;
	uint8_t num_files_printed = 0;
	
	char *file_name;
	
	gfx_SetColor(BLACK);
	gfx_FillRectangle_NoClip(TABLE_LEFT_MARGIN - 2, TABLE_FIRST_ROW_Y - 3, TABLE_WIDTH + 4, NUM_FILES_ONSCREEN * TABLE_ROW_HEIGHT + 2);
	gfx_SetColor(color_theme.table_bg_color);
	gfx_FillRectangle_NoClip(TABLE_LEFT_MARGIN - 1, TABLE_FIRST_ROW_Y - 2, TABLE_WIDTH + 2, NUM_FILES_ONSCREEN * TABLE_ROW_HEIGHT);
	
	for(;;)
	{
		if (num_files_printed >= NUM_FILES_ONSCREEN)
		{
			break;
		};
		
		if (type == RECENTS_TYPE)
		{
			file_name = get_recent_file(num_files_printed);
		} else {
			file_name = ti_DetectVar(&detect_str, NULL, type);
		};
		
		if (file_name == NULL)
		{
			break;
		};
		
		if (++num_files_retrieved > list_offset) {
			
			if (*file_name != '!' && *file_name != '#') {
				
				gfx_SetTextBGColor(color_theme.table_bg_color);
				gfx_SetTextFGColor(color_theme.table_text_color);
				gfx_SetTextTransparentColor(color_theme.table_bg_color);
				gfx_SetColor(BLACK);
				if (num_files_printed == selected_file_offset) {
					gfx_SetTextBGColor(BLACK);
					gfx_SetTextFGColor(WHITE);
					gfx_SetTextTransparentColor(BLACK);
					gfx_FillRectangle_NoClip(TABLE_LEFT_MARGIN, TABLE_FIRST_ROW_Y - 1 + (TABLE_ROW_HEIGHT * selected_file_offset), TABLE_WIDTH, TABLE_ROW_HEIGHT - 2);
					gfx_SetColor(WHITE);
					strcpy(selected_file_name, file_name);
				};
				
				print_file_entry(file_name, type, TABLE_FIRST_ROW_Y + (TABLE_ROW_HEIGHT * num_files_printed++));
			};
		};
	};
	
	if (num_files_printed == 0) {
		gfx_SetTextBGColor(WHITE);
		gfx_SetTextFGColor(LT_GRAY);
		gfx_SetTextTransparentColor(WHITE);
		gfx_PrintStringXY(empty_file_list, (LCD_WIDTH / 2) - gfx_GetStringWidth(empty_file_list) / 2, TABLE_FIRST_ROW_Y);
	};
	
	return selected_file_name;
}

static void search_files(uint8_t *table_num, uint24_t *file_num)
{
	char buffer[9] = {'\0'};
	uint8_t buffer_size = 8;
	char *keymap [3] = {UPPERCASE_LETTERS, LOWERCASE_LETTERS, NUMBERS};
	void *detect_str;
	uint24_t file;
	uint8_t curr_table;
	char *file_name;
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	gfx_PrintStringXY("Search Files:", 5, 226);
	gfx_SetColor(BLACK);
	gfx_FillRectangle_NoClip(98, 223, 102, FONT_HEIGHT + 6);
	gfx_SetColor(WHITE);
	gfx_FillRectangle_NoClip(99, 224, 100, FONT_HEIGHT + 4);
	gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	gui_Input(buffer, buffer_size, keymap, 0, 3, 100, 225, 100, FONT_HEIGHT + 4);
	
	if (*buffer == '\0')
	{
		return;
	};
	
	
	for (curr_table = APPVAR_TABLE_NUM; curr_table < NUM_TABLES; curr_table++)
	{
		detect_str = 0;
		file = 0;
		while ((file_name = ti_DetectVar(&detect_str, NULL, table_order[curr_table])) != NULL)
		{
			dbg_sprintf(dbgout, "file_name = %s\n", file_name);
			
			if (!strcmp(buffer, file_name))
			{
				*table_num = curr_table;
				*file_num = file;
				return;
			};
			file++;
		};
	}
	
	/*
	file = 0;
	while ((file_name = ti_DetectVar(&detect_str, NULL, type)) != NULL)
	{
		//dbg_sprintf(dbgout, "file_name = %s\n", file_name);
		
		if (!strcmp(buffer, file_name))
		{
			*table_num = table;
			*file_num = file;
			return;
		};
		file++;
	};
	*/
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	gfx_SetTextFGColor(WHITE);
	gfx_PrintStringXY("File not found!", 5, 226);
	gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	delay(200);
	while (!os_GetCSC());
	return;
}

static bool check_for_headless_start(void)
{
	ti_var_t config_data_slot;
	header_config_t *header = malloc(sizeof(header_config_t));
	
	if ((config_data_slot = ti_Open(HS_CONFIG_APPVAR, "r")) != 0)
	{
		ti_Read(header, 3, 1, config_data_slot);
		
		dbg_sprintf(dbgout, "headless_start_flag = 0x%6x\n", header->headless_start_flag);
		dbg_sprintf(dbgout, "HEADLESS_START_FLAG = 0x%6x\n", HEADLESS_START_FLAG);
		
		free(header);
		ti_Close(config_data_slot);
		if (!memcmp(header->headless_start_flag, HEADLESS_START_FLAG, strlen(HEADLESS_START_FLAG)))
		{
			dbg_sprintf(dbgout, "Starting headlessly...\n");
			
			editor_HeadlessStart();
			return true;
		};
	};
	return false;
}

int main(void)
{
	uint8_t table_num = RECENTS_TABLE_NUM;
	uint8_t table_type;
	uint24_t selected_file = 0;
	int8_t key;
	char selected_file_name[10] = {'\0'};
	
	bool redraw_background = true;
	
	gfx_Begin();
	gfx_SetDrawBuffer();
	
	ti_CloseAll();
	
	load_default_color_theme();
	
	if (check_for_headless_start())
	{
		gfx_End();
		return 0;
	};
	
	gfx_FillScreen(WHITE);
	gui_DrawMessageDialog("Loading files...");
	
	if (ti_Open(RECENT_FILES_APPVAR, "r") == 0)
	{
		create_recents_appvar();
	};
	update_recent_files_list();
	
	get_num_recent_files();
	get_num_appvars();
	get_num_protected_prgms();
	get_num_prgms();
	
	for (;;)
	{
		if (redraw_background)
		{
			gfx_SetColor(LT_GRAY);
			gfx_FillRectangle_NoClip(0, 20, LCD_WIDTH, LCD_HEIGHT - 40);
			draw_title_bar();
			draw_menu_bar();
			draw_table_header(35);
		};
		
		table_type = table_order[table_num];
		draw_file_list_header(21, table_num);
		print_file_table(selected_file_name, table_type, (selected_file / NUM_FILES_ONSCREEN) * (selected_file % (NUM_FILES_ONSCREEN - 1)), selected_file - ((selected_file / NUM_FILES_ONSCREEN) * (selected_file % (NUM_FILES_ONSCREEN - 1))));
		gfx_BlitBuffer();
		
		do {
			kb_Scan();
			gui_DrawTime(200);
		} while ((key = asm_GetCSC()) == -1);
		
		if (key == sk_Left && table_num > 0)
		{
			table_num--;
			selected_file = 0;
		};
		
		if (key == sk_Right && table_num < NUM_TABLES - 1)
		{
			table_num++;
			selected_file = 0;
		};
		
		if (kb_Data[7] & kb_Up)
		{
			if (kb_Data[2] & kb_Alpha)
			{
				if (selected_file > NUM_FILES_ONSCREEN - 1)
				{
					selected_file -= NUM_FILES_ONSCREEN;
					selected_file++;
				} else {
					selected_file = 0;
				};
			}
			else if (selected_file > 0)
			{
				selected_file--;
			};
		};
		
		if (kb_Data[7] & kb_Down)
		{
			if (kb_Data[2] & kb_Alpha)
			{
				if ((selected_file + NUM_FILES_ONSCREEN - 1) < num_files[table_num])
				{
					selected_file += NUM_FILES_ONSCREEN;
					selected_file--;
				} else {
					selected_file = num_files[table_num] - 1;
				};
			}
			else if (selected_file < num_files[table_num] - 1)
			{
				selected_file++;
			};
		};
		
		if (key == sk_1)
			table_num = 0;
		if (key == sk_2)
			table_num = 1;
		if (key == sk_3)
			table_num = 2;
		if (key == sk_4)
			table_num = 3;
		
		if ((key == sk_2nd || key == sk_Enter) && *selected_file_name != '\0')
		{
			gui_DrawMessageDialog("Copying file...");
			if (table_type != RECENTS_TYPE)
			{
				add_recent_file(selected_file_name, table_type);
				editor_FileNormalStart(selected_file_name, table_type);
			} else {
				add_recent_file(selected_file_name, get_recent_file_type(selected_file_name));
				editor_FileNormalStart(selected_file_name, get_recent_file_type(selected_file_name));
			};
			redraw_background = true;
		};
		
		if (key == sk_Yequ)
		{
			editor_RAMNormalStart();
			redraw_background = true;
		};
		
		if (key == sk_Window)
		{
			editor_ROMViewer();
			redraw_background = true;
		};
		
		if (key == sk_Zoom && table_type != RECENTS_TYPE)
		{
			search_files(&table_num, &selected_file);
			redraw_background = true;
		};
		
		if (key == sk_Graph || key == sk_Clear)
		{
			break;
		};
	};
	
	gfx_End();
	return 0;
}
