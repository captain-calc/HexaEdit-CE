// Name:    Captain Calc
// File:    gui.h
// Purpose: Defines the functions declared in gui.h.


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
#include <sys/power.h>
#include <sys/timers.h>
#include <ti/getcsc.h>
#include <ti/vars.h>
#include <assert.h>
#include <debug.h>
#include <fileioc.h>
#include <graphx.h>
#include <keypadc.h>
#include <string.h>
#include <time.h>

#include "ccdbg/ccdbg.h"
#include "asmutil.h"
#include "cutil.h"
#include "gui.h"
#include "hevat.h"
#include "keypad.h"
#include "tools.h"


// Some defines for how characters are spaced in the editor GUI.
#define HEX_COL_WIDTH   (20)
#define ASCII_COL_WIDTH (10)
#define ROW_HEIGHT      (11)

#define LIST_ITEM_PXL_HEIGHT (G_FONT_HEIGHT + 3)


// =============================================================================
// STATIC FUNCTION DECLARATIONS
// =============================================================================


static void print_ascii(uint8_t value, uint24_t xpos, uint8_t ypos);


static void print_hex(uint8_t value, uint24_t xpos, uint8_t ypos);


// Pre: Text colors set.
static void draw_battery_status(void);


// Description: Returns the number of bytes onscreen at any given moment.
static uint8_t get_num_bytes_onscreen(const s_editor* const editor);


static void print_text_wrap(const char* const text);


// =============================================================================
// PUBLIC FUNCTION DEFINITIONS
// =============================================================================


void gui_SetTextColor(uint8_t bg_color, uint8_t fg_color)
{
  gfx_SetTextBGColor(bg_color);
  gfx_SetTextFGColor(fg_color);
  gfx_SetTextTransparentColor(bg_color);
  return;
}


void gui_PrintText(const char* const text, const uint8_t text_color)
{
	const char* character = text;

	gfx_SetColor(text_color);

	while (*character != '\0')
	{
		if (*character == G_HEXAEDIT_THETA)
		{
			gfx_PrintChar('O');
			gfx_HorizLine(gfx_GetTextX() - 6, gfx_GetTextY() + 3, 3);
		}
		else
		{
			gfx_PrintChar(*character);
		};

		character++;
	};

	return;
}


void gui_MessageWindowBlocking(const char* const title, const char* const msg)
{
  const uint8_t WIDTH = 200;
  const uint8_t HEIGHT = 100;
  const uint24_t XPOS = (LCD_WIDTH - WIDTH) / 2;
  const uint8_t YPOS = (LCD_HEIGHT - HEIGHT) / 2;

  gfx_SetColor(g_color.background);
  gfx_FillRectangle_NoClip(XPOS - 2, YPOS - 2, WIDTH + 4, HEIGHT + 4);
  gfx_SetColor(g_color.bar);
  gfx_FillRectangle_NoClip(XPOS, YPOS, WIDTH, HEIGHT);
  gfx_SetColor(g_color.editor_side_panel);
  gfx_FillRectangle_NoClip(XPOS + 2, YPOS + 16, WIDTH - 4, HEIGHT - 18);

  gfx_SetTextXY(XPOS + 4, YPOS + 18);
  gui_SetTextColor(g_color.editor_side_panel, g_color.editor_text_normal);
  print_text_wrap(msg);

  gfx_SetColor(g_color.bar);
  gfx_FillRectangle_NoClip(XPOS + WIDTH - 57, YPOS + HEIGHT - 14, 55, 12);
  gui_SetTextColor(g_color.bar, g_color.bar_text);
  gfx_PrintStringXY(title, XPOS + 3, YPOS + 4);
  gfx_PrintStringXY("[clear]", XPOS + WIDTH - 52, YPOS + HEIGHT - 11);

  gfx_BlitRectangle(1, XPOS - 2, YPOS - 2, WIDTH + 4, HEIGHT + 4);

  do {
    keypad_IdleKeypadBlock();
  } while (!keypad_SinglePressExclusive(kb_KeyClear));
  
  return;
}

