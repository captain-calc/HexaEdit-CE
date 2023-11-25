// Name:    Captain Calc
// File:    tools.c
// Purpose: Defines the functions declared in tools.h.


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


#include <debug.h>
#include <assert.h>
#include <fileioc.h>
#include <string.h>

#include "ccdbg/ccdbg.h"
#include "asmutil.h"
#include "defines.h"
#include "hevat.h"
#include "tools.h"


#define MAX_VAR_DATA_SIZE (65504)


enum UNDO_ACTION_CODES : uint8_t
{
  UNDO_WRITE_NIBBLE = 200,
  UNDO_WRITE_BYTE,
  UNDO_INSERT_BYTES,
  UNDO_PASTE_BYTES,
  UNDO_DELETE_OR_CUT_BYTES
};


// File globals. Do NOT use these outside of this file.
static uint8_t g_cutcopy_buffer[G_MAX_SELECTION_SIZE];
static uint8_t g_cutcopy_buffer_size = 0;

static uint8_t g_undo_stack[30000];
static uint8_t* g_undo_sp = g_undo_stack + (sizeof g_undo_stack) - 1;


// =============================================================================
// STATIC FUNCTION DECLARATIONS
// =============================================================================


static bool archive_edit_buffer(void);


static bool shrink_edit_buffer(const s_editor* const editor);


static bool save_edited_var(s_calc_var* const var);


static bool undo_buffer_has_room(const uint24_t size);


static void addundo_delete_or_cut_or_paste_bytes(
  s_editor* const editor, const uint8_t code
);


// =============================================================================
// PUBLIC FUNCTION DEFINITIONS
// =============================================================================


void tool_FatalErrorExit(void)
{
  tool_DeleteEditBuffer();
  exit(1);
}


uint8_t tool_GetCutCopyBufferSize()
{
  return g_cutcopy_buffer_size;
}


void tool_InitUndoStack(void)
{
CCDBG_BEGINBLOCK("tool_InitUndoStack");
CCDBG_DUMP_PTR(g_undo_stack);

  g_undo_sp = g_undo_stack + (sizeof g_undo_stack) - 1;

CCDBG_DUMP_PTR(g_undo_sp);
CCDBG_ENDBLOCK();

  return;
}


bool tool_CreateRecentsAppvar(void)
{
CCDBG_BEGINBLOCK("tool_CreateRecentsAppvar");

  uint8_t handle;

  if ((handle = ti_Open(G_RECENTS_APPVAR_NAME, "r")))
    ti_Close(handle);
  else if ((handle = ti_Open(G_RECENTS_APPVAR_NAME, "w")))
  {
    if (ti_Resize(255, handle))
    {
      memset(ti_GetDataPtr(handle), '\0', 255);
      ti_Close(handle);
    }
    else
    {
CCDBG_PUTS("Could not resize recents appvar");
CCDBG_ENDBLOCK();

      ti_Close(handle);
      return false;
    }
  }
  else
  {
CCDBG_PUTS("Could not open/create recents appvar");
CCDBG_ENDBLOCK();

    return false;
  }

CCDBG_ENDBLOCK();

  return true;
}


bool tool_CreateEditBuffer(s_editor* const editor)
{
CCDBG_BEGINBLOCK("create_edit_buffer");

  void* free = NULL;
  uint8_t handle;
  size_t free_ram;

  if (!(handle = ti_Open(G_EDIT_BUFFER_APPVAR_NAME, "w")))
  {
CCDBG_PUTS("Cannot open edit buffer appvar.");
CCDBG_ENDBLOCK();
    return false;
  }

  free_ram = os_MemChk(&free);
  editor->buffer_size = min(MAX_VAR_DATA_SIZE, free_ram);

  if (ti_Resize(editor->buffer_size, handle) <= 0)
  {
CCDBG_PUTS("Resize failed.");
CCDBG_ENDBLOCK();
    return false;
  }

  ti_Close(handle);

CCDBG_DUMP_UINT(editor->buffer_size);
CCDBG_ENDBLOCK();

  return true;
}


