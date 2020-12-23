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


void gui_PrintFileName(char *name) {

	char *character = name;
	

	while (*character != '\0') {
		if (*character == 0x5B) {
			gfx_PrintChar('O');
			gfx_HorizLine(gfx_GetTextX() - 6, gfx_GetTextY() + 3, 3);
		} else {
			gfx_PrintChar(*character);
		};
		character++;
	};

	return;
}

void gui_DrawMessageDialog(const char *message) {
	
	uint24_t message_width = gfx_GetStringWidth(message);
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(150 - message_width / 2, 100, message_width + 20, 40);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
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

void gui_DrawBatteryStatus(void) {
	
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
	gfx_SetTextFGColor(WHITE);
	sprintf(status_str, "%d", 25 * status);
	if (status == 4) {
		*(status_str + 3) = '%';
	} else {
		*(status_str + 2) = '%';
	};
	gfx_PrintStringXY(status_str, 306 - gfx_GetStringWidth(status_str), 6);
	return;
}

void gui_DrawTime(uint24_t xPos) {
	
	uint8_t seconds = 0, minutes = 0, hour = 0;
	const char *time_ind[2] = {"AM", "PM"};
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(xPos, 6, 55, 7);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
	
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

/* buffer_size excludes the null terminator. */
char *gui_Input(char buffer[], uint8_t buffer_size, char *keymaps[], uint8_t keymap_num, uint8_t num_keymaps, uint24_t x, uint8_t y, uint24_t width, uint8_t height)
{
	uint8_t i = 0;
	int8_t key;
	char *keymap = keymaps[keymap_num];
	char keymap_indicator;
	
	memset(buffer, '\0', buffer_size);
	
	for (;;) {
		gfx_SetColor(WHITE);
		gfx_FillRectangle_NoClip(x, y, width, FONT_HEIGHT + 2);
		gfx_SetTextXY(x + 1, y + 1);
		gfx_SetTextBGColor(WHITE);
		gfx_SetTextFGColor(BLACK);
		gfx_SetTextTransparentColor(WHITE);
		gui_PrintFileName(buffer);
		gfx_SetTextBGColor(BLACK);
		gfx_SetTextFGColor(WHITE);
		gfx_SetTextTransparentColor(BLACK);
		gfx_SetColor(BLACK);
		gfx_FillRectangle_NoClip(x + gfx_GetStringWidth(buffer) + 2, y, 9, FONT_HEIGHT + 2);
		
		keymap_indicator = keymap[47];	// Uppercase and lowercase letters
		if (keymap_indicator == '\0')
		{
			keymap_indicator = keymap[33];	// Numbers
		};
		gfx_SetTextXY(x + gfx_GetStringWidth(buffer) + 3, y + 1);
		gfx_PrintChar(keymap_indicator);
		
		gfx_BlitRectangle(1, x, y, width, height);
		
		dbg_sprintf(dbgout, "Before key loop\n");
		
		do {
			kb_Scan();
		} while ((key = asm_GetCSC()) == -1);
		
		//dbg_sprintf(dbgout, "input key = %d\n", key);
		
		if (keymap[key] != '\0' && i < buffer_size)
		{
			buffer[i++] = keymap[key];
		};
		
		if (key == sk_Del && i > 0)
		{
			buffer[--i] = '\0';
		};
		
		if (key == sk_Alpha)
		{
			if (++keymap_num > num_keymaps - 1)
			{
				keymap_num = 0;
			};
			keymap = keymaps[keymap_num];
		};
		
		if (key == sk_2nd || key == sk_Enter)
		{
			return buffer;
		};
		
		if (key == sk_Clear)
		{
			memset(buffer, '\0', buffer_size);
			return buffer;
		};
		
		delay(200);
	};
}