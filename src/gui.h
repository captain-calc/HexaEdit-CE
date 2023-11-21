// Name:    Captain Calc
// File:    gui.h
// Purpose: Declares all of the graphical routines used by HexaEdit.


/*
BSD 3-Clause License

Copyright (c) 2023, Caleb "Captain Calc" Arant
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


#ifndef GUI_H
#define GUI_H


#include "defines.h"
#include "list.h"


// TODO: Rename
void gui_SetTextColor(uint8_t bg_color, uint8_t fg_color);

void gui_PrintText(const char* const text, const uint8_t text_color);

void gui_MessageWindowBlocking(const char* const title, const char* const msg);

void gui_ErrorWindow(const char* const msg);

void gui_DrawActiveList(const list* const list);

void gui_DrawDormantList(const list* const list);

void gui_EraseHEVATEntryInfo(void);

void gui_DrawHEVATEntryInfo(void* vatptr);

void gui_DrawMemoryAmounts(const s_editor* const editor);

void gui_DrawMainMenuTopBar(uint24_t num_list_items);

void gui_DrawMainMenuListDividers(void);

void gui_DrawMainMenuBottomBar(void);

void gui_DrawLocationColumn(const s_editor* const editor);

void gui_PrintData(const s_editor* const editor);

void gui_DrawTitleBar(const s_editor* const editor);

void gui_DrawToolBar(const s_editor* const editor);

void gui_DrawFindPromptMessage(const char* const message);

void gui_DrawFindPhraseToolbar(
  const uint8_t match_idx, const uint8_t num_matches
);

void gui_DrawSavePrompt(void);

void gui_DrawInputPrompt(
  const char* const prompt, const uint24_t input_field_width
);

void gui_DrawKeymapIndicator(
  const char indicator, const uint24_t x_pos, const uint8_t y_pos
);

// <buffer_size> excludes the null terminator. If you give a <buffer_size> of 8,
// the actual number of bytes in the buffer should be 9.
void gui_Input(
  char* const buffer,
  const uint8_t buffer_size,
  const uint24_t x_pos,
  const uint8_t y_pos,
  const uint24_t width,
  const char* const keymap[7]
);


#endif