void* tool_EditBufferPtr(uint24_t* size)
{
  void* data = NULL;
  uint8_t handle;

  if ((handle = ti_Open(G_EDIT_BUFFER_APPVAR_NAME, "r")))
  {
    data = ti_GetDataPtr(handle);
    *size = ti_GetSize(handle);
    ti_Close(handle);
  }

  return data;
}


bool tool_CheckFreeRAM(const uint24_t amt)
{
  void* free = NULL;

  return (os_MemChk(free) > amt);
}


int8_t tool_SaveModifiedVar(s_editor* const editor)
{
CCDBG_BEGINBLOCK("tool_SaveModifiedVar");

  s_calc_var var;
  uint8_t var_type = editor->tios_var_type;

  if (
    !hevat_GetVarInfoByNameAndType(
      &var, editor->name, editor->name_length, editor->tios_var_type
    )
  )
  {
CCDBG_PUTS("Could not get variable info");
CCDBG_ENDBLOCK();

    return 0;
  }

  if (
    var_type == OS_TYPE_PRGM
    || var_type == OS_TYPE_PROT_PRGM
    || var_type == OS_TYPE_APPVAR
  )
  {
CCDBG_PUTS("Saving variable that can be resized");

    if (shrink_edit_buffer(editor))
    {
CCDBG_DUMP_UINT(tool_CheckFreeRAM(editor->data_size));

      if (!tool_CheckFreeRAM(editor->data_size))
      {
        if (!archive_edit_buffer())
          return 0;
      }

      if (!save_edited_var(&var))
      {
CCDBG_PUTS("Variable not saved");
        return 0;
      }

      tool_DeleteEditBuffer();

      if (!tool_CreateEditBuffer(editor))
      {
CCDBG_PUTS("FATAL: Could not create edit buffer");
        return -1;
      }
    }
    else
    {
CCDBG_PUTS("Edit buffer not shrunk");
    }
  }
  else
  {
CCDBG_PUTS("Saving variable that cannot be resized");
CCDBG_DUMP_PTR(var.data);

    tool_LoadEditBufferIntoVar(editor, var.data);
  }

CCDBG_ENDBLOCK();

  return 1;
}


bool tool_BufferVarData(
  s_editor* const editor,
  void* data,
  const uint24_t size,
  const uint24_t offset
)
{
CCDBG_BEGINBLOCK("tool_BufferVarData");
CCDBG_DUMP_PTR(editor->base_address);
CCDBG_DUMP_UINT(editor->buffer_size);

  assert(editor->base_address);
  assert(editor->buffer_size);

  editor->data_size = size;

CCDBG_DUMP_UINT(editor->data_size);

  if (size > editor->buffer_size)
  {
CCDBG_PUTS("Buffer size too small for variable.");
CCDBG_ENDBLOCK();
    return false;
  }

  if (!size)
    editor->near_size = 0;
  else if (editor->selection_size)
    editor->near_size = offset + editor->selection_size;
  else
    editor->near_size = offset + 1;

  assert(editor->near_size <= size);

  editor->far_size = size - editor->near_size;

CCDBG_DUMP_UINT(editor->near_size);
CCDBG_DUMP_UINT(editor->far_size);
CCDBG_PUTS("About to copy data");

  asmutil_CopyData(data, editor->base_address, editor->near_size, 1);
  asmutil_CopyData(
    data + size - 1,
    editor->base_address + editor->buffer_size - 1,
    editor->far_size,
    0
  );

CCDBG_DUMP_UINT(editor->data_size);
CCDBG_ENDBLOCK();
  return true;
}


void tool_LoadEditBufferIntoVar(s_editor* const editor, uint8_t* const var_data)
{
  asmutil_CopyData(editor->base_address, var_data, editor->near_size, 1);
  asmutil_CopyData(
    editor->base_address + editor->buffer_size - editor->far_size,
    var_data + editor->near_size,
    editor->far_size,
    1
  );

  return;
}


