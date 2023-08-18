// Name:    Captain Calc
// File:    main_gui.c
// Purpose: Defines the functions declared in main_gui.h and any supporting
//          static functions.


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
#include "main_gui.h"
#include "tools.h"


// =============================================================================
// STATIC FUNCTION DECLARATIONS
// =============================================================================


void binary_search(
  s_list* list,
  const uint8_t hevat_group_idx,
  const uint8_t letter
);


// =============================================================================
// PUBLIC FUNCTION DEFINITIONS
// =============================================================================


int maingui_Main(s_editor* const editor)
{
CCDBG_BEGINBLOCK("maingui_Main");

  s_list* list;
  bool quit = false;
  bool redraw_all = true;
  bool open_variable = false;
  uint8_t hevat_group_idx = HEVAT__RECENTS;
  uint8_t letter;

  s_list master_list = {
    .num_items = HEVAT__NUM_GROUPS,
    .xpos = 2,
    .ypos = 22,
    .width = 101,
    .section_length = 19,
    .base_offset = 0,
    .section_offset = 0
  };

  s_list sub_list = {
  .num_items = 0,
  .xpos = 109,
  .ypos = 22,
  .width = 101,
  .section_length = 19,
  .base_offset = 0,
  .section_offset = 0
  };

  if (!hevat_Load())
    return 1;

  list = &master_list;

  while (!quit)
  {
    if (open_variable)
    {
      void* vatptr = hevat_Ptr(hevat_group_idx, gui_ListAbsOffset(list));
      hevat_AddRecent(vatptr);
      hevat_SaveRecents();
      editor_OpenVarEditor(editor, vatptr, 0);

      if (!hevat_Load())
      {
CCDBG_ENDBLOCK();

        gui_ErrorWindow("Cannot reload HEVAT.$Program will close.");
        return 1;
      }

      redraw_all = true;
      open_variable = false;
    }

    if (redraw_all)
    {
      gfx_FillScreen(g_color.background);
      gui_DrawMainMenuTopBar(list->num_items);
      gui_DrawMainMenuListDividers();
      gui_DrawMemoryAmounts();
      gui_DrawMainMenuBottomBar();
    }

    hevat_group_idx = gui_ListAbsOffset(&master_list);
    sub_list.num_items = hevat_NumEntries(hevat_group_idx);
    gui_DrawList(&master_list, HEVAT__GROUP_NAMES);
    gui_DrawHEVATList(&sub_list, hevat_group_idx);
    gui_DrawSelectedListIndicator(list == &master_list);

    if (list == &sub_list)
    {
      gui_EraseHEVATEntryInfo();
      gui_DrawHEVATEntryInfo(
        hevat_Ptr(hevat_group_idx, gui_ListAbsOffset(&sub_list))
      );
    }
    else
    {
      // Slow down the cursor.
      delay(30);
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
        "Version: 3.0.0$Author:$  Caleb \"Captain Calc\" Arant$$This program can damage$your calculator if used$incautiously.$Read the README."
      );
      redraw_all = true;
    }

    if (keypad_SinglePressExclusive(kb_KeyClear))
      quit = true;

    if (list == &sub_list)
    {
      if (
        keypad_SinglePressExclusive(kb_Key2nd)
        || keypad_SinglePressExclusive(kb_KeyEnter)
      )
      {
        open_variable = true;
      }

      if (keypad_SinglePressExclusive(kb_KeyLeft))
      {
        gui_ResetListOffset(list);
        gui_EraseHEVATEntryInfo();
        list = &master_list;
        redraw_all = true;
      }

      if (
        hevat_group_idx != HEVAT__RECENTS
        && keypad_ExclusiveASCII(&letter, 'A')
      )
      {
        binary_search(list, hevat_group_idx, letter);
      }
    }

    if (keypad_SinglePressExclusive(kb_KeyRight))
    {
      if (list == &master_list && sub_list.num_items)
      {
        list = &sub_list;
        redraw_all = true;
      }
      else if (list == &sub_list)
        open_variable = true;
    }

    if (kb_IsDown(kb_KeyUp))
      gui_DecrementListOffset(list);

    if (kb_IsDown(kb_KeyDown))
      gui_IncrementListOffset(list);
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


void binary_search(
  s_list* list,
  const uint8_t hevat_group_idx,
  const uint8_t letter
)
{
CCDBG_BEGINBLOCK("binary_search");
CCDBG_DUMP_UINT(letter);

  char print_name[20] = { '\0' };
  s_calc_var var;
  uint24_t left = 0;
  uint24_t right = list->num_items;
  uint24_t middle;
  bool var_found;
  bool less_than;

  while (left < right)
  {    
    middle = (left + right) / 2;

CCDBG_DUMP_UINT(middle);
CCDBG_DUMP_PTR(hevat_Ptr(hevat_group_idx, middle));

    var.vatptr = hevat_Ptr(hevat_group_idx, middle);
    var_found = hevat_GetVarInfoByVAT(&var);

    if (!var_found)
    {

CCDBG_PUTS("Variable not found.");

      break;
    }

    memset(print_name, '\0', 20);
    hevat_VarNameToASCII(print_name, (const uint8_t*)var.name, var.named);

CCDBG_PUTS("Variable found.");

    
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

  if (var_found)
  {
    while (gui_ListAbsOffset(list) < left - (left == list->num_items ? 1 : 0))
      gui_IncrementListOffset(list);

    while (gui_ListAbsOffset(list) > left)
      gui_DecrementListOffset(list);
  }

CCDBG_ENDBLOCK();

  return;
}