void gui_ErrorWindow(const char* const msg)
{
  gui_MessageWindowBlocking("Error", msg);
  return;
}


uint24_t gui_ListAbsOffset(const s_list* const list)
{
  return list->base_offset + list->section_offset;
}


void gui_ResetListOffset(s_list* const list)
{
  list->base_offset = 0;
  list->section_offset = 0;
  return;
}


void gui_DecrementListOffset(s_list* const list)
{
  if (list->section_offset)
    list->section_offset--;
  else if (list->base_offset)
    list->base_offset--;

  return;
}


void gui_IncrementListOffset(s_list* const list)
{
  uint24_t section_end = (
    list->num_items > list->section_length
    ? list->section_length
    : list->num_items
  );

  if (list->section_offset + 1 < section_end)
  {
    list->section_offset++;
  }
  else if (gui_ListAbsOffset(list) + 1 < list->num_items)
  {
    list->base_offset++;
  }

  return;
}


static uint8_t draw_list_item_bg(
  s_list* const list, const bool highlight, uint8_t* const ypos
)
{
  uint8_t text_color;

  if (highlight)
  {
    gfx_SetColor(g_color.list_cursor);
    gui_SetTextColor(g_color.list_cursor, g_color.list_text_selected);
    text_color = g_color.list_text_selected;
  }
  else
  {
    gfx_SetColor(g_color.background);
    gui_SetTextColor(g_color.background, g_color.list_text_normal);
    text_color = g_color.list_text_normal;
  }

  gfx_FillRectangle_NoClip(list->xpos, *ypos, list->width, LIST_ITEM_PXL_HEIGHT);
  gfx_SetTextXY(list->xpos + 1, *ypos + 1);
  *ypos += LIST_ITEM_PXL_HEIGHT;

  return text_color;
}


void gui_DrawList(s_list* const list, const char** item_names)
{
  uint8_t ypos = list->ypos;
  uint24_t abs_offset = gui_ListAbsOffset(list);
  uint24_t end = list->base_offset + (
    list->num_items > list->section_length
    ? list->section_length
    : list->num_items
  );

  gfx_SetColor(g_color.background);
  gfx_FillRectangle_NoClip(list->xpos, list->ypos, list->width, 198);

  for (uint8_t idx = list->base_offset; idx < end; idx++)
  {
    draw_list_item_bg(list, abs_offset == idx, &ypos);
    gfx_PrintString(item_names[idx]);
  }

  return;
}


void gui_DrawHEVATList(s_list* const list, const uint8_t hevat_group_idx)
{
  s_calc_var var;
  char print_name[20] = { '\0' };
  uint8_t text_color;
  uint8_t ypos = list->ypos;
  uint24_t abs_offset = gui_ListAbsOffset(list);
  uint24_t end = list->base_offset + (
    list->num_items > list->section_length
    ? list->section_length
    : list->num_items
  );

  gfx_SetColor(g_color.background);
  gfx_FillRectangle_NoClip(list->xpos, list->ypos, list->width, 198);

  for (uint8_t idx = list->base_offset; idx < end; idx++)
  {
    var.vatptr = hevat_Ptr(hevat_group_idx, idx);
    hevat_GetVarInfoByVAT(&var);
    hevat_VarNameToASCII(print_name, (const uint8_t*)var.name, var.named);
    text_color = draw_list_item_bg(list, abs_offset == idx, &ypos);
    gui_PrintText(print_name, text_color);
  }
  
  return;
}


void gui_DrawSelectedListIndicator(const bool master_list)
{
  gfx_SetColor(g_color.bar);
  gfx_FillRectangle_NoClip((master_list ? 2 : 109), 214, 101, 4);
  return;
}


void gui_EraseHEVATEntryInfo(void)
{
  gfx_SetColor(g_color.background);
  gfx_FillRectangle_NoClip(216, 22, 104, 150);
  return;
}


