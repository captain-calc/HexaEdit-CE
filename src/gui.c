#include "defines.h"
#include "gui.h"

#include <graphx.h>
#include <tice.h>

#include <stdbool.h>
#include <stdio.h>


void draw_window(char *title, bool highlighted, uint24_t xPos, uint8_t yPos, uint24_t width, uint8_t height) {
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(xPos, yPos, width, 14);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
	gfx_PrintStringXY(title, xPos + width / 2 - gfx_GetStringWidth(title) / 2, yPos + 3);
	gfx_SetColor(BLACK);
	gfx_Rectangle_NoClip(xPos, yPos + 15, width, height - 15);
	gfx_SetColor(LT_GRAY);
	if (highlighted)
		gfx_SetColor(WHITE);
	gfx_FillRectangle_NoClip(xPos + 1, yPos + 16, width - 2, height - 17);
	return;
}

void draw_message_dialog(const char *message) {
	
	uint24_t message_width = gfx_GetStringWidth(message);
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(150 - message_width / 2, 100, message_width + 20, 40);
	gfx_SetTextBGColor(DK_GRAY);
	gfx_SetTextFGColor(WHITE);
	gfx_SetTextTransparentColor(DK_GRAY);
	gfx_PrintStringXY(message, 160 - gfx_GetStringWidth(message) / 2, 117);
	gfx_BlitBuffer();
	delay(200);
	return;
}

void draw_battery_status(void) {
	
	uint8_t status;
	char status_str[5] = {'\0'};
	
	status = boot_GetBatteryStatus();
	gfx_SetColor(BLACK);
	gfx_HorizLine_NoClip(310, 3, 4);
	gfx_FillRectangle_NoClip(308, 4, 8, 12);
	gfx_SetColor(WHITE);
	gfx_FillRectangle_NoClip(309, 5, 6, 10);
	gfx_SetColor(GREEN);
	gfx_FillRectangle_NoClip(310, 14 - (2 * status), 4, 2 * status);
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

void draw_time(uint24_t xPos) {
	
	uint8_t seconds = 0, minutes = 0, hour = 0;
	const char *time_ind[2] = {"AM", "PM"};
	
	gfx_SetColor(DK_GRAY);
	gfx_FillRectangle_NoClip(xPos, 6, 55, 7);
	
	boot_GetTime(&seconds, &minutes, &hour);
	gfx_SetTextXY(xPos, 6);
	gfx_SetTextFGColor(WHITE);
	if (hour == 0) hour = 12;
	if (hour > 12) hour -= 12;
	gfx_PrintUInt((unsigned int)hour, 2);
	gfx_PrintString(":");
	gfx_PrintUInt((unsigned int)minutes, 2);
	
	gfx_PrintStringXY(time_ind[(uint8_t)boot_IsAfterNoon], xPos + 38, 6);
	return;
}

void print_file_name(char *name) {

	char *character = name;
	uint8_t name_width = 0;
	

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