void tool_DeleteEditBuffer(void)
{
#ifndef NDEBUG
  uint8_t handle = ti_Open(G_EDIT_BUFFER_APPVAR_NAME, "r");
  assert(handle != 0);
  assert(ti_Close(handle) != 0);
#endif

  int deleted; (void)deleted;

  deleted = ti_Delete(G_EDIT_BUFFER_APPVAR_NAME);

  // If this assert() fires, the delete failed.
  assert(deleted != 0);
  return;
}


bool tool_IsAvailable(const s_editor* const editor, void* const tool_func_ptr)
{
  const uint8_t NUM_TOOLS = 11;

  void* tools[] = {
    &tool_WriteNibble,
    &tool_WriteByte,
    &tool_Goto,
    &tool_InsertBytes,
    &tool_DeleteBytes,
    &tool_CopyBytes,
    &tool_CutBytes,
    &tool_PasteBytes,
    &tool_FindPhrase,
    &tool_SwitchWritingMode,
    &tool_UndoLastAction
  };
  bool available = false;
  uint8_t idx;

  for (idx = 0; idx < NUM_TOOLS; idx++)
  {
    if (tools[idx] == tool_func_ptr)
      break;
  }

  assert(idx < NUM_TOOLS);

  switch (idx)
  {
    case 0:  // tool_WriteNibble
      if (
        !editor->selection_active
        && editor->near_size
        && editor->writing_mode == 'x'
      )
      {
        available = true;
      }
      break;

    case 1: // tool_WriteByte
      if (
        !editor->selection_active
        && editor->near_size
        && editor->writing_mode != 'x'
      )
      {
        available = true;
      }
      break;

    case 2: // tool_Goto
      if (!editor->selection_active && editor->near_size)
        available = true;
      break;

    case 3: // tool_InsertBytes
      if (
        editor->is_tios_var
        && editor->access_type == 'i'
        && !editor->selection_active
      )
      {
        available = true;
      }
      break;

    case 4: // tool_DeleteBytes
      if (
        editor->is_tios_var && editor->access_type == 'i' && editor->near_size
      )
      {
        available = true;
      }
      break;

    case 5: // tool_CopyBytes
      if (
        editor->selection_active
        && editor->selection_size
        && editor->near_size >= editor->selection_size
      )
      {
        available = true;
      }
      break;

    case 6: // tool_CutBytes
      if (
        editor->is_tios_var
        && editor->access_type == 'i'
        && editor->selection_active
        && editor->selection_size
        && editor->near_size >= editor->selection_size
      )
      {
        available = true;
      }
      break;

    case 7: // tool_PasteBytes
      if (
        (editor->access_type == 'w' || editor->access_type == 'i')
        && g_cutcopy_buffer_size
        && (editor->selection_size == g_cutcopy_buffer_size)
      )
      {
        available = true;
      }
      break;

    case 8: // tool_FindPhrase
      if (
        !editor->selection_active && editor->data_size >= G_NUM_BYTES_ONSCREEN
      )
      {
        available = true;
      }
      break;

    case 9: // tool_SwitchWritingMode
      if (!editor->selection_active)
        available = true;
      break;

    case 10: // tool_UndoLastAction
      if (
        !editor->selection_active
        && editor->undo_buffer_active
        && editor->num_changes
      )
      {
        available = true;
      }
      break;

    default:
      break;
  }

  return available;
}


