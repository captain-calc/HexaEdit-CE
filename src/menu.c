
#include "debug.h"

#include "asmutil.h"
#include "colors.h"
#include "cutil.h"
#include "editor.h"
#include "gui.h"
#include "menu.h"
#include "settings.h"

#include <fileioc.h>
#include <graphx.h>
#include <keypadc.h>
#include <tice.h>

#include <stdbool.h>
#include <stdint.h>
#include <string.h>


uint8_t NUM_FILES_PER_TYPE[NUM_TABLES];
uint8_t TABLE_ORDER[4] = {
	HEXAEDIT_RECENTS_TYPE,
	HEXAEDIT_APPVAR_TYPE,
	HEXAEDIT_ASM_PRGM_TYPE,
	HEXAEDIT_BASIC_PRGM_TYPE
};
char *TABLE_NAMES[] = {"Recents", "Appvars", "ASM Prgms", "BASIC Prgms"};

const char *COLUMN_HEADERS[] = {"Name", "Arc", "Lock", "VAT", "Data", "Size"};
uint8_t COLUMN_WIDTHS[] = {70, 30, 40, 60, 53, 55};

#define TABLE_LEFT_MARGIN	6
#define TABLE_RIGHT_MARGIN	314
#define TABLE_WIDTH		(TABLE_RIGHT_MARGIN - TABLE_LEFT_MARGIN)
#define TABLE_FIRST_ROW_Y	55
#define TABLE_ROW_HEIGHT	11
#define TABLE_NUM_COLS		6

#define COL_1_X		TABLE_LEFT_MARGIN
#define COL_2_X		COL_1_X + COLUMN_WIDTHS[0]
#define COL_3_X		COL_2_X + COLUMN_WIDTHS[1]
#define COL_4_X		COL_3_X + COLUMN_WIDTHS[2]
#define COL_5_X		COL_4_X + COLUMN_WIDTHS[3]
#define COL_6_X		COL_5_X + COLUMN_WIDTHS[4]


static file_data_t *get_appvar_data(const char *name)
{
	ti_var_t slot;
	file_data_t *appvar_data;
	
	if ((slot = ti_Open(name, "r")) == 0)
		return NULL;
	
	// dbg_sprintf(dbgout, "Adding appvar \"%s\"\n", name);
	
	appvar_data = malloc(sizeof(file_data_t));
	
	memset(appvar_data->name, '\0', FILE_NAME_LEN);
	strncpy(appvar_data->name, name, strlen(name));
	appvar_data->hexaedit_type = HEXAEDIT_APPVAR_TYPE;
	appvar_data->is_archived = ti_IsArchived(slot);
	appvar_data->is_protected = false;
	appvar_data->vat_ptr = ti_GetVATPtr(slot);
	appvar_data->data_ptr = ti_GetDataPtr(slot);
	appvar_data->size = ti_GetSize(slot);
	ti_Close(slot);

	return appvar_data;
}

static void free_appvars(file_list_t *file_list)
{
	uint16_t i = 0;
	
	while(i < file_list->num_appvars)
	{
		free(file_list->appvars[i++]);
	};
	return;
}

static void load_main_appvars(file_list_t *main_file_list)
{
	void *detect_str = 0;
	char *appvar;
	file_data_t *appvar_data;

	while ((appvar = ti_DetectVar(&detect_str, NULL, TI_APPVAR_TYPE)) != NULL)
	{
		if (main_file_list->num_appvars == MAX_NUM_APPVARS)
			break;
		
		if ((appvar_data = get_appvar_data(appvar)) != NULL)
			main_file_list->appvars[main_file_list->num_appvars++] = appvar_data;
	};
	return;
}

