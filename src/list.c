// Name:    Captain Calc
// File:    list.c
// Purpose: Defines the functions declared in list.h.


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

#include <sys/lcd.h>
#include <assert.h>
#include <graphx.h>
#include <stdbool.h>

#include "ccdbg/ccdbg.h"
#include "defines.h"
#include "hevat.h"
#include "list.h"


void list_Initialize(list* const list)
{
  list->visible_item_count = VISIBLE_ITEM_COUNT;
  list_MoveCursorIndexToStart(list);
  return;
}


void list_SetRoutineToGetItemNames(
  list* const list, void (*routine)(char*, uint24_t)
)
{
  assert(routine != NULL);

  list->get_item_name = routine;
  return;
}


void list_SetPosition(
  list* const list, const uint24_t xpos, const uint8_t ypos
)
{
  assert(xpos < LCD_WIDTH);
  assert(ypos < LCD_HEIGHT);

  list->xpos = xpos;
  list->ypos = ypos;
  return;
}


void list_SetTotalItemCount(list* const list, const uint24_t count)
{
  list->total_item_count = count;
  return;
}


uint24_t list_GetTotalItemCount(list* const list)
{
  return list->total_item_count;
}


uint24_t list_GetCursorIndex(const list* const list)
{
  assert(list->window_offset + list->cursor_offset < list->total_item_count);

  return list->window_offset + list->cursor_offset;
}


void list_MoveCursorIndexToStart(list* const list)
{
  list->window_offset = 0;
  list->cursor_offset = 0;
  return;
}


void list_IncrementCursorIndex(list* const list)
{
  uint24_t last_visible_item_offset = (
    list->total_item_count > list->visible_item_count
    ? list->visible_item_count
    : list->total_item_count
  );

  if ((list->cursor_offset + 1) < last_visible_item_offset)
  {
    list->cursor_offset++;
  }
  else if (list_GetCursorIndex(list) + 1 < list->total_item_count)
  {
    list->window_offset++;
  }

  return;
}


void list_DecrementCursorIndex(list* const list)
{
  if (list->cursor_offset)
  {
    list->cursor_offset--;
  }
  else if (list->window_offset)
  {
    list->window_offset--;
  }

  return;
}


void list_JumpToItemWhoseNameStartsWithLetter(
  list* const list, const uint8_t letter
)
{
CCDBG_BEGINBLOCK("list_JumpToItemWhoseNameStartsWithLetter");
CCDBG_DUMP_UINT(letter);

  char print_name[20] = { '\0' };
  uint24_t left = 0;
  uint24_t right = list->total_item_count;
  uint24_t middle;
  bool less_than;

  while (left < right)
  {
    middle = (left + right) / 2;

CCDBG_DUMP_UINT(middle);

    list->get_item_name(print_name, middle);

    if (print_name[0] >= 'a')
      less_than = ((print_name[0] - 'a' + 'A') < letter);
    else
      less_than = (print_name[0] < letter);

    if (less_than)
      left = middle + 1;
    else
      right = middle;
  }

CCDBG_DUMP_UINT(left);

  if (left == list->total_item_count)
    left--;

  while (list_GetCursorIndex(list) < left)
    list_IncrementCursorIndex(list);

  while (list_GetCursorIndex(list) > left)
    list_DecrementCursorIndex(list);

CCDBG_ENDBLOCK();

  return;
}
