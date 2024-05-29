// Name:    Captain Calc
// File:    tools.h
// Purpose: Declares several standalone functions, tools, that modify the
//          the editor's state or buffer. These are the core functionality of
//          HexaEdit.


/*
BSD 3-Clause License

Copyright (c) 2024, Caleb "Captain Calc" Arant
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef TOOLS_H
#define TOOLS_H


#include "defines.h"


void tool_FatalErrorExit(void);


uint8_t tool_GetCutCopyBufferSize();


void tool_InitUndoStack(void);


bool tool_CreateRecentsAppvar(void);


bool tool_CreateEditBuffer(s_editor* const editor);


// Post: If successful, pointer returned and <size> set to buffer size.
//       Otherwise, NULL returned and <size> is undefined.
void* tool_EditBufferPtr(uint24_t* size);


bool tool_CheckFreeRAM(const uint24_t amt);


// Description: Success -> 1
//              Variable not saved, but program can continue -> 0
//              Fatal program error -> -1
int8_t tool_SaveModifiedVar(s_editor* const editor);


// Pre:  <editor->base_address> and <editor->buffer_size>
// Post: <editor->data_size>, <editor->near_size>, and <editor->far_size> set.
bool tool_BufferVarData(
  s_editor* const editor,
  void* data,
  const uint24_t size,
  const uint24_t offset
);


void tool_LoadEditBufferIntoVar(
  s_editor* const editor, uint8_t* const var_data
);


// Description: Deletes the edit buffer appvar.
// Pre:         Edit buffer appvar must exist.
// Post:        Edit buffer appvar deleted.
void tool_DeleteEditBuffer(void);


// Description: Determines based on the editor's state if a tool can be used.
// Pre:         Pointer to tool must be valid.
// Post:        If tool can be used, true returned.
//              If tool cannot be used, false returned.
bool tool_IsAvailable(const s_editor* const editor, void* const tool_func_ptr);


// Description: <direction> == 0 decreases the size of the near buffer.
//              <direction> == 1 increases the size of the near buffer.
void tool_MoveCursor(
  s_editor* const editor, const uint8_t direction, const uint24_t amt
);


bool tool_UpdateWindowOffset(s_editor* const editor);


void tool_WriteNibble(s_editor* const editor, uint8_t nibble);


void tool_WriteByte(s_editor* const editor, uint8_t byte);


// Description: Moves the editing cursor to the given offset. If the offset
//              exceeds the data size, the offset defaults to the maximum valid
//              offset.
void tool_Goto(s_editor* const editor, uint24_t offset);


void tool_InsertBytes(s_editor* const editor, const uint24_t num);


void tool_DeleteBytes(s_editor* const editor);


void tool_CopyBytes(const s_editor* const editor);


void tool_CutBytes(s_editor* const editor);


void tool_PasteBytes(s_editor* const editor);


void tool_FindPhrase(
  s_editor* const editor,
  const uint8_t phrase[],
  const uint8_t length,
  uint24_t matches[],
  uint8_t* num_matches
);


void tool_SwitchWritingMode(s_editor* const editor);


void tool_UndoLastAction(s_editor* const editor);


void tool_AddUndo_WriteNibble(s_editor* const editor);


void tool_AddUndo_WriteByte(s_editor* const editor);


void tool_AddUndo_InsertBytes(s_editor* const editor, const uint8_t num_bytes);


void tool_AddUndo_DeleteOrCutBytes(s_editor* const editor);


void tool_AddUndo_PasteBytes(s_editor* const editor);


#endif
