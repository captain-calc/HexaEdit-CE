// Name:        Captain Calc
// Date:        July 12, 2021
// File:        main.c
// Description: Loads default color theme, checks for Headless Start, and runs
//              main menu.

// PROGRAM INFO
//
// Version:     See PROGRAM_VERSION in menu.h
// Description: A hex editor for the TI-84 Plus CE.


#include "colors.h"
#include "editor.h"
#include "gui.h"
#include "menu.h"
#include "settings.h"

#include <string.h>

#include <tice.h>
#include <graphx.h>
#include <fileioc.h>

#include <debug.h>


// Global structure for color theme configuration
color_theme_config_t color_theme;


// Function Prototypes
static void load_default_color_theme(void);
static bool headless_start(void);

/*
While the editor is being rewritten, disable the Headless Start.


int main(void)
{
	gfx_Begin();
	gfx_SetDrawBuffer();
	
  // Load default color theme BEFORE checking for headless start in case the
  // headless start does not specify a custom color theme.
	load_default_color_theme();
	
  // Always create the settings appvar with the default phrase search range
  // in case another program modified it.
	if (settings_InitSettingsAppvar())
  {
    if (!headless_start())
		  main_menu();
  }
  else
  {
    gui_DrawMessageDialog_Blocking("Could not create settings appvar. Exiting...");
  };
	
	gfx_End();
	return 0;
}
*/


int main(void)
{
	gfx_Begin();
	gfx_SetDrawBuffer();
	
  // Load default color theme BEFORE checking for headless start in case the
  // headless start does not specify a custom color theme.
	load_default_color_theme();
	
	if (settings_InitSettingsAppvar())
		main_menu();
  else
    gui_DrawMessageDialog_Blocking("Could not create settings appvar. Exiting...");
	
	gfx_End();
	return 0;
}


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


static bool headless_start(void)
{
	ti_var_t slot;
  char hs_config_ans_flag[HS_CONFIG_ANS_FLAG_LEN];
  bool start_headless = false;
	
  ti_CloseAll();
  
	if ((slot = ti_OpenVar(ti_Ans, "r", TI_STRING_TYPE)))
	{
    ti_Read(&hs_config_ans_flag, HS_CONFIG_ANS_FLAG_LEN, 1, slot);
    
    if (!strncmp(hs_config_ans_flag, HS_CONFIG_ANS_FLAG, HS_CONFIG_ANS_FLAG_LEN))
    {
dbg_sprintf(dbgout, "hs_config_ans_flag = %s\n", hs_config_ans_flag);
      editor_HeadlessStart();
      start_headless = true;
    };

    ti_Close(slot);
	};
  
	return start_headless;
}