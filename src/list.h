// Name:    Captain Calc
// File:    list.h
// Purpose: Declares the API for manipulating the main menu list objects.


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

#ifndef LIST_H
#define LIST_H


#include <stdint.h>


typedef struct
{
  void (*get_item_name)(char[20], uint24_t);
  uint24_t total_item_count;
  uint8_t visible_item_count;
  uint24_t xpos;
  uint8_t ypos;
  uint8_t width;
  uint24_t window_offset;
  uint24_t cursor_offset;
} list;

#define VISIBLE_ITEM_COUNT                         (19)
#define LIST_WIDTH_IN_PIXELS                      (101)
#define LIST_ITEM_HEIGHT_IN_PIXELS  (G_FONT_HEIGHT + 3)

typedef struct
{
  uint8_t background;
  uint8_t cursor;
  uint8_t cursor_text;
  uint8_t normal_text;
} list_color_palette;


void list_Initialize(list* const list);

void list_SetRoutineToGetItemNames(
  list* const list, void (*routine)(char*, uint24_t)
);

void list_SetPosition(
  list* const list, const uint24_t xpos, const uint8_t ypos
);

void list_SetTotalItemCount(list* const list, const uint24_t count);

uint24_t list_GetTotalItemCount(list* const list);

uint24_t list_GetCursorIndex(const list* const list);

void list_MoveCursorIndexToStart(list* const list);

void list_IncrementCursorIndex(list* const list);

void list_DecrementCursorIndex(list* const list);

void list_JumpToItemWhoseNameStartsWithLetter(
  list* const list, const uint8_t letter
);


#endif