static file_data_t *get_program_data(const char *name)
{
	ti_var_t slot;
	file_data_t *program_data = malloc(sizeof(file_data_t));
	uint16_t asm_program_flag = 0;
	
	slot = ti_OpenVar(name, "r", TI_PRGM_TYPE);
	
	if (slot == 0)
	{
		free(program_data);
		return NULL;
	};
	
	ti_Read(&asm_program_flag, 2, 1, slot);
	if (asm_program_flag == ASM_PROGRAM_FLAG)
	{
		program_data->hexaedit_type = HEXAEDIT_ASM_PRGM_TYPE;
	} else {
		program_data->hexaedit_type = HEXAEDIT_BASIC_PRGM_TYPE;
	};
	
	memset(program_data->name, '\0', FILE_NAME_LEN);
	strncpy(program_data->name, name, strlen(name));
	program_data->is_archived = ti_IsArchived(slot);
	program_data->vat_ptr = ti_GetVATPtr(slot);
	program_data->data_ptr = ti_GetDataPtr(slot);
	program_data->size = ti_GetSize(slot);
	
  program_data->is_protected = false;
  if (*program_data->vat_ptr == TI_PPRGM_TYPE)
    program_data->is_protected = true;
  
	ti_Close(slot);
	return program_data;
}

static void free_asm_programs(file_list_t *file_list)
{
	uint16_t i = 0;
	
	while(i < file_list->num_asm_programs)
	{
		free(file_list->asm_programs[i++]);
	};
	return;
}

static void free_basic_programs(file_list_t *file_list)
{
	uint16_t i = 0;
	
	while(i < file_list->num_basic_programs)
	{
		free(file_list->basic_programs[i++]);
	};
	return;
}

static void load_main_programs(file_list_t *main_file_list, uint8_t ti_type)
{
	void *detect_str = 0;
	char *program;
	ti_var_t test_slot;
	uint16_t asm_program_flag;
	file_data_t *program_data;

	for (;;)
	{
		// A C-implementation of the Python *continue* keyword to avoid deep if-statement
		// nesting.
		CONTINUE:
		
		if ((program = ti_DetectVar(&detect_str, NULL, ti_type)) == NULL)
			return;
		
		if (*program == '!' || *program == '#')
			goto CONTINUE;
		
		if ((program_data = get_program_data(program)) == NULL)
			goto CONTINUE;
		
		if ((test_slot = ti_OpenVar(program, "r", ti_type)) == 0)
			goto CONTINUE;
		
		asm_program_flag = 0;
		ti_Read(&asm_program_flag, 2, 1, test_slot);
		ti_Close(test_slot);
		
		// dbg_sprintf(dbgout, "Adding program \"%s\"\n", program);
		
		if (asm_program_flag == ASM_PROGRAM_FLAG)
		{
			if (main_file_list->num_asm_programs < MAX_NUM_ASM_PROGRAMS)
			{
				main_file_list->asm_programs[main_file_list->num_asm_programs++] = program_data;
			};
		}
		else if (main_file_list->num_basic_programs < MAX_NUM_BASIC_PROGRAMS)
		{
			main_file_list->basic_programs[main_file_list->num_basic_programs++] = program_data;
		};
	};
}

static void load_main_files(file_list_t *main_file_list)
{
	// Since malloc does not initialize the memory it allocates, these
	// counters must be initialized in order to avoid undefined behavior.
	main_file_list->num_appvars = 0;
	main_file_list->num_asm_programs = 0;
	main_file_list->num_basic_programs = 0;
	
	load_main_appvars(main_file_list);
	load_main_programs(main_file_list, TI_PPRGM_TYPE);
	load_main_programs(main_file_list, TI_PRGM_TYPE);
	return;
}

static void free_main_files(file_list_t *file_list)
{
	free_appvars(file_list);
	free_asm_programs(file_list);
	free_basic_programs(file_list);
	free(file_list);
	return;
}

static void add_recent_file(recent_file_list_t *list, file_data_t *file)
{
	uint16_t file_num;
	
	// Verify that the file to be added is not in the list already.
	// If it is, shift all of the other file pointers before it
	// forward by one pointer space and write the file pointer to
	// the first list index.
	
	for (file_num = 0; file_num < list->num_files; file_num++)
	{
		if (list->files[file_num] == file)
		{
			while (file_num > 0)
			{
				list->files[file_num] = list->files[file_num - 1];
				file_num--;
			};
			
			list->files[file_num] = file;
			return;
		};
	};
	
	// If the file is not already in the list, shift any file pointers in the list forward
	// by one pointer space. If the list already contains 15 pointers, free the last pointer
	// and overwrite it. Finally, allocate the memory for the added file and write its pointer
	// to the start of the file list.
	
	
	if (file_num == MAX_NUM_RECENT_FILES)
	{
		file_num--;
		free(list->files[file_num]);
	};
	
	while (file_num > 0)
	{
		list->files[file_num] = list->files[file_num - 1];
		file_num--;
	};
	
	list->files[0] = file;
	
	if (list->num_files < MAX_NUM_RECENT_FILES)
		list->num_files++;
	
	return;
}

