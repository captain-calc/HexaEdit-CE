// Name:    Captain Calc
// File:    editor.c
// Purpose: Defines the functions declared in editor.h as well as several static
//          supporting functions.


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
#include <sys/timers.h>
#include <ti/vars.h>
#include <assert.h>
#include <graphx.h>
#include <string.h>

#include "ccdbg/ccdbg.h"
#include "cutil.h"
#include "defines.h"
#include "editor.h"
#include "gui.h"
#include "hevat.h"
#include "keypad.h"
#include "tools.h"


// =============================================================================
// STATIC FUNCTION DECLARATIONS
// =============================================================================


// Description: Handles the editor's main loop.
static void run_editor(s_editor* const editor);


static void goto_prompt(s_editor* const editor);


static void insert_bytes_prompt(s_editor* const editor);


static void find_prompt(s_editor* const editor);


static bool save_changes_prompt(s_editor* const editor);


// =============================================================================
// PUBLIC FUNCTION DEFINITIONS
// =============================================================================


void editor_OpenVarEditor(
  s_editor* const editor, void* const vatptr, const uint24_t offset
)
{
CCDBG_BEGINBLOCK("editor_OpenVarEditor");

  s_calc_var var;
  uint24_t var_data_size;
  uint8_t* var_data;

  var.vatptr = vatptr;
  hevat_GetVarInfoByVAT(&var);
  memset(editor->name, '\0', G_EDITOR_NAME_MAX_LEN);
  strncpy(editor->name, var.name, var.name_length);
  editor->name_length = var.name_length;
  editor->is_tios_var = true;
  editor->tios_var_type = var.type;
  editor->undo_buffer_active = true;
  editor->num_changes = 0;
  editor->base_address = tool_EditBufferPtr(&editor->buffer_size);

  if (var.named)
  {
    editor->access_type = 'i';
    var_data = var.data + 2;
    var_data_size = var.size - 2;
  }
  else if (var.archived)
  {
    editor->access_type = 'r';
    var_data = var.data;
    var_data_size = var.size;
  }
  else
  {
    editor->access_type = 'w';
    var_data = var.data;
    var_data_size = var.size;
  }

  editor->selection_size = 1;
  editor->selection_active = false;
  editor->high_nibble = true;

  if (tool_BufferVarData(editor, var_data, var_data_size, offset))
  {
    tool_InitUndoStack();
    run_editor(editor);
  }

CCDBG_ENDBLOCK();

  return;
}


void editor_OpenMemEditor(
  s_editor* const editor,
  const char* const name,
  uint8_t* const base_address,
  const uint24_t size,
  const uint24_t offset
)
{
CCDBG_BEGINBLOCK("editor_OpenMemEditor");
CCDBG_DUMP_UINT(strlen(name));

  memset(editor->name, '\0', G_EDITOR_NAME_MAX_LEN);
  strncpy(editor->name, name, strlen(name));
  editor->name_length = strlen(name);
  editor->is_tios_var = false;
  editor->tios_var_type = 0;
  editor->undo_buffer_active = true;
  editor->num_changes = 0;
  editor->base_address = base_address;
  editor->data_size = size;
  editor->buffer_size = size;

  if ((uint24_t)base_address < (uint24_t)G_ROM_BASE_ADDRESS + G_ROM_SIZE)
    editor->access_type = 'r';
  else
    editor->access_type = 'w';

  editor->near_size = offset + 1;
  editor->far_size = size - offset - 1;
  editor->selection_size = 1;
  editor->selection_active = false;
  editor->high_nibble = true;

  tool_InitUndoStack();
  run_editor(editor);

CCDBG_ENDBLOCK();

  return;
}


// =============================================================================
// STATIC FUNCTION DEFINITIONS
// =============================================================================