void tool_MoveCursor(
  s_editor* const editor, const uint8_t direction, const uint24_t amt
)
{
  uint24_t copy_size = amt;
  uint24_t copy_src_offset = 0;
  uint24_t copy_dest_offset = 0;

  if (direction == 0)
  {
    // editor->near_size is guaranteed to be at least one IF the edit buffer
    // has data in it.
    if (editor->data_size)
      copy_size = min(editor->near_size - 1, copy_size);
    else
      copy_size = min(editor->near_size, copy_size);

    editor->near_size -= copy_size;
    editor->far_size += copy_size;
    copy_src_offset = editor->near_size;
    copy_dest_offset = editor->buffer_size - editor->far_size;

    if (editor->selection_active)
      editor->selection_size -= min(editor->selection_size, copy_size);
  }
  else
  {
    copy_size = min(editor->far_size, copy_size);

    if (editor->selection_active)
    {
      // The min() prevents users from moving past the end of the selected data
      // if they have reached the selection limit.
      copy_size = min(G_MAX_SELECTION_SIZE - editor->selection_size, copy_size);
      editor->selection_size += copy_size;
    }

    editor->near_size += copy_size;
    editor->far_size -= copy_size;
    copy_src_offset = editor->buffer_size - editor->far_size - copy_size;
    copy_dest_offset = editor->near_size - copy_size;
  }

  if (editor->is_tios_var)
  {
    asmutil_CopyData(
      editor->base_address + copy_src_offset,
      editor->base_address + copy_dest_offset,
      copy_size,
      1
    );
  }

  // Any time the cursor is moved, reset the nibble selector to the high nibble.
  editor->high_nibble = true;

  if (!editor->selection_size)
  {
    editor->selection_active = false;
    editor->selection_size = 1;
  }

  return;
}


bool tool_UpdateWindowOffset(s_editor* const editor)
{
  uint24_t old_window_offset = editor->window_offset;

  // Special case for empty variables.
  if (!editor->near_size)
    editor->window_offset = 0;

  while (editor->near_size && editor->near_size < (editor->window_offset + 1))
    editor->window_offset -= G_COLS_ONSCREEN;

  while (editor->near_size > (editor->window_offset + G_NUM_BYTES_ONSCREEN))
    editor->window_offset += G_COLS_ONSCREEN;

  return (old_window_offset != editor->window_offset);
}


void tool_WriteNibble(s_editor* const editor, uint8_t nibble)
{
  assert(editor->near_size);
  assert(!editor->selection_active);
  assert(editor->writing_mode == 'x');

  uint8_t* address = editor->base_address + editor->near_size - 1;

	if (editor->high_nibble)
	{
		nibble *= 16;
		*address &= 0x0f;
	}
  else
    *address &= 0xf0;

	*address |= nibble;
	return;
}


void tool_WriteByte(s_editor* const editor, uint8_t byte)
{
  assert(editor->near_size);
  assert(!editor->selection_active);
  assert(editor->writing_mode != 'x');

  uint8_t* address = editor->base_address + editor->near_size - 1;
  *address = byte;
  return;
}


void tool_Goto(s_editor* const editor, uint24_t offset)
{
CCDBG_BEGINBLOCK("tool_Goto");

  assert(!editor->selection_active);
  assert(editor->near_size);

  uint24_t diff;
  uint8_t direction;

  offset = min(editor->data_size - 1, offset);

  if (offset > (editor->near_size - 1))
  {
    diff = offset - editor->near_size + 1;
    direction = 1;
  }
  else
  {
    diff = editor->near_size - 1 - offset;
    direction = 0;
  }

CCDBG_DUMP_UINT(diff);

  tool_MoveCursor(editor, direction, diff);

CCDBG_ENDBLOCK();

  // We could get away with just the middle assert(), but the other two provide
  // valuable debugging info.
  assert(editor->near_size);
  assert(editor->near_size == offset + 1);
  assert(editor->near_size <= editor->data_size);
  return;
}


void tool_InsertBytes(s_editor* const editor, const uint24_t num)
{
  assert(editor->is_tios_var);
  assert(editor->access_type == 'i');
  assert(!editor->selection_active);

  uint24_t bytes_inserted = min(editor->buffer_size - editor->data_size, num);

  editor->data_size += bytes_inserted;

  // Empty variable.
  if (!editor->near_size)
  {
    editor->near_size = 1;
  }
  else
  {
    // Copy the byte at the cursor to the far buffer.
    editor->far_size++;
    *(editor->base_address + editor->buffer_size - editor->far_size) = *(
      editor->base_address + editor->near_size - 1
    );
  }

  // The near buffer always consumes one of the inserted bytes.
  bytes_inserted--;
  editor->far_size += bytes_inserted;

  // Set the inserted bytes to 0x00.
  *(editor->base_address + editor->near_size - 1) = 0x00;
  memset(
    editor->base_address + editor->buffer_size - editor->far_size,
    0x00,
    bytes_inserted
  );

  return;
}