static bool load_recent_files(recent_file_list_t *recent_file_list, file_list_t *main_file_list)
{
	ti_var_t recent_files_appvar;
	file_data_t **file_list;
	uint16_t list_file_num, num_list_files;
	uint8_t recent_file_num = 0;
	char recent_file_name[FILE_NAME_LEN + 1];
	uint8_t hexaedit_type;
	
	// All of the file data pointers in the recent_file_list are copies of main_file_list
	// pointers. So, when the main_file_list file pointers are freed, the recent_file_list
	// pointer copies will point to already freed memory, eliminating the need to free any
	// recent_file_list pointers. The recent_file_list struct itself, however, must be
	// freed normally.
	
	
	if ((recent_files_appvar = ti_Open(RECENT_FILES_APPVAR, "r")) == 0)
		return false;
	
	while (ti_Tell(recent_files_appvar) < ti_GetSize(recent_files_appvar) - 1)
	{
		memset(recent_file_name, '\0', FILE_NAME_LEN + 1);
		ti_Read(&recent_file_name, FILE_NAME_LEN, 1, recent_files_appvar);
		hexaedit_type = (uint8_t)ti_GetC(recent_files_appvar);
		
		// dbg_sprintf(dbgout, "recent_file_name = %s\n", recent_file_name);
		// dbg_sprintf(dbgout, "Flush buffer\n");
		
		if (hexaedit_type == HEXAEDIT_APPVAR_TYPE)
		{
			file_list = main_file_list->appvars;
			num_list_files = main_file_list->num_appvars;
		}
		else if (hexaedit_type == HEXAEDIT_ASM_PRGM_TYPE)
		{
			file_list = main_file_list->asm_programs;
			num_list_files = main_file_list->num_asm_programs;
		} else {
			file_list = main_file_list->basic_programs;
			num_list_files = main_file_list->num_basic_programs;
		};
		
		list_file_num = 0;
		
		while (list_file_num < num_list_files)
		{
			// dbg_sprintf(dbgout, "file_list = %s\n", file_list[list_file_num]->name);
			
			if (!strcmp(recent_file_name, file_list[list_file_num]->name))
			{
				recent_file_list->files[recent_file_num] = file_list[list_file_num];
				recent_file_num++;
				break;
			};
			
			list_file_num++;
		};
	};
	
	recent_file_list->num_files = recent_file_num;
	
	ti_Close(recent_files_appvar);
	return true;
}

static void save_recent_files(recent_file_list_t *recent_file_list)
{
	ti_var_t recent_files_appvar;
	uint16_t file_num = 0;
	
	if ((recent_files_appvar = ti_Open(RECENT_FILES_APPVAR, "w")) == 0)
	{
		gui_DrawMessageDialog_Blocking("Could not save Recent Files list");
		return;
	};
	
	while (file_num < recent_file_list->num_files)
	{
		ti_Write(&recent_file_list->files[file_num]->name, FILE_NAME_LEN, 1, recent_files_appvar);
		ti_PutC((char)(recent_file_list->files[file_num]->hexaedit_type), recent_files_appvar);
		
		file_num++;
	};
	
	return;
}

static bool create_recent_files_appvar(void)
{
	ti_var_t appvar;
	
	ti_CloseAll();
	appvar = ti_Open(RECENT_FILES_APPVAR, "w");
	if (!appvar) {
		return false;
	};
	
	NUM_FILES_PER_TYPE[RECENTS_TABLE_NUM] = 0;
	
	ti_Close(appvar);
	return true;
}

