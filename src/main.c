
/*-----------------------------------------
 * Program Name: HexaEdit CE
 * Version:      2.0.1 CE
 * Author:       Captain Calc   
 * Description:  A hex editor for the TI-84
 *               Plus CE.
 *-----------------------------------------
*/

#include "asmutil.h"
#include "colors.h"
#include "editor.h"
#include "gui.h"
#include "menu.h"

#include <string.h>

#include <tice.h>
#include <graphx.h>
#include <fileioc.h>
#include <keypadc.h>

// Debugging
#include "debug.h"

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

static bool check_for_headless_start(void)
{
	ti_var_t config_data_slot;
	
	if ((config_data_slot = ti_Open(HS_CONFIG_APPVAR, "r")) != 0)
	{
		editor_HeadlessStart();
		return true;
	};
	return false;
}

int main(void)
{
	gfx_Begin();
	gfx_SetDrawBuffer();
	ti_CloseAll();
	
	load_default_color_theme();
	
	if (check_for_headless_start())
	{
		gfx_End();
		return 0;
	};
	
	main_menu();
	
	gfx_End();
	return 0;
}