void gui_DrawHEVATEntryInfo(void* vatptr)
{
CCDBG_BEGINBLOCK("gui_DrawHEVATEntryInfo");
CCDBG_DUMP_PTR(vatptr);

  const uint24_t XPOS = 216;
  const uint8_t START_YPOS = 22;
  const uint8_t DIVIDER_PXL_HEIGHT = LIST_ITEM_PXL_HEIGHT + 1;

  s_calc_var var;
  char ptr[7] = { '\0' };
  uint24_t size;

  for (uint8_t idx = 0; idx < 4; idx++)
  {
    gfx_SetColor(g_color.editor_side_panel);
    gfx_FillRectangle_NoClip(
      XPOS - 2, START_YPOS - 2 + (idx * 2 * DIVIDER_PXL_HEIGHT),
      106,
      DIVIDER_PXL_HEIGHT
    );
  }

  var.vatptr = vatptr;
  hevat_GetVarInfoByVAT(&var);

  gui_SetTextColor(g_color.background, g_color.list_text_normal);
  gfx_SetTextXY(XPOS, START_YPOS);
  gfx_PrintString("OS Type: ");
  gfx_PrintUInt(var.type, cutil_Log10(var.type));

  gfx_SetTextXY(XPOS, START_YPOS + 11);
  gfx_PrintString("VAT: 0x");
  cutil_UintToHex(ptr, (uint24_t)vatptr);
  gfx_PrintString(ptr);

  gfx_SetTextXY(XPOS, START_YPOS + 22);
  gfx_PrintString("Data: 0x");
  cutil_UintToHex(ptr, (uint24_t)var.data);
  gfx_PrintString(ptr);

  size = var.size;

  // Do not include size bytes at the start of programs, protected programs,
  // appvars, and groups.
  if (var.named)
    size -= 2;

  gfx_SetTextXY(XPOS, START_YPOS + 33);
  gfx_PrintString("Size: ");
  gfx_PrintUInt(size, cutil_Log10(size));

  gfx_SetTextXY(XPOS, START_YPOS + 44);
  gfx_PrintString("Location: ");
  gfx_PrintString((var.archived ? "ROM" : "RAM"));

  gfx_SetTextXY(XPOS, START_YPOS + 55);
  gfx_PrintString("Locked: ");
  gfx_PrintString((var.type == OS_TYPE_PROT_PRGM) ? "Yes" : "No");

  gfx_SetTextXY(XPOS, START_YPOS + 66);
  gfx_PrintString("Hidden: ");
  gfx_PrintString((cutil_IsVarHidden(var.name) ? "Yes" : "No"));

CCDBG_ENDBLOCK();
  return;
}


void gui_DrawMemoryAmounts(const s_editor* const editor)
{
  void** free = NULL;
  uint24_t free_ram = os_MemChk(free);

  gui_SetTextColor(g_color.background, g_color.list_text_normal);
  gfx_SetTextXY(216, 189);
  gfx_PrintString("EDB: ");
  gfx_PrintUInt(editor->buffer_size, cutil_Log10(editor->buffer_size));

  gfx_SetTextXY(216, 200);
  gfx_PrintString("RAM: ");
  gfx_PrintUInt(free_ram, cutil_Log10(free_ram));

  os_ArcChk();
  gfx_SetTextXY(216, 211);
  gfx_PrintString("ROM: ");
  gfx_PrintUInt(os_TempFreeArc, cutil_Log10(os_TempFreeArc));

  return;
}


void gui_DrawMainMenuTopBar(uint24_t num_list_items)
{
  gfx_SetColor(g_color.bar);
  gfx_FillRectangle_NoClip(0, 0, LCD_WIDTH, 20);
  
  gui_SetTextColor(g_color.bar, g_color.list_text_normal);
  gfx_PrintStringXY("HexaEdit 3", 4, 5);
  gfx_SetTextFGColor(g_color.bar_text);
  gfx_PrintStringXY("HexaEdit 3", 5, 6);

  gfx_SetTextXY(130, 6);
  gfx_SetTextFGColor(g_color.bar_text);
  gfx_PrintUInt(num_list_items, cutil_Log10(num_list_items));
  gfx_PrintString(" items");

  draw_battery_status();
  return;
}