static file_data_t *set_selected_file(file_list_t *file_list, recent_file_list_t *recent_file_list, uint8_t table_num, uint8_t selected_file_offset)
{
	file_data_t *selected_file = NULL;
	
	if (table_num == RECENTS_TABLE_NUM && recent_file_list->num_files > 0)
	{
		selected_file = recent_file_list->files[selected_file_offset];
	}
	else if (table_num == APPVAR_TABLE_NUM && file_list->num_appvars > 0)
	{
		selected_file = file_list->appvars[selected_file_offset];
	}
	else if (table_num == ASM_PRGM_TABLE_NUM && file_list->num_asm_programs > 0)
	{
		selected_file = file_list->asm_programs[selected_file_offset];
	}
	else if (table_num == BASIC_PRGM_TABLE_NUM && file_list->num_basic_programs > 0)
	{
		selected_file = file_list->basic_programs[selected_file_offset];
	};
	return selected_file;
}

void menu_DrawTitleBar(void)
{
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(0, 0, LCD_WIDTH, 20);
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	gfx_PrintStringXY(PROGRAM_NAME, 5, 6);
	gfx_PrintStringXY(PROGRAM_VERSION, 9 + gfx_GetStringWidth(PROGRAM_NAME), 6);
	//gui_DrawBatteryStatus();
	return;
}

static void draw_menu_bar(uint8_t table_num)
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

	if (table_num != HEXAEDIT_RECENTS_TYPE)
		gfx_PrintStringXY("Search", 130, 226);
	gfx_PrintStringXY("Conf", 224, 226);
	gfx_PrintStringXY("Exit", 286, 226);
	
	return;
}

static void draw_file_list_header(uint8_t y, uint8_t table_num)
{
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
			gfx_FillRectangle_NoClip(x - 3, y, gfx_GetStringWidth(TABLE_NAMES[i]) + 4, 12);
			gfx_SetTextFGColor(color_theme.table_selector_color);
		};
		gfx_PrintStringXY(TABLE_NAMES[i], x, y + 2);
		x += gfx_GetStringWidth(TABLE_NAMES[i]) + 15;
	};
	return;
}

static void draw_table_header(uint8_t y)
{
	uint24_t x = TABLE_LEFT_MARGIN + 1;
	uint8_t col;
	
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(TABLE_LEFT_MARGIN - 2, y, TABLE_WIDTH + 4, 16);
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	
	y += 4;
	
	for (col = 0; col < TABLE_NUM_COLS; col++)
	{
		gfx_PrintStringXY(COLUMN_HEADERS[col], x, y);
		x += COLUMN_WIDTHS[col];
	};
	return;
}

static void print_file_data(file_data_t *file_data, uint8_t text_fg_color, uint8_t yPos)
{
	char value[7] = {'\0'};
	
	gfx_SetTextXY(COL_1_X + 1, yPos);
	gui_PrintFileName(file_data->name, text_fg_color);
	
	if (file_data->is_archived)
		gfx_PrintStringXY("*", COL_2_X, yPos);

	if (file_data->is_protected)
		gfx_PrintStringXY("*", COL_3_X, yPos);

	cutil_PtrSprintf(value, file_data->vat_ptr);
	gfx_PrintStringXY(value, COL_4_X, yPos);
	
	cutil_PtrSprintf(value, file_data->data_ptr);
	gfx_PrintStringXY(value, COL_5_X, yPos);
	
	sprintf(value, "%d", file_data->size);
	gfx_PrintStringXY(value, COL_6_X + COLUMN_WIDTHS[5] - gfx_GetStringWidth(value), yPos);
	return;
}

static void print_table_entry(file_data_t *file_data, bool print_inverted, uint8_t yPos)
{
	uint8_t text_fg_color;
	
	gfx_SetTextBGColor(color_theme.table_bg_color);
	text_fg_color = color_theme.table_text_color;
	gfx_SetTextTransparentColor(color_theme.table_bg_color);
	
	if (print_inverted)
	{
		gfx_SetTextBGColor(color_theme.table_selector_color);
		text_fg_color = color_theme.selected_table_text_color;
		gfx_SetTextTransparentColor(color_theme.table_selector_color);
		gfx_SetColor(color_theme.table_selector_color);
		gfx_FillRectangle_NoClip(TABLE_LEFT_MARGIN, yPos - 1, TABLE_WIDTH, TABLE_ROW_HEIGHT - 2);
	};
	
	gfx_SetTextFGColor(text_fg_color);
	print_file_data(file_data, text_fg_color, yPos);
	return;
}

