#include "asmutil.h"
#include "colors.h"
#include "gui.h"
#include "menu.h"
#include "settings.h"

#include <graphx.h>
#include <keypadc.h>
#include <tice.h>

#include <stdint.h>


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


bool settings_InitSettingsAppvar(void)
{
	ti_var_t slot;
	uint24_t search_range = ROM_SEARCH_RANGE;
	
	if (!(slot = ti_Open(HEXA_SETTINGS_APPVAR, "w")))
		return false;
	
	ti_Write(&search_range, sizeof(uint24_t), 1, slot);
	ti_Close(slot);
	return true;
}


uint24_t settings_GetPhraseSearchRange(void)
{
	ti_var_t slot;
	uint24_t search_range;
	
	if ((slot = ti_Open(HEXA_SETTINGS_APPVAR, "r")))
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
  // An uint24_t can have up to 8 digits.
  uint8_t SEARCH_RANGE_NUM_DIGITS = 8;
  
	int8_t key;
	
  gfx_FillScreen(color_theme.background_color);
  menu_DrawTitleBar();
  draw_navbar();
  gfx_SetTextBGColor(color_theme.background_color);
  gfx_SetTextFGColor(color_theme.table_text_color);
  gfx_SetTextTransparentColor(color_theme.background_color);
  gfx_PrintStringXY("Phrase Search Range (bytes):   ", 10, 25);
  gfx_PrintUInt(settings_GetPhraseSearchRange(), SEARCH_RANGE_NUM_DIGITS);
  gfx_BlitBuffer();
  
	for (;;)
	{
		do {
			kb_Scan();
		} while ((key = asm_GetCSC()) == -1);
		
		if (key == sk_Clear || key == sk_Graph)
			break;
		
		delay(100);
	};
  
	return;
}