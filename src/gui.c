#include <debug.h>

#include "asmutil.h"
#include "colors.h"
#include "editor.h"
#include "gui.h"

#include <graphx.h>
#include <keypadc.h>
#include <tice.h>

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>


static uint24_t get_string_width(char *string)
{
	char *character = string;
	uint24_t width = 0;
	
	while (*character != '\0')
	{
		if (*character == 0x5b)
		{
			width += gfx_GetCharWidth('O');
		} else {
			width += gfx_GetCharWidth(*character);
		};
		character++;
	};
	return width;
}

void gui_PrintText(const char *name, uint8_t text_color)
{
	const char *character = name;
	
	gfx_SetColor(text_color);

	while (*character != '\0') {
		if (*character == 0x5b) {
			gfx_PrintChar('O');
			gfx_HorizLine(gfx_GetTextX() - 6, gfx_GetTextY() + 3, 3);
		} else {
			gfx_PrintChar(*character);
		};
		character++;
	};

	return;
}

void gui_PrintFileName(const char *name, uint8_t text_color)
{
	// If the file is hidden, print a tilde for the first character
	// of the name
	
	if (*name < 0x20)
	{
		gfx_PrintChar('~');
		name++;
	};
	
	gui_PrintText(name, text_color);
	return;
}

void gui_DrawMessageDialog(const char *message)
{
	uint24_t message_width = gfx_GetStringWidth(message);
	
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(150 - message_width / 2, 100, message_width + 20, 40);
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	gfx_PrintStringXY(message, 160 - gfx_GetStringWidth(message) / 2, 117);
	gfx_BlitBuffer();
	return;
}

void gui_DrawMessageDialog_Blocking(const char *message)
{
	gui_DrawMessageDialog(message);
	delay(200);
	while (!os_GetCSC());
	return;
}

void gui_DrawBatteryStatus(void)
{
	uint8_t status;
	uint8_t colors[4] = {RED, RED, YELLOW, GREEN};
	char status_str[5] = {'\0'};
	
	status = boot_GetBatteryStatus();
	gfx_SetColor(BLACK);
	gfx_HorizLine_NoClip(310, 4, 4);
	gfx_FillRectangle_NoClip(308, 5, 8, 10);
	gfx_SetColor(WHITE);
	gfx_FillRectangle_NoClip(309, 6, 6, 8);
	gfx_SetColor(colors[status]);
	gfx_FillRectangle_NoClip(309, 14 - (2 * status), 6, 2 * status);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	sprintf(status_str, "%d", 25 * status);
	if (status == 4) {
		*(status_str + 3) = '%';
	} else {
		*(status_str + 2) = '%';
	};
	gfx_PrintStringXY(status_str, 306 - gfx_GetStringWidth(status_str), 6);
	return;
}

void gui_DrawTime(uint24_t xPos)
{
	uint8_t seconds = 0, minutes = 0, hour = 0;
	const char *time_ind[2] = {"AM", "PM"};
	
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(xPos, 6, 55, 7);
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	
	boot_GetTime(&seconds, &minutes, &hour);
	gfx_SetTextXY(xPos, 6);
	if (hour == 0) hour = 12;
	if (hour > 12) hour -= 12;
	gfx_PrintUInt((unsigned int)hour, 2);
	gfx_PrintString(":");
	gfx_PrintUInt((unsigned int)minutes, 2);
	
	gfx_PrintStringXY(time_ind[(uint8_t)boot_IsAfterNoon()], xPos + 38, 6);
	gfx_BlitRectangle(1, xPos, 6, 55, 7);
	return;
}

void gui_DrawInputPrompt(const char *prompt, uint24_t input_field_width)
{
	uint24_t x = gfx_GetStringWidth(prompt) + 10;
	
	gfx_SetColor(color_theme.bar_color);
	gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
	
	gfx_SetTextBGColor(color_theme.bar_color);
	gfx_SetTextFGColor(color_theme.bar_text_color);
	gfx_SetTextTransparentColor(color_theme.bar_color);
	
	gfx_PrintStringXY(prompt, 5, 226);
	
	gfx_SetColor(color_theme.table_text_color);
	gfx_FillRectangle_NoClip(x, 223, input_field_width, FONT_HEIGHT + 6);
	
	gfx_SetColor(color_theme.table_bg_color);
	gfx_FillRectangle_NoClip(x + 1, 224, input_field_width - 2, FONT_HEIGHT + 4);
	
	return;
}

void gui_DrawKeymapIndicator(const char indicator, uint24_t xPos, uint8_t yPos)
{
	gfx_SetColor(color_theme.table_selector_color);
	gfx_FillRectangle_NoClip(xPos, yPos, gfx_GetCharWidth(indicator) + 3, FONT_HEIGHT + 6);
	
	gfx_SetTextBGColor(color_theme.table_selector_color);
	gfx_SetTextFGColor(color_theme.selected_table_text_color);
	gfx_SetTextTransparentColor(color_theme.table_selector_color);
	gfx_SetTextXY(xPos + 2, yPos + 3);
	gfx_PrintChar(indicator);
	return;
}

/* buffer_size excludes the null terminator. */
int8_t gui_Input(char buffer[], uint8_t buffer_size, uint24_t xPos, uint8_t yPos, uint24_t width, const char keymap[])
{
	int8_t key;
	uint8_t i = 0;
	uint8_t offset = strlen(buffer);
	
	gfx_SetTextXY(xPos + 2, yPos + 2);
	gui_PrintText(buffer, color_theme.table_text_color);
	
	gfx_BlitRectangle(1, xPos, yPos, width, FONT_HEIGHT + 4);
	
	do {
		kb_Scan();
		
		gfx_SetColor(color_theme.table_bg_color);
		
		if (i < 120)
		{
			gfx_SetColor(color_theme.table_text_color);
		};
		
		gfx_FillRectangle_NoClip(xPos + get_string_width(buffer) + 2, yPos + 1, 2, FONT_HEIGHT + 2);
		gfx_BlitRectangle(1, xPos + get_string_width(buffer) + 2, yPos + 1, 2, FONT_HEIGHT + 2);
		
		if (i++ > 240)
			i = 0;
		
	} while ((key = asm_GetCSC()) == -1);
	
	// dbg_sprintf(dbgout, "input key = %d\n", key);
	
	if (keymap[key] != '\0' && offset < buffer_size)
	{
		buffer[offset++] = keymap[key];
	};
	
	if (key == sk_Del && offset > 0)
	{
		buffer[--offset] = '\0';
	};
	
	if (key == sk_Clear)
		memset(buffer, '\0', buffer_size);
	
	return key;
}