void tool_DeleteBytes(s_editor* const editor)
{
CCDBG_BEGINBLOCK("tool_DeleteBytes");

  assert(editor->is_tios_var);
  assert(editor->access_type == 'i');
  assert(editor->near_size);

  editor->data_size -= editor->selection_size;

CCDBG_DUMP_UINT(editor->selection_size);

  // Algorithm:
  //   - Reduce the near buffer's size by <selection_size>
  //   - If the far buffer has bytes, move its first byte to the end of the near
  //     buffer.

  editor->near_size -= editor->selection_size;

  if (editor->far_size)
  {
    *(editor->base_address + editor->near_size) = *(
      editor->base_address + editor->buffer_size - editor->far_size
    );
    editor->near_size++;
    editor->far_size--;
  }

  editor->selection_active = false;
  editor->selection_size = 1;

CCDBG_PUTS("Returning...");
CCDBG_DUMP_UINT(editor->near_size);
CCDBG_DUMP_UINT(editor->far_size);
CCDBG_DUMP_UINT(editor->data_size);
CCDBG_ENDBLOCK();
  return;
}


void tool_CopyBytes(const s_editor* const editor)
{
  assert(editor->selection_active);
  assert(editor->selection_size);
  assert(editor->near_size >= editor->selection_size);

  asmutil_CopyData(
    editor->base_address + editor->near_size - editor->selection_size,
    g_cutcopy_buffer,
    editor->selection_size,
    1
  );

  g_cutcopy_buffer_size = editor->selection_size;
  return;
}


void tool_CutBytes(s_editor* const editor)
{
  assert(editor->is_tios_var);
  assert(editor->access_type == 'i');
  assert(editor->selection_active);
  assert(editor->selection_size);
  assert(editor->near_size >= editor->selection_size);

  tool_CopyBytes(editor);
  tool_DeleteBytes(editor);
  return;
}


void tool_PasteBytes(s_editor* const editor)
{
  assert(editor->access_type == 'w' || editor->access_type == 'i');
  assert(g_cutcopy_buffer_size);
  assert(editor->selection_size == g_cutcopy_buffer_size);

  asmutil_CopyData(
    g_cutcopy_buffer,
    editor->base_address + editor->near_size - editor->selection_size,
    g_cutcopy_buffer_size,
    1
  );

  return;
}


void tool_FindPhrase(
  s_editor* const editor,
  const uint8_t phrase[],
  const uint8_t length,
  uint24_t matches[],
  uint8_t* const num_matches
)
{
CCDBG_BEGINBLOCK("tool_FindPhrase");
CCDBG_DUMP_UINT(editor->near_size);

  assert(!editor->selection_active);
  assert(editor->data_size > G_NUM_BYTES_ONSCREEN);
  assert(length < editor->data_size);
  assert(*num_matches == 0);
  assert(length > 1);

  const uint8_t MAX_NUM_MATCHES = 255;

  uint8_t num_matches_near_buffer = 0;
  uint8_t num_matches_far_buffer = 0;
  uint8_t* far_start_address;

  // If the very last byte of the near buffer is the start of the phrase, we
  // need add <length> - 1 bytes from the start of the far buffer to the end of
  // the near buffer to catch that phrase occurrance.
  tool_MoveCursor(editor, 1, length - 1);

  if (
    !memcmp(editor->base_address + editor->near_size - length, phrase, length)
  )
  {
    // Add the base address because the first convert for-loop expects it.
    matches[0] = (uint24_t)editor->base_address + editor->near_size - length;
    num_matches_far_buffer = 1;
  }

  // Move the cursor back to its original position.
  tool_MoveCursor(editor, 0, length - 1);

  CCDBG_DUMP_UINT(editor->near_size);

  far_start_address = (
    editor->base_address + editor->buffer_size - editor->far_size
  );

  num_matches_far_buffer += asmutil_FindPhrase(
    far_start_address,
    editor->base_address + editor->buffer_size - 1,
    phrase,
    length,
    matches + num_matches_far_buffer,
    MAX_NUM_MATCHES
  );

  // Convert the physical memory addresses into offsets relative to the edit
  // buffer's base address.
  for (uint8_t idx = 0; idx < num_matches_far_buffer; idx++)
  {
    matches[idx] -= (uint24_t)far_start_address + editor->near_size - 2;
  }

  num_matches_near_buffer = asmutil_FindPhrase(
    editor->base_address,
    editor->base_address + editor->near_size - 1,
    phrase,
    length,
    matches + num_matches_far_buffer,
    MAX_NUM_MATCHES - num_matches_far_buffer
  );

  // Convert the physical memory addresses into offsets relative to the edit
  // buffer's base address.
  for (
    uint8_t idx = num_matches_far_buffer;
    idx < num_matches_near_buffer + num_matches_far_buffer;
    idx++
  )
  {
    matches[idx] -= (uint24_t)editor->base_address;
  }

  *num_matches = num_matches_near_buffer + num_matches_far_buffer;

#if USE_CCDBG
CCDBG_DUMP_UINT(*num_matches);
CCDBG_DUMP_UINT(num_matches_far_buffer);
CCDBG_DUMP_UINT(num_matches_near_buffer);
CCDBG_DUMP_PTR(editor->base_address);
CCDBG_DUMP_UINT(editor->near_size);
CCDBG_DUMP_PTR(editor->base_address + editor->near_size - 1);

for (uint8_t idx = 0; idx < *num_matches; idx++)
{
  CCDBG_DUMP_UINT(matches[idx]);
}
#endif

CCDBG_ENDBLOCK();

  return;
}


