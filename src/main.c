// Name:    Captain Calc
// File:    main.c
// Purpose: Initializes global data structures, sets up program files, and
//          determines whether to spawn a Headless Start editor or start the
//          main menu.

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
#include <keypadc.h>

#include "ccdbg/ccdbg.h"
#include "defines.h"
#include "gui.h"
#include "main_gui.h"
#include "main_hl.h"
#include "tools.h"


// =============================================================================
// STATIC FUNCTION DECLARATIONS
// =============================================================================


// Description: Starts the calculator's 8-bit graphics.
// Pre:         Calculator must be in TI-OS graphics mode.
// Post:        8-bit graphics set up.
static void open_gfx(void);


// Description: Ends the 8-bit graphics mode and returns to the TI-OS graphics
//              mode.
// Pre:         Calculator must be in 8-bit graphics mode.
// Post:        Calculator in TI-OS graphics mode.
static void close_gfx(void);


// =============================================================================
// MAIN FUNCTION
// =============================================================================


int main(void)
{
CCDBG_BEGINBLOCK("main");

  int retval = 0;
  bool headless_start = false;

  s_editor editor = {
    .name = { '\0' },
    .name_length = 0,
    .access_type = '\0',
    .is_tios_var = false,
    .tios_var_type = 0,
    .undo_buffer_active = false,
    .num_changes = 0,
    .base_address = NULL,
    .data_size = 0,
    .buffer_size = 0,
    .window_offset = 0,
    .location_col_mode = 'o',
    .writing_mode = 'x',
    .near_size = 0,
    .far_size = 0,
    .selection_size = 1,
    .selection_active = false,
    .high_nibble = true
  };

  // mainhl_SetMemEditor();
  // mainhl_SetVarEditor();
  headless_start = mainhl_CheckAns();

CCDBG_PUTS("Checked Ans.");

  if (!tool_CreateRecentsAppvar())
  {
    open_gfx();
    gui_ErrorWindow("Cannot create Recents$appvar.");
    close_gfx();

CCDBG_ENDBLOCK();

    return 1;
  }

CCDBG_PUTS("Created Recents appvar.");

  if (!tool_CreateEditBuffer(&editor))
  {
    open_gfx();
    gui_ErrorWindow("Cannot create edit$buffer.");
    close_gfx();

CCDBG_ENDBLOCK();

    return 1;
  }

CCDBG_PUTS("Created edit buffer.");

  open_gfx();

  if (headless_start)
    retval = mainhl_RunEditor(&editor);
  else
    retval = maingui_Main(&editor);

  close_gfx();
  tool_DeleteEditBuffer();

CCDBG_ENDBLOCK();

  return retval;
}


// =============================================================================
// STATIC FUNCTION DEFINITIONS
// =============================================================================


static void open_gfx(void)
{
  gfx_Begin();
  gfx_SetDrawBuffer();
  kb_SetMode(MODE_3_CONTINUOUS);
  return;
}


static void close_gfx(void)
{
  gfx_End();
  return;
}