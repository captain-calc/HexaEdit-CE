// Name:    Captain Calc
// Date:    July 13, 2021
// File:    editor_actions.h
// Purpose: editor_actions implements each of the editor tools, such as byte
//          insertion/deletion, the "Goto" function, writing nibbles, and so
//          on. It also provides the functions for undoing each action.


#ifndef EDITOR_ACTIONS_H
#define EDITOR_ACTIONS_H


#include "editor.h"

#include <stdbool.h>
#include <stdint.h>


// FILE DEFINES
// ============================================================================

#define MAX_NUM_PHRASE_OCCURANCES   ((uint8_t)255)


// PRIVATE FUNCTION DECLARATIONS
// ============================================================================


// PUBLIC FUNCTION DECLARATIONS
// ============================================================================


// Description: 
// Pre:         
// Post:        
void editact_SpriteViewer(editor_t *editor, cursor_t *cursor);


// Description: 
// Pre:         
// Post:        
void editact_Goto(editor_t *editor, cursor_t *cursor, uint8_t *ptr);


// Description: 
// Pre:         
// Post:        
bool editact_DeleteBytes(
  editor_t *editor,
  cursor_t *cursor,
  uint8_t *deletion_point,
  uint24_t num_bytes
);


// Description: 
// Pre:         
// Post:        
bool editact_InsertBytes(
  editor_t *editor,
  uint8_t *insertion_point,
  uint24_t num_bytes
);


// Description: 
// Pre:         
// Post:        
bool editact_CreateUndoInsertBytesAction(
  editor_t *editor,
  cursor_t *cursor,
  uint24_t num_bytes
);


// Description: 
// Pre:         
// Post:        
bool editact_CreateDeleteBytesUndoAction(
  editor_t *editor,
  cursor_t *cursor,
  uint24_t num_bytes
);


// Description: 
// Pre:         
// Post:        
uint8_t editact_GetNibble(cursor_t *cursor, uint8_t *ptr);


// Description: 
// Pre:         
// Post:        
void editact_WriteNibble(cursor_t *cursor, uint8_t nibble);


// Description: 
// Pre:         
// Post:        
bool editact_CreateUndoWriteNibbleAction(
 editor_t *editor,
 cursor_t *cursor,
 uint8_t nibble
);


// Description: 
// Pre:         
// Post:        
bool editact_UndoAction(editor_t *editor, cursor_t *cursor);


// Description: 
// Pre:         
// Post:        
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