static void erase_table(void)
{
	gfx_SetColor(BLACK);
	gfx_FillRectangle_NoClip(TABLE_LEFT_MARGIN - 2, TABLE_FIRST_ROW_Y - 3, TABLE_WIDTH + 4, NUM_FILES_ONSCREEN * TABLE_ROW_HEIGHT + 2);
	gfx_SetColor(color_theme.table_bg_color);
	gfx_FillRectangle_NoClip(TABLE_LEFT_MARGIN - 1, TABLE_FIRST_ROW_Y - 2, TABLE_WIDTH + 2, NUM_FILES_ONSCREEN * TABLE_ROW_HEIGHT);
	return;
}

static void print_empty_table_message(void)
{
	const char *empty_table_message = "-- No files found --";
	
	gfx_SetTextBGColor(color_theme.table_bg_color);
	gfx_SetTextFGColor(color_theme.background_color);
	gfx_SetTextTransparentColor(color_theme.table_bg_color);
	gfx_PrintStringXY(empty_table_message, (LCD_WIDTH / 2) - gfx_GetStringWidth(empty_table_message) / 2, TABLE_FIRST_ROW_Y);
	return;
}

static void print_recent_files_table(recent_file_list_t *recent_file_list, uint8_t selected_file_offset)
{
	uint8_t file_num = 0;
	bool print_inverted;
	uint8_t y;
	
	for (;;)
	{
		// dbg_sprintf(dbgout, "num_files = %d\n", recent_file_list->num_files);
		
		// Since the maximum number of recent files can never exceed one table, a list offset
		// and NUM_FILES_ONSCREEN test is unnecessary, unlike print_main_files_table().
		if (file_num == recent_file_list->num_files)
			break;
		
		y = TABLE_FIRST_ROW_Y + (TABLE_ROW_HEIGHT * file_num);
		print_inverted = false;
		
		if (file_num == selected_file_offset)
			print_inverted = true;
		
		print_table_entry(recent_file_list->files[file_num], print_inverted, y);
		file_num++;
	};
	
	if (file_num == 0)
		print_empty_table_message();
	
	return;
}

static void print_main_files_table(file_list_t *file_list, uint8_t table_num, uint8_t list_offset, uint8_t selected_file_offset)
{
	uint8_t num_files;
	uint8_t file_num = 0;
	uint8_t num_files_printed = 0;
	bool print_inverted;
	uint8_t y;
	
	if (table_num == APPVAR_TABLE_NUM)
	{
		num_files = file_list->num_appvars;
	}
	else if (table_num == ASM_PRGM_TABLE_NUM)
	{
		num_files = file_list->num_asm_programs;
	}
	else if (table_num == BASIC_PRGM_TABLE_NUM)
	{
		num_files = file_list->num_basic_programs;
	} else {
		return;
	};
	
	while (file_num < list_offset)
		file_num++;
	
	for (;;)
	{
		if (num_files_printed == NUM_FILES_ONSCREEN)
			break;
		
		if (file_num == num_files)
			break;
		
		y = TABLE_FIRST_ROW_Y + (TABLE_ROW_HEIGHT * num_files_printed);
		
		print_inverted = false;
		if (file_num == selected_file_offset)
			print_inverted = true;
		
		if (table_num == APPVAR_TABLE_NUM)
		{
			print_table_entry(file_list->appvars[file_num], print_inverted, y);
		}
		else if (table_num == ASM_PRGM_TABLE_NUM)
		{
			print_table_entry(file_list->asm_programs[file_num], print_inverted, y);
		}
		else if (table_num == BASIC_PRGM_TABLE_NUM)
		{
			print_table_entry(file_list->basic_programs[file_num], print_inverted, y);
		};
		
		num_files_printed++;
		file_num++;
	};
	
	if (num_files_printed == 0)
		print_empty_table_message();
	
	return;
}