static void run_editor(s_editor* const editor)
{
CCDBG_BEGINBLOCK("run_editor");
  bool quit = false;
  bool redraw_location_col = true;  // Draw the column for initialization.
  bool blit = true;
  bool accel_cursor = false;
  uint8_t writing_value = 0;

  if (editor->selection_size > 1)
    editor->selection_active = true;

  tool_UpdateWindowOffset(editor);

  while (!quit)
  {
    gfx_SetColor(g_color.bar);
    gfx_VertLine_NoClip(60, 20, 200);
    gfx_VertLine_NoClip(61, 20, 200);
    gfx_VertLine_NoClip(230, 20, 200);
    gfx_VertLine_NoClip(231, 20, 200);

    gui_DrawTitleBar(editor);
    gui_DrawToolBar(editor);
    gui_PrintData(editor);

    if (redraw_location_col)
    {
      gui_DrawLocationColumn(editor);
      redraw_location_col = false;
      blit = true;
    }

    if (blit)
    {
      gfx_BlitBuffer();
      blit = false;
    }
    else
      gfx_SwapDraw();

    // Slow down the cursor for small variables.
    if (editor->data_size < G_NUM_BYTES_ONSCREEN / 2)
      delay(30);

    keypad_IdleKeypadBlock();

    if (
      keypad_SinglePressExclusive(kb_KeyYequ)
      && tool_IsAvailable(editor, &tool_FindPhrase)
    )
    {
      find_prompt(editor);
      redraw_location_col = true;
    }

    if (
      keypad_SinglePressExclusive(kb_KeyWindow)
      && tool_IsAvailable(editor, &tool_InsertBytes)
    )
    {
      insert_bytes_prompt(editor);
      redraw_location_col = true;
    }

    if (keypad_SinglePressExclusive(kb_KeyZoom))
    {
      if (tool_IsAvailable(editor, &tool_Goto))
      {
        goto_prompt(editor);
      }
      else if (tool_IsAvailable(editor, &tool_CopyBytes))
      {
        tool_CopyBytes(editor);
      }
    }

    if (keypad_SinglePressExclusive(kb_KeyTrace))
    {
      if (tool_IsAvailable(editor, &tool_UndoLastAction))
      {
        tool_UndoLastAction(editor);
      }
      else if (tool_IsAvailable(editor, &tool_CutBytes))
      {
        tool_AddUndo_DeleteOrCutBytes(editor);
        tool_CutBytes(editor);
      }

      redraw_location_col = true;
    }

    if (keypad_SinglePressExclusive(kb_KeyGraph))
    {
      if (tool_IsAvailable(editor, &tool_SwitchWritingMode))
      {
        tool_SwitchWritingMode(editor);
      }
      else if (tool_IsAvailable(editor, &tool_PasteBytes))
      {
        tool_AddUndo_PasteBytes(editor);
        tool_PasteBytes(editor);
        redraw_location_col = true;
      }
    }

    if (
      (
        keypad_SinglePressExclusive(kb_Key2nd)
        || keypad_SinglePressExclusive(kb_KeyEnter)
      )
      && editor->near_size
    )
    {
      editor->selection_active = !editor->selection_active;

      if (!editor->selection_active)
        editor->selection_size = 1;
    }

    if (
      keypad_SinglePressExclusive(kb_KeyMode)
      && !editor->is_tios_var
    )
    {
      if (editor->location_col_mode == 'a')
        editor->location_col_mode = 'o';
      else
        editor->location_col_mode = 'a';

      redraw_location_col = true;
    }

    if (
      kb_IsDown(kb_KeyDel)
      && tool_IsAvailable(editor, &tool_DeleteBytes)
    )
    {
      tool_AddUndo_DeleteOrCutBytes(editor);
      tool_DeleteBytes(editor);
      redraw_location_col = true;
    }

    if (keypad_SinglePressExclusive(kb_KeyClear))
    {
      if (editor->num_changes)
        quit = save_changes_prompt(editor);
      else
        quit = true;
    }

    if (kb_IsDown(kb_KeyAlpha))
      accel_cursor = true;
    else
      accel_cursor = false;

    if (kb_IsDown(kb_KeyLeft))
      tool_MoveCursor(editor, 0, 1);

    if (kb_IsDown(kb_KeyRight))
      tool_MoveCursor(editor, 1, 1);

    if (kb_IsDown(kb_KeyUp))
    {
      tool_MoveCursor(
        editor, 0, (accel_cursor ? G_NUM_BYTES_ONSCREEN : G_COLS_ONSCREEN)
      );
    }

    if (kb_IsDown(kb_KeyDown))
    {
      tool_MoveCursor(
        editor, 1, (accel_cursor ? G_NUM_BYTES_ONSCREEN : G_COLS_ONSCREEN)
      );
    }

    if (editor->writing_mode == 'x')
    {
      if (
        keypad_ExclusiveNibble(&writing_value)
        && tool_IsAvailable(editor, &tool_WriteNibble)
      )
      {
        tool_AddUndo_WriteNibble(editor);
        tool_WriteNibble(editor, writing_value);

        if (!editor->high_nibble)
          tool_MoveCursor(editor, 1, 1);
        else
          editor->high_nibble = false;
      }
    }
    else
    {
      if (
        keypad_ExclusiveASCII(&writing_value, editor->writing_mode)
        && tool_IsAvailable(editor, &tool_WriteByte)
      )
      {
        tool_AddUndo_WriteByte(editor);
        tool_WriteByte(editor, writing_value);
        tool_MoveCursor(editor, 1, 1);
      }
    }

    redraw_location_col |= tool_UpdateWindowOffset(editor);
  }

CCDBG_ENDBLOCK();
  return;
}