void gui_DrawMainMenuListDividers(void)
{
  gfx_SetColor(g_color.bar);
  gfx_VertLine_NoClip(105, 20, 200);
  gfx_VertLine_NoClip(106, 20, 200);
  gfx_VertLine_NoClip(212, 20, 200);
  gfx_VertLine_NoClip(213, 20, 200);
  return;
}


void gui_DrawMainMenuBottomBar(void)
{
  gfx_SetColor(g_color.bar);
  gfx_FillRectangle_NoClip(0, 220, LCD_WIDTH, 20);
  gui_SetTextColor(g_color.bar, g_color.bar_text);

  gfx_PrintStringXY("ROM", 5, 226);
  gfx_PrintStringXY("RAM", 68, 226);
  gfx_PrintStringXY("Ports", 138, 226);
  gfx_PrintStringXY("About", 274, 226);
  return;
}


void gui_DrawLocationColumn(const s_editor* const editor)
{
  char address[7] = { '\0' };
  uint24_t offset;
  uint8_t num_bytes_onscreen = get_num_bytes_onscreen(editor);
  uint8_t num_rows_onscreen = (num_bytes_onscreen / G_COLS_ONSCREEN)
                              + (num_bytes_onscreen % G_COLS_ONSCREEN ? 1 : 0);

  gfx_SetColor(g_color.editor_side_panel);
  gfx_FillRectangle_NoClip(0, 20, 60, 200);
  gui_SetTextColor(g_color.editor_side_panel, g_color.editor_text_normal);

  for (uint8_t row = 0; row < num_rows_onscreen; row++)
  {
    gfx_SetTextXY(3, 22 + (row * ROW_HEIGHT));

    offset = editor->window_offset + (row * G_COLS_ONSCREEN);

    if (editor->location_col_mode == 'a')
    {
      cutil_UintToHex(address,(uint24_t)editor->base_address + offset);
      gfx_PrintString(address);
    }
    else
      gfx_PrintUInt(offset, 7);
  }

  return;
}


void gui_PrintData(const s_editor* const editor)
{
  const uint8_t START_HEX_XPOS = 67;
  const uint8_t START_YPOS = 22;
  const uint8_t START_ASCII_XPOS = 237;

  uint24_t hex_xpos = START_HEX_XPOS;
  uint8_t ypos = START_YPOS;
  uint24_t ascii_xpos = START_ASCII_XPOS;
  uint8_t* window_address = editor->base_address + editor->window_offset;
  uint8_t* address = window_address;
  uint24_t address_offset;

  // Take the minimum of the number of bytes that can fit onscreen and the
  // number of bytes in both segments of the edit buffer, starting from the
  // window offset.
  uint8_t count = get_num_bytes_onscreen(editor);

  gfx_SetColor(g_color.background);
  gfx_FillRectangle_NoClip(START_HEX_XPOS - 5, START_YPOS - 2, 168, 200);
  gfx_SetColor(g_color.editor_side_panel);
  gfx_FillRectangle_NoClip(START_ASCII_XPOS - 5, START_YPOS - 2, 88, 200);

  for (uint8_t idx = 0; idx < count; idx++)
  {
    address_offset = address - editor->base_address;

    if (address_offset == editor->near_size)
      address = editor->base_address + editor->buffer_size - editor->far_size;

    gui_SetTextColor(g_color.background, g_color.editor_text_normal);

    if (
      address_offset >= (editor->near_size - editor->selection_size)
      && address_offset < editor->near_size
    )
    {
      gfx_SetColor(g_color.editor_cursor);
      gfx_FillRectangle_NoClip(
        hex_xpos - 1, ypos - 1, HEX_COL_WIDTH, ROW_HEIGHT
      );
      gfx_FillRectangle_NoClip(
        ascii_xpos - 1, ypos - 1, ASCII_COL_WIDTH, ROW_HEIGHT
      );
      gui_SetTextColor(g_color.editor_cursor, g_color.editor_text_selected);
    }

    if (editor->window_offset + idx + 1 == editor->near_size)
    {
      gfx_SetColor(g_color.editor_text_normal);
      gfx_HorizLine_NoClip(
        hex_xpos - 1 + (9 * !editor->high_nibble), ypos + G_FONT_HEIGHT + 1, 9
      );
    }

    gfx_SetColor(g_color.editor_text_normal);
    print_hex(*address, hex_xpos, ypos);
    print_ascii(*address, ascii_xpos, ypos);

    hex_xpos += HEX_COL_WIDTH;
    ascii_xpos += ASCII_COL_WIDTH;

    if (idx && ((idx % G_COLS_ONSCREEN) == (G_COLS_ONSCREEN - 1)))
    {
      hex_xpos = START_HEX_XPOS;
      ascii_xpos = START_ASCII_XPOS;
      ypos += ROW_HEIGHT;
    }

    address++;
  }

  return;
}