static void search_main_files(file_list_t *file_list, uint8_t *table_num, uint8_t *file_num)
{
	char buffer[9] = {'\0'};
	uint8_t buffer_size = 8;
	const char *keymaps[3] = {UPPERCASE_LETTERS, LOWERCASE_LETTERS, NUMBERS};
	const char keymap_indicators[3] = {'A', 'a', '0'};
	uint8_t keymap_num = 0;
	int8_t key;
	
	uint8_t file;
	uint8_t curr_table_num = *table_num;
	
	
	for (;;)
	{
		gui_DrawInputPrompt("Search Files:", 102);
		gui_DrawKeymapIndicator(keymap_indicators[keymap_num], 201, 223);
		gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
		
		gfx_SetTextBGColor(color_theme.table_bg_color);
		gfx_SetTextFGColor(color_theme.table_text_color);
		gfx_SetTextTransparentColor(color_theme.table_bg_color);
		
		key = gui_Input(buffer, buffer_size, 98, 224, 99, keymaps[keymap_num]);
		
		if (key == sk_Alpha)
		{
			if (keymap_num < 2)
			{
				keymap_num++;
			} else {
				keymap_num = 0;
			};
		};
		
		if (key == sk_Clear || key == sk_2nd || key == sk_Enter)
			break;
		
		delay(200);
	};
	
	if (*buffer == '\0')
		return;
	
	for (;;)
	{
		file = 0;
		
		for (;;)
		{
			if (curr_table_num == APPVAR_TABLE_NUM)
			{
				// dbg_sprintf(dbgout, "Searching appvars...\n");
				
				if (file == file_list->num_appvars)
					break;
				
				if (!strcmp(buffer, file_list->appvars[file]->name))
					goto RETURN_SUCCESS;
			}
			else if (curr_table_num == ASM_PRGM_TABLE_NUM)
			{
				// dbg_sprintf(dbgout, "Search asm programs...\n");
				
				if (file == file_list->num_asm_programs)
					break;
				
				if (!strcmp(buffer, file_list->asm_programs[file]->name))
					goto RETURN_SUCCESS;
			}
			else if (curr_table_num == BASIC_PRGM_TABLE_NUM)
			{
				
				// dbg_sprintf(dbgout, "Searching basic programs...\n");
				
				if (file == file_list->num_basic_programs)
					break;
				
				if (!strcmp(buffer, file_list->basic_programs[file]->name))
					goto RETURN_SUCCESS;
			};
			
			file++;
		};
		
		// Loop through the last three file tables (Appvars, ASM Programs, and BASIC Programs).
		// If we have looped through all of them, print error.
		
		curr_table_num++;
		if (curr_table_num == BASIC_PRGM_TABLE_NUM + 1)
			curr_table_num = APPVAR_TABLE_NUM;
		
		if (curr_table_num == *table_num)
			break;
	};
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	gfx_PrintStringXY("File not found!", 5, 226);
	gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	delay(500);
	while (!os_GetCSC());
	return;
	
	RETURN_SUCCESS:
	*table_num = curr_table_num;
	*file_num = file;
	return;
}

static void change_table_num(uint8_t *table_num, uint8_t *selected_file_offset, int8_t key)
{
  uint8_t local_table_num = *table_num;
  
	if (key == sk_Left && *table_num > 0)
		local_table_num--;
	
	if (key == sk_Right && *table_num < NUM_TABLES - 1)
		local_table_num++;
	
	if (key == sk_1)
		local_table_num = 0;
	if (key == sk_2)
		local_table_num = 1;
	if (key == sk_3)
		local_table_num = 2;
	if (key == sk_4)
		local_table_num = 3;
	
	if (local_table_num != *table_num)
	{
		*selected_file_offset = 0;
	};
	
  *table_num = local_table_num;
	
	return;
};

