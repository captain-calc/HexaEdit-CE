#include "colors.h"
#include "editor.h"
#include "editor_gui.h"
#include "gui.h"

#include <graphx.h>

#include <math.h>
#include <stdint.h>

static void _draw_editing_size(editor_t *editor)
{
	uint8_t magnitude = 6;
	
	if (editor->type == ROM_VIEWER)
	{
		magnitude = 7;
	};
	
	gfx_SetTextXY(100, 6);
	if (editor->type == FILE_EDITOR && editor->is_file_empty)
	{
		gfx_PrintUInt(0, 6);
	}
	else
	{
		gfx_PrintUInt(editor->max_address - editor->min_address + 1, magnitude);
	};
	gfx_PrintString(" B");
	return;
}

void editorgui_DrawTopBar(editor_t *editor)
{
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, 0, 320, 20);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
	
	_draw_editing_size(editor);

	gfx_SetTextXY(5, 6);
	
	if (editor->num_changes > 0)
	{
		gfx_PrintString("* ");
	};
	gui_PrintFileName(editor->name);
	
	gui_DrawBatteryStatus();
	return;
}

void editorgui_DrawToolBar(editor_t *editor)
{
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
	gfx_PrintStringXY("Goto", 5, 226);
	if (editor->type == FILE_EDITOR)
	{
		gfx_PrintStringXY("Ins", 70, 226);
	};
	if (editor->num_changes > 0)
	{
		gfx_PrintStringXY("Undo", 226, 226);
	};
	gfx_PrintStringXY("Exit", 286, 226);
	return;
}

void editorgui_DrawAltToolBar(cursor_t *cursor)
{
	uint8_t i;
	uint24_t byte_value = 0;
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(0, 220, 140, 20);
	gfx_FillRectangle_NoClip(226, 220, 50, 20);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
	
	if ((cursor->primary - cursor->secondary) < 3)
	{
		gfx_PrintStringXY("DEC:", 5, 226);
		gfx_SetTextXY(40, 226);
		
		for (i = 0; i < (cursor->primary - cursor->secondary + 1); i++)
		{
			byte_value += (*(cursor->secondary + i) << (8 * i));
		};
		
		gfx_PrintUInt(byte_value, log10((double)byte_value + 10));
	};
	return;
}