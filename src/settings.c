#include "asmutil.h"
#include "colors.h"
#include "editor.h"
#include "gui.h"
#include "menu.h"

#include <graphx.h>
#include <keypadc.h>
#include <tice.h>

#include <stdint.h>


static const uint24_t search_range_opts[] = {
	QUICK_SEARCH,
	MODERATE_SEARCH,
	THOROUGH_SEARCH
};

static void draw_navbar(void)
{
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	
	gfx_PrintStringXY("Back", 286, 226);
	return;
}

static void draw_phrase_search_settings(uint24_t search_range, uint8_t select_pos)
{
	const char *search_range_opt_msgs[] = {
		"Quick Search ( ~10 sec )",
		"Range:  All RAM and all file sizes",
		"Moderate Search ( ~20 sec)",
		"Range:  One-quarter of ROM",
		"Thorough Search ( ~90 sec )",
		"Range:  All ROM",
		"External Setting",
		"Range (bytes):"
	};
	uint8_t num_messages = 7;
	uint8_t message;
	uint8_t yPos;
	
	
	gfx_SetTextBGColor(color_theme.background_color);
	gfx_SetTextFGColor(color_theme.table_text_color);
	gfx_SetTextTransparentColor(color_theme.background_color);
	gfx_PrintStringXY("Phrase Search Settings:", 10, 25);
	
	yPos = 45;
	message = 0;
	
	while (message < num_messages)
	{	
		gfx_SetColor(color_theme.bar_color);
		
		if (select_pos == (message / 2))
		{
			gfx_SetColor(color_theme.cursor_color);
		};
		
		if (search_range_opts[message / 2] == search_range)
		{
			gfx_FillRectangle_NoClip(11, yPos + 1, 7, 7);
		};
		
		gfx_Rectangle_NoClip(10, yPos, 9, 9);
		
		gfx_SetTextXY(30, yPos + 1);
		gfx_PrintString(search_range_opt_msgs[message++]);
		yPos += 10;
		gfx_SetTextXY(46, yPos + 1);
		gfx_PrintString(search_range_opt_msgs[message++]);
		yPos += 15;
	};
	
	if (
		search_range != QUICK_SEARCH &&
		search_range != MODERATE_SEARCH &&
		search_range != THOROUGH_SEARCH
	)
	{
		gfx_SetColor(color_theme.bar_color);
		gfx_FillRectangle_NoClip(11, yPos - 24, 7, 7);
		gfx_SetTextXY(54 + gfx_GetStringWidth(search_range_opt_msgs[--message]), yPos - 14);
		gfx_PrintUInt(search_range, 8);
	};
	
	return;
}

uint24_t settings_GetPhraseSearchRange(void)
{
	ti_var_t slot;
	uint24_t search_range;
	
	if ((slot = ti_Open(HEXAEDIT_CONFIG_APPVAR, "r")) != 0)
	{
		ti_Read(&search_range, sizeof(uint24_t), 1, slot);
		ti_Close(slot);
		return search_range;
	} else {
		gui_DrawMessageDialog_Blocking("Failed to retrieve search range");
		return 0;
	}
}

void settings_Settings(void)
{
	ti_var_t slot;
	uint24_t search_range;
	uint8_t select_pos = 0;
	int8_t key;
	
	search_range = settings_GetPhraseSearchRange();
	
	if (search_range == 0)
		search_range = QUICK_SEARCH;
	
	for (;;)
	{
		gfx_FillScreen(color_theme.background_color);
		menu_DrawTitleBar();
		draw_navbar();
		draw_phrase_search_settings(search_range, select_pos);
		gfx_BlitBuffer();
		
		do {
			kb_Scan();
		} while ((key = asm_GetCSC()) == -1);
		
		if (key == sk_Up && select_pos > 0)
			select_pos--;
		if (key == sk_Down && select_pos < 2)
			select_pos++;
		
		if (key == sk_2nd || key == sk_Enter)
		{
			search_range = search_range_opts[select_pos];
		};
		
		if (key == sk_Clear || key == sk_Graph)
			break;
		
		delay(100);
	};
	
	if ((slot = ti_Open(HEXAEDIT_CONFIG_APPVAR, "w")) == 0)
	{
		gui_DrawMessageDialog_Blocking("Failed to write new search range");
		return;
	};
	
	ti_Write(&search_range, sizeof(uint24_t), 1, slot);
	
	ti_Close(slot);
	return;
}