void gui_DrawTitleBar(const s_editor* const editor)
{
  char name[G_EDITOR_NAME_MAX_LEN] = { '\0' };
  uint8_t cutcopy_buffer_size = tool_GetCutCopyBufferSize();

  gfx_SetColor(g_color.bar);
  gfx_FillRectangle_NoClip(0, 0, LCD_WIDTH, 20);
  gui_SetTextColor(g_color.bar, g_color.bar_text);

  gfx_SetTextXY(5, 6);

  if (editor->num_changes)
    gfx_PrintString("* ");

  if (editor->is_tios_var)
  {
    hevat_VarNameToASCII(
      name,
      (const uint8_t*)editor->name,
      asmutil_IsNamedVar(editor->tios_var_type)
    );
  }
  else
    strncpy(name, editor->name, editor->name_length);

  gui_PrintText(name, g_color.bar_text);

  gfx_SetTextXY(100, 6);
  gfx_PrintUInt(editor->data_size, cutil_Log10(editor->data_size));
  gfx_PrintString(" B");

  gfx_SetTextXY(190, 6);
  gfx_PrintChar(editor->access_type);
  gfx_PrintString(" | ");
  gfx_PrintChar(editor->writing_mode);
  gfx_PrintString(" | ");
  gfx_PrintUInt(cutcopy_buffer_size, cutil_Log10(cutcopy_buffer_size));
  gfx_PrintString(" | ");
  gfx_PrintUInt(editor->selection_size, cutil_Log10(editor->selection_size));

  draw_battery_status();
  return;
}


void gui_DrawToolBar(const s_editor* const editor)
{
  typedef struct
  {
    char* name;
    uint24_t xpos;
    void* func;
  } s_toolbar_tool;

  const uint8_t NUM_SELECTION_INACTIVE_TOOLS = 5;
  const s_toolbar_tool SELECTION_INACTIVE_TOOLS[] = {
    { .name = "Find", .xpos = 5, .func = tool_FindPhrase },
    { .name = "Insert", .xpos = 62, .func = tool_InsertBytes },
    { .name = "Goto", .xpos = 138, .func = tool_Goto },
    { .name = "Undo", .xpos = 224, .func = tool_UndoLastAction },
    { .name = "wMODE", .xpos = 277, .func = tool_SwitchWritingMode },
  };

  const uint8_t NUM_SELECTION_ACTIVE_TOOLS = 3;
  const s_toolbar_tool SELECTION_ACTIVE_TOOLS[] = {
    { .name = "Copy", .xpos = 138, .func = tool_CopyBytes },
    { .name = "Cut", .xpos = 224, .func = tool_CutBytes },
    { .name = "Paste", .xpos = 277, .func = tool_PasteBytes },
  };

  const s_toolbar_tool* tools = SELECTION_INACTIVE_TOOLS;
  uint8_t num_tools = NUM_SELECTION_INACTIVE_TOOLS;
  uint24_t decimal;

  gfx_SetColor(g_color.bar);
  gfx_FillRectangle_NoClip(0, 220, LCD_WIDTH, 20);
  gui_SetTextColor(g_color.bar, g_color.bar_text_dark);

  if (editor->selection_active)
  {
    tools = SELECTION_ACTIVE_TOOLS;
    num_tools = NUM_SELECTION_ACTIVE_TOOLS;

    if (editor->near_size && editor->selection_size <= sizeof(uint24_t))
    {
      gfx_SetTextFGColor(g_color.bar_text);
      gfx_PrintStringXY("Decimal: ", 5, 226);
      decimal = 0;

      for (uint8_t idx = 0; idx < editor->selection_size; idx++)
      {
        decimal += (
          *(
            editor->base_address + editor->near_size
            - editor->selection_size + idx
          ) << (8 * idx)
        );
      }
      
      gfx_PrintUInt(decimal, cutil_Log10(decimal));
    }
  }

  for (uint8_t idx = 0; idx < num_tools; idx++)
  {
    gfx_SetTextFGColor(g_color.bar_text_dark);

    if (tool_IsAvailable(editor, tools[idx].func))
      gfx_SetTextFGColor(g_color.bar_text);

    gfx_PrintStringXY(tools[idx].name, tools[idx].xpos, 226);
  }

  return;
}


