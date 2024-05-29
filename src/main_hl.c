// Name:    Captain Calc
// File:    main_hl.h
// Purpose: Defines the functions declared in main_hl.h.


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


#include <ti/vars.h>
#include <assert.h>
#include <fileioc.h>
#include <hevat.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "ccdbg/ccdbg.h"
#include "editor.h"
#include "gui.h"
#include "main_hl.h"


enum HEADER_FLAGS : uint8_t
{
  COLORSCHEME_PRESENT = 1 << 0,
  MEMORY_EDITOR       = 1 << 1,
  VARIABLE_EDITOR    = 1 << 2,
};

enum MEMORY_EDITOR_FLAGS : uint8_t
{
  RAM_EDITOR = 0,
  PORTS_EDITOR,
  ROM_VIEWER
};

typedef struct
{
  char header[8];
  uint8_t flags;
} s_header;

typedef struct
{
  uint8_t flag;
  uint24_t cursor_offset;
} s_mem_editor;

typedef struct
{
  char name[8];
  uint8_t name_length;
  uint8_t type;
  uint24_t cursor_offset;
} s_var_editor;


// File globals. Do NOT use these outside of this file.
const char* ANS_CONFIG_HEADER = "HexaEdit";
uint8_t g_ans_config[40] = { '\0' };


// =============================================================================
// STATIC FUNCTION DECLARATIONS
// =============================================================================


static void write_header(const uint8_t flags, const uint8_t handle)
{
  ti_Write("HexaEdit", 8, 1, handle);
  ti_Write(&flags, 1, 1, handle);
  return;
}


static void write_mem_editor(
  const s_mem_editor* const mem_editor, const uint8_t handle
)
{
  ti_Write(mem_editor, sizeof(s_mem_editor), 1, handle);
  return;
}


static void write_var_editor(
  const s_var_editor* const var_editor, const uint8_t handle
)
{
  ti_Write(var_editor, sizeof(s_var_editor), 1, handle);
  return;
}


static void write_colorscheme(
  const s_color* const colorscheme, const uint8_t handle
)
{
  ti_Write(colorscheme, sizeof(s_color), 1, handle);
  return;
}


static uint8_t read_flags(void)
{
  s_header* header = (s_header*)g_ans_config;

  return header->flags;
}


static s_mem_editor* read_mem_editor(void)
{
  return (s_mem_editor*)(g_ans_config + sizeof(s_header));
}


static s_var_editor* read_var_editor(void)
{
  return (s_var_editor*)(g_ans_config + sizeof(s_header));
}


static s_color* read_colorscheme(void)
{
  uint8_t flags = read_flags();

  if (flags & MEMORY_EDITOR)
  {
    return (s_color*)(g_ans_config + sizeof(s_header) + sizeof(s_mem_editor));
  }
  else if (flags & VARIABLE_EDITOR)
  {
    return (s_color*)(g_ans_config + sizeof(s_header) + sizeof(s_var_editor));
  }

  return NULL;
}


// =============================================================================
// PUBLIC FUNCTION DECLARATIONS
// =============================================================================

void mainhl_SetMemEditor(void)
{
  uint8_t handle;
  s_mem_editor mem_editor = {
    .flag = RAM_EDITOR,
    .cursor_offset = 0
  };
  s_color colorscheme = {
    .bar = 0,
    .bar_text = 127,
    .bar_text_dark = 130,
    .background = 200,
    .editor_side_panel = 130,
    .editor_cursor = 0x3d,
    .editor_text_normal = 0,
    .editor_text_selected = 127
  };

  if ((handle = ti_OpenVar(OS_VAR_ANS, "w", OS_TYPE_STR)))
  {
    write_header(MEMORY_EDITOR | COLORSCHEME_PRESENT, handle);
    write_mem_editor(&mem_editor, handle);
    write_colorscheme(&colorscheme, handle);
    ti_Close(handle);
  }

  return;
}