static void change_selected_file_offset(uint8_t *table_num, uint8_t *selected_file_offset)
{
	if (kb_Data[7] & kb_Up)
	{
		if (kb_Data[2] & kb_Alpha)
		{
			if (*selected_file_offset > NUM_FILES_ONSCREEN - 1)
			{
				*selected_file_offset -= (NUM_FILES_ONSCREEN - 1);
			} else {
				*selected_file_offset = 0;
			};
		}
		else if (*selected_file_offset > 0)
		{
			(*selected_file_offset)--;
		};
	};
	
	if (kb_Data[7] & kb_Down)
	{
		if (kb_Data[2] & kb_Alpha)
		{
			if ((*selected_file_offset + NUM_FILES_ONSCREEN - 1) < NUM_FILES_PER_TYPE[*table_num])
			{
				*selected_file_offset += (NUM_FILES_ONSCREEN - 1);
			} else {
				*selected_file_offset = NUM_FILES_PER_TYPE[*table_num] - 1;
			};
		}
		else if (*selected_file_offset < NUM_FILES_PER_TYPE[*table_num] - 1)
		{
			(*selected_file_offset)++;
		};
	};
	
	return;
}

static void update_selected_file_size(file_data_t *selected_file)
{
	ti_var_t slot;
	
	if (selected_file->hexaedit_type == HEXAEDIT_APPVAR_TYPE)
	{
		slot = ti_Open(selected_file->name, "r");
	}
	else if (selected_file->is_protected)
	{
		slot = ti_OpenVar(selected_file->name, "r", TI_PPRGM_TYPE);
	} else {
		slot = ti_OpenVar(selected_file->name, "r", TI_PRGM_TYPE);
	};
	
	if (slot == 0)
		return;
	
	selected_file->size = ti_GetSize(slot);
	ti_Close(slot);
	return;
}

