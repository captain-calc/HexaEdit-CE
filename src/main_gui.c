// Name:    Captain Calc
// File:    main_gui.c
// Purpose: Defines the functions declared in main_gui.h and any supporting
//          static functions.


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


#include <sys/timers.h>
#include <assert.h>
#include <graphx.h>
#include <string.h>

#include "ccdbg/ccdbg.h"
#include "defines.h"
#include "editor.h"
#include "gui.h"
#include "hevat.h"
#include "keypad.h"
#include "list.h"
#include "main_gui.h"
#include "tools.h"


// =============================================================================
// STATIC FUNCTION DECLARATIONS
// =============================================================================


static void set_routine_to_get_item_names_for_variables_list(
  list* const list, const uint8_t hevat_group_idx
);


// =============================================================================
// PUBLIC FUNCTION DEFINITIONS
// =============================================================================


int maingui_Main(s_editor* const editor)
{
CCDBG_BEGINBLOCK("maingui_Main");

  const uint8_t KEYPRESS_DELAY_THRESHOLD = 8;

  list hevat_groups_list;
  list variables_list;
  void* vatptr;
  list* active_list = &hevat_groups_list;
  bool quit = false;
  bool redraw_all = true;
  bool open_variable = false;
  uint8_t hevat_group_idx = HEVAT__RECENTS;
  uint8_t letter;

  if (!hevat_Load())
    return 1;

  list_Initialize(&hevat_groups_list);
  list_SetPosition(&hevat_groups_list, 2, 22);
  list_SetTotalItemCount(&hevat_groups_list, HEVAT__NUM_GROUPS);
  list_SetRoutineToGetItemNames(
    &hevat_groups_list, &hevat_GetHEVATGroupNames
  );

  list_Initialize(&variables_list);
  list_SetPosition(&variables_list, 109, 22);

CCDBG_PUTS("After initialization");

  while (!quit)
  {
    if (open_variable)
    {
      vatptr = hevat_Ptr(
        hevat_group_idx, list_GetCursorIndex(&variables_list)
      );

      if (editor_OpenVarEditor(editor, vatptr, 0))
        hevat_AddRecent(vatptr);

      redraw_all = true;
      open_variable = false;
    }

    if (redraw_all)
    {
      gfx_FillScreen(g_color.background);
      gui_DrawMainMenuListDividers();
      gui_DrawMemoryAmounts(editor);
      gui_DrawMainMenuBottomBar();
    }

    gui_DrawMainMenuTopBar(list_GetTotalItemCount(active_list));

CCDBG_PUTS("Before HEVAT group wrangling.");

    if (list_GetCursorIndex(&hevat_groups_list) != hevat_group_idx)
      list_MoveCursorIndexToStart(&variables_list);

    hevat_group_idx = list_GetCursorIndex(&hevat_groups_list);
    list_SetTotalItemCount(&variables_list, hevat_NumEntries(hevat_group_idx));
    set_routine_to_get_item_names_for_variables_list(
      &variables_list, hevat_group_idx
    );

    if (active_list == &hevat_groups_list)
    {
      gui_DrawActiveList(&hevat_groups_list);
      gui_DrawDormantList(&variables_list);

      // Slow down the cursor movement speed.
      delay(30);
    }
    else
    {
      gui_DrawDormantList(&hevat_groups_list);
      gui_DrawActiveList(&variables_list);

      gui_EraseHEVATEntryInfo();
      gui_DrawHEVATEntryInfo(
        hevat_Ptr(hevat_group_idx, list_GetCursorIndex(&variables_list))
      );
    }

    if (redraw_all)
    {
      gfx_BlitBuffer();
      redraw_all = false;
    }
    else
      gfx_SwapDraw();

    keypad_IdleKeypadBlock();

    if (keypad_SinglePressExclusive(kb_KeyYequ))
    {
      editor_OpenMemEditor(editor, "ROM", G_ROM_BASE_ADDRESS, G_ROM_SIZE, 0);
      redraw_all = true;
    }

    if (keypad_SinglePressExclusive(kb_KeyWindow))
    {
      editor_OpenMemEditor(editor, "RAM", G_RAM_BASE_ADDRESS, G_RAM_SIZE, 0);
      redraw_all = true;
    }

    if (keypad_SinglePressExclusive(kb_KeyZoom))
    {
      editor_OpenMemEditor(
        editor, "Ports", G_PORTS_BASE_ADDRESS, G_PORTS_SIZE, 0
      );
      redraw_all = true;
    }

    if (keypad_SinglePressExclusive(kb_KeyGraph))
    {
      gui_MessageWindowBlocking(
        "About",
        "Author:$  Caleb \"Captain Calc\" Arant$$This program can damage" \
        "$your calculator if used$incautiously.$$Read the README."
      );
      redraw_all = true;
    }

    if (keypad_SinglePressExclusive(kb_KeyClear))
      quit = true;

    if (keypad_KeyPressedOrHeld(kb_KeyUp, KEYPRESS_DELAY_THRESHOLD))
      list_DecrementCursorIndex(active_list);

    if (keypad_KeyPressedOrHeld(kb_KeyDown, KEYPRESS_DELAY_THRESHOLD))
      list_IncrementCursorIndex(active_list);

    if (active_list == &hevat_groups_list)
    {
      if (
        keypad_SinglePressExclusive(kb_KeyRight)
        && list_GetTotalItemCount(&variables_list) > 0)
      {
        active_list = &variables_list;
      }
    }
    else
    {
      if (
        keypad_SinglePressExclusive(kb_Key2nd)
        || keypad_SinglePressExclusive(kb_KeyEnter)
        || keypad_SinglePressExclusive(kb_KeyRight)
      )
      {
        open_variable = true;
      }

      if (keypad_SinglePressExclusive(kb_KeyLeft))
      {
        active_list = &hevat_groups_list;
        redraw_all = true;
      }

      if (
        hevat_group_idx != HEVAT__RECENTS
        && keypad_ExclusiveASCII(&letter, 'A')
      )
      {
        list_JumpToItemWhoseNameStartsWithLetter(&variables_list, letter);
      }
    }
  }

  if (!hevat_SaveRecents())
  {
    gui_MessageWindowBlocking(
      "Warning", "Cannot save variables to$Recents appvar."
    );
  }

CCDBG_ENDBLOCK();
  return 0;
}


// =============================================================================
// STATIC FUNCTION DEFINITIONS
// =============================================================================


static void set_routine_to_get_item_names_for_variables_list(
  list* const list, const uint8_t hevat_group_idx
)
{
  assert(hevat_group_idx < HEVAT__NUM_GROUPS);

  void (*routine_list[HEVAT__NUM_GROUPS])(char*, uint24_t) = {
    &hevat_GetRecentsVariableName,
    &hevat_GetAppvarVariableName,
    &hevat_GetProtProgramVariableName,
    &hevat_GetProgramVariableName,
    &hevat_GetRealVariableName,
    &hevat_GetListVariableName,
    &hevat_GetMatrixVariableName,
    &hevat_GetEquationVariableName,
    &hevat_GetStringVariableName,
    &hevat_GetPictureVariableName,
    &hevat_GetGDBVariableName,
    &hevat_GetComplexVariableName,
    &hevat_GetComplexListVariableName,
    &hevat_GetGroupVariableName,
    &hevat_GetOtherVariableName
  };

  list_SetRoutineToGetItemNames(list, routine_list[hevat_group_idx]);
  return;
}
