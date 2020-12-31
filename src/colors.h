#ifndef COLORS_H
#define COLORS_H

#include <stdint.h>

#define DK_GRAY	0x6b
#define LT_GRAY	0xb5
#define BLACK	0x00
#define WHITE	0xff
#define GREEN	0x06
#define BLUE	0x3d
#define RED	0xe0
#define YELLOW	0xe7

typedef struct {
	uint8_t background_color;
	uint8_t bar_color;
	uint8_t bar_text_color;
	uint8_t table_bg_color;
	uint8_t table_text_color;
	uint8_t selected_table_text_color;
	uint8_t table_selector_color;
	uint8_t cursor_color;
} color_theme_config_t;

/* The global structure for HexaEdit's color theme. */
extern color_theme_config_t color_theme;

#endif