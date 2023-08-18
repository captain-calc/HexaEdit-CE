// Name:    Captain Calc
// File:    defines.c
// Purpose: Defines some of the declarations in defines.h


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


#include "defines.h"

#define BLACK   (0x00)
#define BLUE    (0x3d)
#define DK_GRAY (0x6b)
#define LT_GRAY (0xb5)
#define WHITE   (0xff)


const char* G_UPPERCASE_LETTERS_KEYMAP[7] = {
  "\0\0\0\0\0\0\0\0", "\0XSNIDA\0", "\0YTOJEB\0", "\0ZUPKFC\0",
  "\0\0VQLG\0\0", "\0\0WRMH\0\0", "\0\0\0\0"
};

const char* G_LOWERCASE_LETTERS_KEYMAP[7] = {
  "\0\0\0\0\0\0\0\0", "\0xsnida\0", "\0ytojeb\0", "\0zupkfc\0",
  "\0\0vqlg\0\0", "\0\0wrmh\0\0", "\0\0\0\0"
};

const char* G_DIGITS_KEYMAP[7] = {
  "\0\0\0\0\0\0\0\0", "\0\0\0\0\0\0\0", "\x30\x31\x34\x37\0\0\0\0",
  "\0\x32\x35\x38\0\0\0\0", "\0\x33\x36\x39\0\0\0\0", "\0\0\0\0\0\0\0\0",
  "\0\0\0\0"
};

const char* G_HEX_NIBBLES_KEYMAP[7] = {
  "\0\0\0\0\0\0\0\0", "\0\0\0\0\0\x0D\x0A\0", "\x00\x01\x04\x07\0\x0E\x0B\0",
  "\0\x02\x05\x08\0\x0F\x0C\0", "\0\x03\x06\x09\0\0\0\0", "\0\0\0\0\0\0\0\0",
  "\0\0\0\0"
};

const char* G_HEX_ASCII_KEYMAP[7] = {
  "\0\0\0\0\0\0\0\0", "\0\0\0\0\0da\0", "\x30\x31\x34\x37\0eb\0",
  "\0\x32\x35\x38\0fc\0", "\0\x33\x36\x39\0\0\0\0", "\0\0\0\0\0\0\0\0",
  "\0\0\0\0"
};


s_color g_color = {
  .bar = DK_GRAY,
  .bar_text = WHITE,
  .bar_text_dark = LT_GRAY,
  .background = WHITE,
  .editor_side_panel = LT_GRAY,
  .editor_cursor = BLUE,
  .editor_text_normal = BLACK,
  .editor_text_selected = WHITE,
  .list_cursor = BLACK,
  .list_text_normal = BLACK,
  .list_text_selected = WHITE
};