#ifndef EDITOR_ACTIONS_H
#define EDITOR_ACTIONS_H

#include "editor.h"

#include <stdbool.h>
#include <stdint.h>

void editact_SpriteViewer(editor_t *editor, cursor_t *cursor);
void editact_Goto(editor_t *editor, cursor_t *cursor, uint8_t *ptr);
bool editact_DeleteBytes(editor_t *editor, cursor_t *cursor, uint8_t *deletion_point, uint24_t num_bytes);
bool editact_InsertBytes(editor_t *editor, uint8_t *insertion_point, uint24_t num_bytes);
bool editact_CreateUndoInsertBytesAction(editor_t *editor, cursor_t *cursor, uint24_t num_bytes);
bool editact_CreateDeleteBytesUndoAction(editor_t *editor, cursor_t *cursor, uint24_t num_bytes);
uint8_t editact_GetNibble(cursor_t *cursor, uint8_t *ptr);
void editact_WriteNibble(cursor_t *cursor, uint8_t nibble);
bool editact_CreateUndoWriteNibbleAction(editor_t *editor, cursor_t *cursor, uint8_t nibble);
bool editact_UndoAction(editor_t *editor, cursor_t *cursor);
uint8_t editact_FindPhraseOccurances(
	uint8_t *search_start,
	uint24_t search_range,
	uint8_t *search_min,
	uint8_t *search_max,
	char phrase[],
	uint8_t phrase_len,
	uint8_t **occurances
);


#endif