void tool_SwitchWritingMode(s_editor* const editor)
{
  assert(!editor->selection_active);

  const uint8_t NUM_WRITING_MODES = 4;
  const char WRITING_MODES[] = { 'A', 'a', '0', 'x' };

  for (uint8_t idx = 0; idx < NUM_WRITING_MODES; idx++)
  {
    if (editor->writing_mode == WRITING_MODES[idx])
    {
      editor->writing_mode = WRITING_MODES[(idx + 1) % NUM_WRITING_MODES];
      return;
    }
  }

  // If the writing mode cannot be recognized, fail.
  assert(false);
  return;
}


void tool_UndoLastAction(s_editor* const editor)
{
CCDBG_BEGINBLOCK("tool_UndoLastAction");
CCDBG_DUMP_PTR(g_undo_sp);

  assert(!editor->selection_active);
  assert(editor->undo_buffer_active);
  assert(editor->num_changes);

  uint8_t num_bytes;
  uint8_t code = *(++g_undo_sp);

  g_undo_sp++;
CCDBG_DUMP_UINT(code);

  switch (code)
  {
    case UNDO_WRITE_NIBBLE:
      tool_Goto(editor, *(uint24_t*)g_undo_sp);

CCDBG_DUMP_UINT(*(uint24_t*)g_undo_sp);

      g_undo_sp += sizeof(uint24_t);

      if (*g_undo_sp++)
        editor->high_nibble = true;
      else
        editor->high_nibble = false;

      tool_WriteNibble(editor, *g_undo_sp);
      break;

    case UNDO_WRITE_BYTE:
      tool_Goto(editor, *(uint24_t*)g_undo_sp);
      g_undo_sp += sizeof(uint24_t);
      tool_WriteByte(editor, *g_undo_sp);
      break;

    case UNDO_INSERT_BYTES:
      num_bytes = *g_undo_sp++;
      tool_Goto(editor, (*(uint24_t*)g_undo_sp) + num_bytes - 1);
      g_undo_sp += sizeof(uint24_t) - 1;
      editor->selection_active = true;
      editor->selection_size = num_bytes;
      tool_DeleteBytes(editor);
      break;

    case UNDO_DELETE_OR_CUT_BYTES:
    case UNDO_PASTE_BYTES:
      g_cutcopy_buffer_size = *g_undo_sp++;

      if (code != UNDO_PASTE_BYTES)
      {
        tool_Goto(editor, *(uint24_t*)g_undo_sp);
        tool_InsertBytes(editor, g_cutcopy_buffer_size);
      }
      tool_Goto(editor, *(uint24_t*)g_undo_sp + g_cutcopy_buffer_size - 1);
      g_undo_sp += sizeof(uint24_t);
      asmutil_CopyData(g_undo_sp, g_cutcopy_buffer, g_cutcopy_buffer_size, 1);

      if (g_cutcopy_buffer_size > 1)
      {
        editor->selection_active = true;
        editor->selection_size = g_cutcopy_buffer_size;
      }

      tool_PasteBytes(editor);
      g_undo_sp += g_cutcopy_buffer_size - 1;
      g_cutcopy_buffer_size = 0;  // Destroy the cut/copy buffer.
      break;

    default:
      assert(false);
      break;
  }

  editor->num_changes--;

CCDBG_DUMP_PTR(g_undo_sp);
CCDBG_ENDBLOCK();

  return;
}