void mainhl_SetVarEditor(void)
{
  uint8_t handle;
  s_var_editor var_editor = {
    .name = OS_VAR_L1,
    .name_length = 3,
    .type = OS_TYPE_REAL_LIST,
    .cursor_offset = 0
  };
  s_color colorscheme = {
    .bar = 0,
    .bar_text = 127,
    .bar_text_dark = 130,
    .background = 200,
    .editor_side_panel = 130,
    .editor_cursor = 0x3d,
    .editor_text_normal = 0,
    .editor_text_selected = 127
  };

  if ((handle = ti_OpenVar(OS_VAR_ANS, "w", OS_TYPE_STR)))
  {
    write_header(VARIABLE_EDITOR | COLORSCHEME_PRESENT, handle);
    write_var_editor(&var_editor, handle);
    write_colorscheme(&colorscheme, handle);
    ti_Close(handle);
  }
}


bool mainhl_CheckAns(void)
{
CCDBG_BEGINBLOCK("mainhl_CheckAns");

  bool retval = false;
  uint8_t handle;

  if ((handle = ti_OpenVar(OS_VAR_ANS, "r", OS_TYPE_STR)))
  {

    if (
      !memcmp(
        ti_GetDataPtr(handle), ANS_CONFIG_HEADER, strlen(ANS_CONFIG_HEADER)
      )
    )
    {
      // We load the contents of Ans into an array that is global to this file.
      // This allows us to open Ans only once, reducing the number of file
      // operations that could potentially fail.
      ti_Read(g_ans_config, ti_GetSize(handle), 1, handle);
      retval = true;
    }

    ti_Close(handle);
  }

CCDBG_DUMP_PTR(g_ans_config);
CCDBG_DUMP_UINT(retval);
CCDBG_ENDBLOCK();

  return retval;
}


int mainhl_RunEditor(s_editor* const editor)
{
CCDBG_BEGINBLOCK("mainhl_RunEditor");

  const char* const NAME_ARRAY[] = { "RAM", "Ports", "ROM" };
  s_mem_editor* mem_editor;
  s_var_editor* var_editor;
  s_calc_var var;
  const char* name = NULL;
  uint8_t* base_address = NULL;
  uint24_t size = 0;
  uint8_t flags = read_flags();
  bool var_opened = false;
  int retval = 0;

CCDBG_DUMP_UINT(flags);

  if (flags & COLORSCHEME_PRESENT)
    memcpy(&g_color, read_colorscheme(), sizeof(s_color));

  if (flags & MEMORY_EDITOR)
  {
    mem_editor = read_mem_editor();

    switch (mem_editor->flag)
    {
      case RAM_EDITOR:
        name = NAME_ARRAY[0];
        base_address = G_RAM_BASE_ADDRESS;
        size = G_RAM_SIZE;

CCDBG_PUTS("Is RAM editor.");

        break;

      case PORTS_EDITOR:
        name = NAME_ARRAY[1];
        base_address = G_PORTS_BASE_ADDRESS;
        size = G_PORTS_SIZE;

CCDBG_PUTS("Is Ports Editor.");

        break;

      case ROM_VIEWER:
        name = NAME_ARRAY[2];
        base_address = G_ROM_BASE_ADDRESS;
        size = G_ROM_SIZE;

CCDBG_PUTS("Is ROM viewer.");

        break;

      default:
        assert(false);
    }

    if (mem_editor->cursor_offset < size)
    {
      editor_OpenMemEditor(
        editor, name, base_address, size, mem_editor->cursor_offset
      );
    }
  }
  else if (flags & VARIABLE_EDITOR)
  {
    var_editor = read_var_editor();

    var_opened = hevat_GetVarInfoByNameAndType(
      &var,
      var_editor->name,
      var_editor->name_length,
      var_editor->type
    );

CCDBG_PUTS("Is variable editor.");
CCDBG_DUMP_UINT(var_opened);

    if (var_opened)
      editor_OpenVarEditor(editor, var.vatptr, var_editor->cursor_offset);
    else
    {
      gui_ErrorWindow("Variable could not$be opened.");
      retval = 1;
    }
  }

  ti_DeleteVar(OS_VAR_ANS, OS_TYPE_STR);
CCDBG_ENDBLOCK();

  return retval;
}
