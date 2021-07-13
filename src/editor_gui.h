#ifndef EDITOR_GUI_H
#define EDITOR_GUI_H

#include "editor.h"

void editorgui_DrawTopBar(editor_t *editor);
void editorgui_DrawToolBar(editor_t *editor);
void editorgui_DrawAltToolBar(cursor_t *cursor);
void editorgui_SuperuserEngagedScreen(void);
void editorgui_DrawMemAddresses(editor_t *editor, uint24_t x, uint8_t y);
void editorgui_DrawFileOffsets(editor_t *editor, uint24_t x, uint8_t y);
void editorgui_DrawHexTable(editor_t *editor, cursor_t *cursor, uint24_t x, uint8_t y);
void editorgui_DrawAsciiTable(editor_t *editor, cursor_t *cursor, uint24_t x, uint8_t y);
void editorgui_DrawEmptyFileMessage(uint24_t hex_x, uint8_t y);
void editorgui_DrawEditorContents(editor_t *editor, cursor_t *cursor, uint8_t editor_index_method);

#endif