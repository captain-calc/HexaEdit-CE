// Name:    Captain Calc
// File:    defines.h
// Purpose: Declares commonly used defines, data structures, and macros.


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


#ifndef DEFINES_H
#define DEFINES_H


#include <stdbool.h>
#include <stdint.h>


// These are not defined in the toolchain.
#define OS_TYPE_PICT  (0x07)
#define OS_TYPE_GDB   (0x08)
#define OS_TYPE_GROUP (0x17)

// The TI-OS token for theta is 0x5b. HexaEdit uses 0x7f to represent theta, so
// the program can print '[' (0x5b).
#define G_HEXAEDIT_THETA (0x7f)

#define G_EDIT_BUFFER_APPVAR_NAME ("HXAEDITb")
#define G_RECENTS_APPVAR_NAME     ("HXAEDITr")
#define G_RECENTS_APPVAR_SIZE     (255)

#define G_FONT_HEIGHT         (7)
#define G_ROWS_ONSCREEN       (18)
#define G_COLS_ONSCREEN       (8)
#define G_NUM_BYTES_ONSCREEN  (G_ROWS_ONSCREEN * G_COLS_ONSCREEN)
#define G_EDITOR_NAME_MAX_LEN (20)
#define G_MAX_SELECTION_SIZE  (255)

#define G_ROM_BASE_ADDRESS    ((uint8_t*)0x000000)
#define G_ROM_SIZE            (0x400000)
#define G_RAM_BASE_ADDRESS    ((uint8_t*)0xd00000)
#define G_RAM_SIZE            (0x065000)
#define G_PORTS_BASE_ADDRESS  ((uint8_t*)0xe00000)
#define G_PORTS_SIZE          (0x200000)

#define min(x, y) ((x > y ? y : x))

extern const char* G_UPPERCASE_LETTERS_KEYMAP[7];
extern const char* G_LOWERCASE_LETTERS_KEYMAP[7];
extern const char* G_DIGITS_KEYMAP[7];
extern const char* G_HEX_NIBBLES_KEYMAP[7];
extern const char* G_HEX_ASCII_KEYMAP[7];


// <access_type> can have three values:
//   'r': readonly
//   'w': read/write
//   'i': read/write/resize  (for variables only)
// <location_col_mode> can have two values:
//   'a': addresses
//   'o': offsets
// <writing_mode> can have four values:
//   'A': uppercase letters
//   'a': lowercase letters
//   '0': numbers
//   'x': hexadecimal characters
typedef struct
{
  char name[G_EDITOR_NAME_MAX_LEN];
  uint8_t name_length;
  char access_type;
  bool is_tios_var;
  uint8_t tios_var_type;
  bool undo_buffer_active;
  uint24_t num_changes;

  uint8_t* base_address;
  uint24_t data_size;
  uint24_t buffer_size;

  uint24_t window_offset;

  char location_col_mode;
  char writing_mode;

  uint24_t near_size;
  uint24_t far_size;
  uint8_t selection_size;
  bool selection_active;
  bool high_nibble;
} s_editor;


typedef struct
{
  uint8_t bar;
  uint8_t bar_text;
  uint8_t bar_text_dark;
  uint8_t background;
  uint8_t editor_side_panel;
  uint8_t editor_cursor;
  uint8_t editor_text_normal;
  uint8_t editor_text_selected;
  uint8_t list_cursor;
  uint8_t list_text_normal;
  uint8_t list_text_selected;
} s_color;

extern s_color g_color;


#endif