void tool_AddUndo_WriteNibble(s_editor* const editor)
{
  typedef struct
  {
    uint8_t code;
    uint24_t offset;
    bool high_nibble;
    uint8_t nibble;
  } s_data;

  // <editor->near_size> is guaranteed to be at least one for this operation.
  uint24_t offset = editor->near_size - 1;
  uint8_t* address = editor->base_address + offset;

  s_data data = {
    .code = UNDO_WRITE_NIBBLE,
    .offset = offset,
    .high_nibble = editor->high_nibble,
    .nibble = (
      editor->high_nibble ? ((*address >> 4) & 0x0f) : (*address & 0x0f)
    )
  };

  if (!undo_buffer_has_room(sizeof data))
    return;

  g_undo_sp -= ((sizeof data) - 1);
  *(s_data*)g_undo_sp = data;
  g_undo_sp--;
  editor->num_changes++;
  return;
}


void tool_AddUndo_WriteByte(s_editor* const editor)
{
  typedef struct
  {
    uint8_t code;
    uint24_t offset;
    uint8_t byte;
  } s_data;

  // <editor->near_size> is guaranteed to be at least one for this operation.
  uint24_t offset = editor->near_size - 1;
  uint8_t* address = editor->base_address + offset;

  s_data data = {
    .code = UNDO_WRITE_BYTE,
    .offset = offset,
    .byte = *address
  };

  if (!undo_buffer_has_room(sizeof data))
    return;

  g_undo_sp -= ((sizeof data) - 1);
  *(s_data*)g_undo_sp = data;
  g_undo_sp--;
  editor->num_changes++;
  return;
}


void tool_AddUndo_InsertBytes(s_editor* const editor, const uint8_t num_bytes)
{
  typedef struct
  {
    uint8_t code;
    uint8_t num_bytes;
    uint24_t offset;
  } s_data;

  uint24_t offset = editor->near_size - (editor->data_size ? 1 : 0);

  s_data data = {
    .code = UNDO_INSERT_BYTES,
    .num_bytes = num_bytes,
    .offset = offset
  };

  if (!undo_buffer_has_room(sizeof data))
    return;

  g_undo_sp -= ((sizeof data) - 1);
  *(s_data*)g_undo_sp = data;
  g_undo_sp--;
  editor->num_changes++;
  return;
}


void tool_AddUndo_DeleteOrCutBytes(s_editor* const editor)
{
  addundo_delete_or_cut_or_paste_bytes(editor, UNDO_DELETE_OR_CUT_BYTES);
  return;
}


void tool_AddUndo_PasteBytes(s_editor* const editor)
{
  addundo_delete_or_cut_or_paste_bytes(editor, UNDO_PASTE_BYTES);
  return;
}


// =============================================================================
// STATIC FUNCTION DEFINITIONS
// =============================================================================


