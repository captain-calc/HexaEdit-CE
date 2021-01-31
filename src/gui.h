#ifndef GUI_H
#define GUI_H

#include <stdbool.h>
#include <stdint.h>


#define UPPERCASE_LETTERS	"\0\0\0\0\0\0\0\0\0\0\0WRMH\0\0\0[VQLG\0\0\0ZUPKFC" \
				"\0\0YTOJEB\0\0XSNIDA\0\0\0\0\0\0\0\0"
#define LOWERCASE_LETTERS	"\0\0\0\0\0\0\0\0\0\0\0wrmh\0\0\0[vqlg\0\0\0zupkfc" \
				"\0\0ytojeb\0\0xsnida\0\0\0\0\0\0\0\0"
#define NUMBERS			"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\x33\x36\x39\0" \
				"\0\0\0\0\x32\x35\x38\0\0\0\0\x30\x31\x34\x37\0\0\0" \
				"\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0\0"


void gui_DrawMessageDialog(const char *message);
void gui_DrawMessageDialog_Blocking(const char *message);
void gui_DrawBatteryStatus(void);
void gui_DrawTime(uint24_t xPos);
void gui_PrintText(const char *name, uint8_t text_color);
void gui_PrintFileName(const char *name, uint8_t text_color);
void gui_DrawInputPrompt(const char *prompt, uint24_t input_field_width);
void gui_DrawKeymapIndicator(const char indicator, uint24_t x, uint8_t y);
int8_t gui_Input(char buffer[], uint8_t buffer_size, uint24_t x, uint8_t y, uint24_t width, const char keymap[]);
void gui_DrawProgressBar(uint24_t xPos, uint8_t yPos, uint24_t width, uint8_t progress, uint8_t range);

#endif