static void goto_prompt(s_editor* const editor)
{
  const char** keymap = NULL;
  char keymap_indicator = 'x';
  char buffer[8] = { '\0' };
  uint8_t buffer_size;
  uint24_t offset;
  uint24_t address;
  
  if (editor->location_col_mode == 'o')
  {
    keymap = G_DIGITS_KEYMAP;
    keymap_indicator = '0';
    buffer_size = 7;
  }
  else
  {
    keymap = G_HEX_ASCII_KEYMAP;
    buffer_size = 6;
  }

  gui_DrawInputPrompt("Goto:", 102);
  gui_DrawKeymapIndicator(keymap_indicator, 135, 223);
  gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);

  while (true)
  {
    kb_Scan();
    gui_DrawInputPrompt("Goto:", 102);
    gui_DrawKeymapIndicator(keymap_indicator, 135, 223);
    gui_SetTextColor(g_color.background, g_color.list_text_normal);
    gui_Input(buffer, buffer_size, 44, 224, 99, keymap);
    gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
    
    if (keypad_SinglePressExclusive(kb_KeyClear))
      return;
    
    if (
      keypad_SinglePressExclusive(kb_Key2nd)
      || keypad_SinglePressExclusive(kb_KeyEnter)
    )
    {
      break;
    }
  }

CCDBG_PUTS(buffer);

  if (editor->location_col_mode == 'o')
    offset = atoi(buffer);
  else
  {
    address = cutil_HexToUint(buffer);

    if (address < (uint24_t)editor->base_address)
      offset = 0;
    else
      offset = address - (uint24_t)editor->base_address;
  }

CCDBG_DUMP_UINT(offset);

  tool_Goto(editor, offset);
  return;
}


static void insert_bytes_prompt(s_editor* const editor)
{
  char buffer[7] = { '\0' };

  gui_DrawInputPrompt("Insert:", 102);
  gui_DrawKeymapIndicator('0', 151, 223);
  gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);

  while (true)
  {
    kb_Scan();
    gui_DrawInputPrompt("Insert:", 102);
    gui_DrawKeymapIndicator('0', 151, 223);
    gui_SetTextColor(g_color.background, g_color.list_text_normal);
    gui_Input(buffer, 6, 60, 224, 99, G_DIGITS_KEYMAP);
    gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
    
    if (keypad_SinglePressExclusive(kb_KeyClear))
      return;
    
    if (
      keypad_SinglePressExclusive(kb_Key2nd)
      || keypad_SinglePressExclusive(kb_KeyEnter)
    )
    {
      break;
    }
  }

  tool_AddUndo_InsertBytes(editor, atoi(buffer));
  tool_InsertBytes(editor, atoi(buffer));
  return;
}


static void ascii_to_nibble(char* ascii_buffer, char* nibble_buffer)
{
  assert(strlen(ascii_buffer) % 2 == 0);

  uint8_t idx = 0;
  uint8_t length = strlen(ascii_buffer);

  while (idx < length)
  {
    nibble_buffer[idx / 2] = (
      ascii_buffer[idx] <= '9'
      ? ascii_buffer[idx] - '0'
      : ascii_buffer[idx] - 'a' + 10
    ) << 4;
    idx++;
    nibble_buffer[idx / 2] |= (
      ascii_buffer[idx] <= '9'
      ? ascii_buffer[idx] - '0'
      : ascii_buffer[idx] - 'a' + 10
    );
    idx++;
  }

  return;
}


static void find_viewer(
  s_editor* const editor, uint24_t matches[], uint8_t num_matches
)
{
  uint8_t prev_idx = 0;
  uint8_t idx = 0;

  while (true)
  {
    // This section massages the window offset using selective goto actions to
    // make the entire selected phrase appear onscreen.
    // ==================
    editor->selection_active = false;
    tool_Goto(editor, matches[idx]);

    if (prev_idx >= idx)
      tool_UpdateWindowOffset(editor);

    tool_Goto(editor, matches[idx] + editor->selection_size - 1);

    if (prev_idx <= idx)
      tool_UpdateWindowOffset(editor);
    
    editor->selection_active = true;
    // ==================

    gui_DrawLocationColumn(editor);
    gui_PrintData(editor);
    gui_DrawFindPhraseToolbar(idx, num_matches);
    gfx_BlitBuffer();

    keypad_IdleKeypadBlock();

    if (
      keypad_SinglePressExclusive(kb_Key2nd)
      || keypad_SinglePressExclusive(kb_KeyClear)
    )
    {
      break;
    }

    prev_idx = idx;

    if (kb_IsDown(kb_KeyUp))
      idx = (idx ? idx - 1 : num_matches - 1);
    else if (kb_IsDown(kb_KeyDown))
      idx = (idx + 1) % num_matches;
  }

  editor->selection_active = false;
  editor->selection_size = 1;

  return;
}