static bool shrink_edit_buffer(const s_editor* const editor)
{
CCDBG_BEGINBLOCK("shrink_edit_buffer");

  // <editor->near_size> is guaranteed to be at least one IF the edit buffer
  // has data in it.
  uint8_t* src = (
    editor->base_address + editor->near_size - (editor->data_size ? 1 : 0)
  );
  uint8_t* dest;
  bool retval = false;
  uint8_t handle;
  int resized;

  if (editor->far_size)
  {
    dest = editor->base_address + editor->buffer_size - editor->far_size - 1;
    asmutil_CopyData(src, dest, editor->near_size, 0);
  }

  if ((handle = ti_Open(G_EDIT_BUFFER_APPVAR_NAME, "r")))
  {
    resized = ti_Resize(editor->data_size, handle);

    // If the edit buffer appvar is of the maximum size, ti_Resize() will
    // return 0 because the variable cannot be resized. This is a valid
    // state, so it will not be treated as an error.
    if (resized <= 0 && editor->data_size != MAX_VAR_DATA_SIZE)
      retval = false;
    else
      retval = true;

CCDBG_DUMP_UINT(ti_GetSize(handle));

    ti_Close(handle);
  }

CCDBG_DUMP_UINT(retval);
CCDBG_ENDBLOCK();

  return retval;
}


static bool archive_edit_buffer(void)
{
CCDBG_BEGINBLOCK("archive_edit_buffer");

  bool retval = false;
  uint8_t handle;

  if ((handle = ti_Open(G_EDIT_BUFFER_APPVAR_NAME, "r")))
  {
    ti_SetArchiveStatus(true, handle);
    ti_Close(handle);
    retval = true;
  }

CCDBG_DUMP_UINT(retval);
CCDBG_ENDBLOCK();

  return retval;
}


static bool save_edited_var(s_calc_var* const var)
{
CCDBG_BEGINBLOCK("save_edited_var");

  bool retval = false;
  uint8_t handle_buffer;
  uint8_t handle_var;

  if ((handle_buffer = ti_Open(G_EDIT_BUFFER_APPVAR_NAME, "r")))
  {
    if ((handle_var = ti_OpenVar(var->name, "w", var->type)))
    {
      ti_Resize(ti_GetSize(handle_buffer), handle_var);

      if (ti_GetSize(handle_var) == ti_GetSize(handle_buffer))
      {
        asmutil_CopyData(
          ti_GetDataPtr(handle_buffer),
          ti_GetDataPtr(handle_var),
          ti_GetSize(handle_buffer),
          1
        );

        if (ti_SetArchiveStatus(var->archived, handle_var))
          retval = true;
        else
        {
CCDBG_PUTS("Could not archive variable");
        }
      }
      else
      {
CCDBG_PUTS("Variable not resized");
      }

      ti_Close(handle_var);
    }
    else
    {
CCDBG_PUTS("Variable not opened");
    }

    ti_Close(handle_buffer);
  }
  else
  {
CCDBG_PUTS("Edit buffer not opened");
  }

CCDBG_ENDBLOCK();

  return retval;
}


static bool undo_buffer_has_room(const uint24_t size)
{
  if (g_undo_sp < g_undo_stack + size)
  {
    assert(false && "TODO: Mention undo buffer is full");
    return false;
  }

  return true;
}


static void addundo_delete_or_cut_or_paste_bytes(
  s_editor* const editor, const uint8_t code
)
{
  typedef struct
  {
    uint8_t code;
    uint8_t num_bytes;
    uint24_t offset;
    uint8_t* data[G_MAX_SELECTION_SIZE];
  } s_data;

  // <editor->near_size> is guaranteed to be at least the same size as
  // <editor->selection_size> for this operation.
  uint24_t offset = editor->near_size - editor->selection_size;

  s_data data = {
    .code = code,
    .num_bytes = editor->selection_size,
    .offset = offset
  };

  if (!undo_buffer_has_room(sizeof data))
    return;

  g_undo_sp -= data.num_bytes - 1;
  asmutil_CopyData(editor->base_address + offset, g_undo_sp, data.num_bytes, 1);
  g_undo_sp -= sizeof data.offset;
  *(uint24_t*)g_undo_sp = data.offset;
  g_undo_sp -= sizeof data.num_bytes;
  *g_undo_sp = data.num_bytes;
  g_undo_sp -= sizeof data.code;
  *g_undo_sp = data.code;
  g_undo_sp--;
  editor->num_changes++;
  return;
}
