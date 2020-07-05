#ifndef GUI_H
#define GUI_H

#include <stdbool.h>
#include <stdint.h>

void draw_window(char *title, bool highlighted, uint24_t xPos, uint8_t yPos, uint24_t width, uint8_t height);
void draw_message_dialog(const char *message);
void draw_battery_status(void);
void draw_time(uint24_t xPos);
void print_file_name(char *name);

#endif