static void find_prompt(s_editor* const editor)
{
  const char** keymaps[] = {
    G_HEX_ASCII_KEYMAP, G_UPPERCASE_LETTERS_KEYMAP,
    G_LOWERCASE_LETTERS_KEYMAP, G_DIGITS_KEYMAP
  };
  const char keymap_indicators[] = { 'x', 'A', 'a', '0' };
  char buffer[17] = { '\0' };
  char phrase[8] = { '\0' };
  uint24_t matches[255];
  uint8_t num_matches = 0;
  uint8_t phrase_length;
  uint8_t keymap_idx = 0;
  uint8_t buffer_size = 16;
  bool find_phrase = false;

  gui_DrawInputPrompt("Find:", 152);
  gui_DrawKeymapIndicator(keymap_indicators[keymap_idx], 182, 223);
  gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);

  while (true)
  {
    kb_Scan();
    gui_DrawInputPrompt("Find:", 152);
    gui_DrawKeymapIndicator(keymap_indicators[keymap_idx], 182, 223);
    gui_SetTextColor(g_color.background, g_color.list_text_normal);
    gui_Input(buffer, buffer_size, 42, 224, 149, keymaps[keymap_idx]);
    gfx_BlitRectangle(1, 0, LCD_HEIGHT - 20, LCD_WIDTH, 20);

    if (keypad_SinglePressExclusive(kb_KeyClear) && !strlen(buffer))
      break;

    if (keypad_SinglePressExclusive(kb_KeyAlpha))
    {
      if (!strlen(buffer))
        keymap_idx = (keymap_idx + 1) % 4;
      else if (keymaps[keymap_idx] != G_HEX_ASCII_KEYMAP)
        keymap_idx = ((keymap_idx + 1) % 4) + (keymap_idx == 3 ? 1 : 0);

      if (keymaps[keymap_idx] == G_HEX_ASCII_KEYMAP)
        buffer_size = 16;
      else
        buffer_size = 8;
    }
    
    if (
      keypad_SinglePressExclusive(kb_Key2nd)
      || keypad_SinglePressExclusive(kb_KeyEnter)
    )
    {
      phrase_length = strlen(buffer);

      if (keymaps[keymap_idx] == G_HEX_ASCII_KEYMAP)
      {
        if (phrase_length > 3 && phrase_length % 2 == 0)
        {
          ascii_to_nibble(buffer, phrase);
          phrase_length /= 2;
          find_phrase = true;
        }
        else
          gui_DrawFindPromptMessage("4 or more chars");
      }
      else
      {
        if (phrase_length > 1)
        {
          strncpy(phrase, buffer, buffer_size);
          find_phrase = true;
        }
        else
          gui_DrawFindPromptMessage("2 or more chars");
      }

      if (find_phrase)
      {
        gui_DrawFindPromptMessage("Searching...");
        
        tool_FindPhrase(
          editor, (const uint8_t*)phrase, phrase_length, matches, &num_matches
        );

        if (num_matches)
          find_viewer(editor, matches, num_matches);
        else
        {
          gui_DrawFindPromptMessage("0 matches");

          while (true)
          {
            keypad_IdleKeypadBlock();

            if (keypad_SinglePressExclusive(kb_KeyClear))
              break;
          }
        }

        break;
      }
    }
  }

  return;
}


// Post: Exit editor -> true
//       Do not exit editor -> false
static bool save_changes_prompt(s_editor* const editor)
{
  bool retval = false;
  bool quit = false;
  int8_t save_var_retval;

  gui_DrawSavePrompt();
  gfx_BlitRectangle(1, 0, 220, LCD_WIDTH, 20);

  while (!quit)
  {
    keypad_IdleKeypadBlock();

    if (keypad_SinglePressExclusive(kb_KeyZoom))
      quit = true;
    
    if (keypad_SinglePressExclusive(kb_KeyTrace))
    {
      if (!editor->is_tios_var)
      {
        while (editor->num_changes)
          tool_UndoLastAction(editor);
      }

      retval = true;
      quit = true;
    }

    if (keypad_SinglePressExclusive(kb_KeyGraph))
    {
      if (editor->is_tios_var)
        save_var_retval = tool_SaveModifiedVar(editor);
      else
        save_var_retval = 1;

      if (save_var_retval == 0)
      {
        gui_ErrorWindow("Unable to save changes.");
      }
      else if (save_var_retval == -1)
      {
        gui_MessageWindowBlocking(
          "Fatal Error", "Program will close.$Changes may not be$saved."
        );
        tool_FatalErrorExit();
      }

      retval = true;
      quit = true;
    }
  }
  
  return retval;
}