void main_menu(void)
{
	file_list_t *main_file_list;
	recent_file_list_t *recent_file_list;
	file_data_t *selected_file;
	
	uint8_t table_num = RECENTS_TABLE_NUM;
	uint8_t list_offset = 0;
	uint8_t selected_file_offset = 0;
	int8_t key;
	
	bool redraw_background = true;
	
	gfx_FillScreen(WHITE);
	gui_DrawMessageDialog("Loading files...");
	
	if ((main_file_list = malloc(sizeof(file_list_t))) == NULL)
		return;
	
	load_main_files(main_file_list);
	NUM_FILES_PER_TYPE[APPVAR_TABLE_NUM] = main_file_list->num_appvars;
	NUM_FILES_PER_TYPE[ASM_PRGM_TABLE_NUM] = main_file_list->num_asm_programs;
	NUM_FILES_PER_TYPE[BASIC_PRGM_TABLE_NUM] = main_file_list->num_basic_programs;
	
// dbg_sprintf(dbgout, "main_file_list = 0x%6x | size = %d\n", main_file_list, (uint24_t)sizeof(file_list_t));
	
	if (ti_Open(RECENT_FILES_APPVAR, "r") == 0)
	{
		create_recent_files_appvar();
	};
	
	if ((recent_file_list = malloc(sizeof(recent_file_list_t))) == NULL)
	{
		free(main_file_list);
		return;
	};
	
	if (!load_recent_files(recent_file_list, main_file_list))
	{
		gui_DrawMessageDialog_Blocking("Could not load Recent Files");
		free(recent_file_list);
		free(main_file_list);
		return;
	};
	
//dbg_sprintf(dbgout, "recent_file_list = 0x%6x | size = %d\n", recent_file_list, (uint24_t)sizeof(recent_file_list_t));
	
	NUM_FILES_PER_TYPE[RECENTS_TABLE_NUM] = recent_file_list->num_files;
	
	for (;;)
	{
		while (selected_file_offset < list_offset && list_offset > 0)
			list_offset--;
		
		while (selected_file_offset >= list_offset + NUM_FILES_ONSCREEN)
			list_offset++;
	
		selected_file = set_selected_file(main_file_list, recent_file_list, table_num, selected_file_offset);
		
		if (redraw_background)
		{
			gfx_SetColor(LT_GRAY);
			gfx_FillRectangle_NoClip(0, 20, LCD_WIDTH, LCD_HEIGHT - 40);
			menu_DrawTitleBar();
			draw_menu_bar(table_num);
			draw_table_header(35);
      draw_file_list_header(21, table_num);
		};
		
		// draw_file_list_header(21, table_num);
		erase_table();
		
		if (table_num == RECENTS_TABLE_NUM)
		{
			print_recent_files_table(recent_file_list, selected_file_offset);
		} else {
			print_main_files_table(main_file_list, table_num, list_offset, selected_file_offset);
      
      // If only a few file entries are drawn, the editor will respond very
      // quickly to keypresses, making it hard to manage. Introduce some
      // delay after short file list rendering.
      // NOTE: This is inefficient and bloats the main function. Strongly
      // advise rewriting main_menu() for efficiency improvements.
      if ((table_num == APPVAR_TABLE_NUM) && (main_file_list->num_appvars < NUM_FILES_ONSCREEN))
        delay(100);
      else if ((table_num == ASM_PRGM_TABLE_NUM) && (main_file_list->num_asm_programs < NUM_FILES_ONSCREEN))
        delay(100);
      if ((table_num == BASIC_PRGM_TABLE_NUM) && (main_file_list->num_basic_programs < NUM_FILES_ONSCREEN))
        delay(100);
		};
		
		gfx_BlitBuffer();
		
		do {
			kb_Scan();
			//gui_DrawTime(200);
		} while ((key = asm_GetCSC()) == -1);
		
		change_selected_file_offset(&table_num, &selected_file_offset);
		change_table_num(&table_num, &selected_file_offset, key);
		
		if ((key == sk_2nd || key == sk_Enter) && selected_file != NULL)
		{
			gui_DrawMessageDialog("Copying file...");
			
			if (selected_file->is_protected)
			{
				editor_FileEditor(selected_file->name, TI_PPRGM_TYPE, 0, 0);
			}
			else if (selected_file->hexaedit_type == HEXAEDIT_APPVAR_TYPE)
			{
				editor_FileEditor(selected_file->name, TI_APPVAR_TYPE, 0, 0);
			} else {
				editor_FileEditor(selected_file->name, TI_PRGM_TYPE, 0, 0);
			};
			
			update_selected_file_size(selected_file);
			add_recent_file(recent_file_list, selected_file);
			NUM_FILES_PER_TYPE[RECENTS_TABLE_NUM] = recent_file_list->num_files;
			
			// If file edited was Recent File, move selected_file_offset to top of list
			if (table_num == RECENTS_TABLE_NUM)
				selected_file_offset = 0;
			
			redraw_background = true;
      
      // Prevent keypress fall-throughs.
      delay(100);
		};
		
		if (key == sk_Yequ)
		{
			editor_RAMEditor(0, 0);
			redraw_background = true;
      
      // Prevent keypress fall-throughs.
      delay(100);
		};
		
		if (key == sk_Window)
		{
			editor_ROMViewer(0, 0);
			redraw_background = true;
      
      // Prevent keypress fall-throughs.
      delay(100);
		};
    
    if (key == sk_Mode)
    {
      // Tell the user what is happening since the RAM editor takes awhile to
      // open.
      gui_DrawMessageDialog("Opening editor at file VAT pointer");
      delay(100);
      
      editor_RAMEditor(selected_file->vat_ptr - RAM_MIN_ADDRESS, selected_file->vat_ptr - RAM_MIN_ADDRESS);
      redraw_background = true;
      
      // Prevent keypress fall-throughs.
      delay(100);
    };
    
    // Temporary arrangement for testing purposes.
    if (key == sk_GraphVar)
    {
      editor_PortsEditor(0, 0);
      redraw_background = true;
      
      // Prevent keypress fall-throughs.
      delay(100);
    };
		
		if (key == sk_Zoom && TABLE_ORDER[table_num] != HEXAEDIT_RECENTS_TYPE)
		{
			search_main_files(main_file_list, &table_num, &selected_file_offset);
			redraw_background = true;
		};
		
		if (key == sk_Trace)
			settings_Settings();
		
		if (key == sk_Graph || key == sk_Clear)
		{
			break;
		};
	};
  
	// The Recent Files must be saved before the main_file_list is freed.
	save_recent_files(recent_file_list);
	free(recent_file_list);
	free_main_files(main_file_list);
	return;
}