void gui_DrawFindPromptMessage(const char* const message)
{
  uint24_t width = gfx_GetStringWidth(message);

  gui_SetTextColor(g_color.bar, g_color.bar_text);
  gfx_FillRectangle_NoClip(315 - width, 226, G_FONT_HEIGHT, width);
  gfx_PrintStringXY(message, 315 - width, 226);
  gfx_BlitRectangle(1, 0, 220, LCD_WIDTH, 20);
  return;
}


void gui_DrawFindPhraseToolbar(
  const uint8_t match_idx, const uint8_t num_matches
)
{
  uint24_t actual_match_idx = match_idx + (num_matches ? 1 : 0);

  gfx_SetColor(g_color.bar);
  gfx_FillRectangle_NoClip(0, 220, LCD_WIDTH, 20);
  gui_SetTextColor(g_color.bar, g_color.bar_text);

  gfx_SetTextXY(5, 226);
  gfx_PrintUInt(actual_match_idx, cutil_Log10(actual_match_idx));
  gfx_PrintString(" / ");
  gfx_PrintUInt(num_matches, cutil_Log10(num_matches));
  return;
}


void gui_DrawSavePrompt(void)
{
  gfx_SetColor(g_color.bar);
  gfx_FillRectangle_NoClip(0, 220, LCD_WIDTH, 20);
  gui_SetTextColor(g_color.bar, g_color.bar_text);

  gfx_PrintStringXY("Save changes?", 5, 226);
  gfx_PrintStringXY("Cancel", 132, 226);
  gfx_PrintStringXY("No", 226, 226);
  gfx_PrintStringXY("Yes", 285, 226);
  return;
}


void gui_DrawInputPrompt(
  const char* const prompt, const uint24_t input_field_width
)
{
  uint24_t x = gfx_GetStringWidth(prompt) + 10;
  
  gfx_SetColor(g_color.bar);
  gfx_FillRectangle_NoClip(0, LCD_HEIGHT - 20, LCD_WIDTH, 20);
  
  gui_SetTextColor(g_color.bar, g_color.bar_text);
  gfx_PrintStringXY(prompt, 5, 226);
  
  gfx_SetColor(g_color.list_text_normal);
  gfx_FillRectangle_NoClip(x, 223, input_field_width, G_FONT_HEIGHT + 6);
  
  gfx_SetColor(g_color.background);
  gfx_FillRectangle_NoClip(
    x + 1, 224, input_field_width - 2, G_FONT_HEIGHT + 4
  );
  
  return;
}


void gui_DrawKeymapIndicator(
  const char indicator, const uint24_t x_pos, const uint8_t y_pos
)
{
  gfx_SetColor(g_color.list_cursor);
  gfx_FillRectangle_NoClip(
    x_pos, y_pos, gfx_GetCharWidth(indicator) + 3, G_FONT_HEIGHT + 6
  );
  
  gui_SetTextColor(g_color.list_cursor, g_color.list_text_selected);
  gfx_SetTextXY(x_pos + 2, y_pos + 3);
  gfx_PrintChar(indicator);
  return;
}


void gui_Input(
  char* const buffer,
  const uint8_t buffer_size,
  const uint24_t x_pos,
  const uint8_t y_pos,
  const uint24_t width,
  const char* const keymap[7]
)
{
  uint8_t value;
  uint8_t offset = strlen(buffer);
  
  gfx_SetTextXY(x_pos + 2, y_pos + 2);
  gui_PrintText(buffer, g_color.list_cursor);
  gfx_FillRectangle_NoClip(
    x_pos + gfx_GetStringWidth(buffer) + 2, y_pos + 1, 2, G_FONT_HEIGHT + 2
  );  
  gfx_BlitRectangle(1, x_pos, y_pos, width, G_FONT_HEIGHT + 4);
  gfx_SetColor(g_color.list_cursor);

  do {
    kb_Scan();
  } while (!kb_AnyKey());

  // The [del] and [clear] key checks must fall through in case the calling
  // function redefines the purpose of those keys.
  if (kb_IsDown(kb_KeyDel) && offset)
    buffer[--offset] = '\0';

  if (kb_IsDown(kb_KeyClear))
    memset(buffer, '\0', buffer_size);

  if (!keypad_ExclusiveKeymap(keymap, &value) && offset < buffer_size)
    buffer[offset++] = value;

  delay(150);

  return;
}


// =============================================================================
// STATIC FUNCTION DEFINITIONS
// =============================================================================


static void print_ascii(uint8_t value, uint24_t xpos, uint8_t ypos)
{
  gfx_SetTextXY(xpos, ypos);

  if (value < 32 || value > 127)
    gfx_Rectangle_NoClip(xpos, ypos + 1, 6, 6);
  else
    gfx_PrintChar(value);

  return;
}


static void print_hex(uint8_t value, uint24_t xpos, uint8_t ypos)
{
  const char HEX_CHARS[] = "0123456789abcdef";

  char hex[3] = { '\0' };

  hex[0] = HEX_CHARS[value / 16];
  hex[1] = HEX_CHARS[value % 16];
  gfx_PrintStringXY(hex, xpos, ypos);
  return;
}


static void draw_battery_status(void)
{
  const double FIVE_MIN_IN_SEC = 300;

  static uint8_t charging = 0;
  static uint8_t percentage = 0;
  static double prev_clock = 0;
  double curr_clock = clock();

  if (
    !prev_clock
    || ((curr_clock - prev_clock) / CLOCKS_PER_SEC) > FIVE_MIN_IN_SEC)
  {
    prev_clock = curr_clock;
    percentage = 25 * boot_GetBatteryStatus();
    charging = boot_BatteryCharging();
  }

  gfx_SetTextXY(
    315 - gfx_GetCharWidth('%')
    - (cutil_Log10(percentage) * gfx_GetCharWidth('0'))
    - (charging ? gfx_GetStringWidth(" CHR") : 0),
    6
  );
  gfx_PrintUInt(percentage, cutil_Log10(percentage));
  gfx_PrintString("% ");

  if (charging)
    gfx_PrintString("CHR");

  return;
}


static uint8_t get_num_bytes_onscreen(const s_editor* const editor)
{
  uint8_t count = min(
    editor->data_size - editor->window_offset,
    G_NUM_BYTES_ONSCREEN
  );

  return count;
}


static void print_text_wrap(const char* const text)
{
  const char* c = text;
  uint24_t orig_xpos = gfx_GetTextX();

  while (*c != '\0')
  {
    if (*c == '$')
      gfx_SetTextXY(orig_xpos, gfx_GetTextY() + 9);
    else
      gfx_PrintChar(*c);

    c++;
  }

  return;
}
