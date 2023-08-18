// Name:    Captain Calc
// File:    keypad.c
// Purpose: Defines the functions declared in keypad.h.


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


#include <assert.h>

#include "defines.h"
#include "keypad.h"


// https://www.eevblog.com/forum/beginners/from-bit-position-to-array-index/
#define only_one_bit_set(byte) \
( (byte & (byte - 1) || !byte) ? 0 : 1 )


// =============================================================================
// PUBLIC FUNCTION DEFINITIONS
// =============================================================================


// https://www.eevblog.com/forum/beginners/from-bit-position-to-array-index/
static uint8_t bit_to_idx(uint8_t byte)
{
  uint8_t result = 0;
  if (byte & 0b11110000) result |= 4;
  if (byte & 0b11001100) result |= 2;
  if (byte & 0b10101010) result |= 1;
  return result;
}


// Post: If a pressed key is found with a corresponding keymap value, <value>
//       set and 0 returned.
//       If more than one key is pressed, 1 returned.
//       If no keys with corresponding keymap values were found, 2 returned.
uint8_t keypad_ExclusiveKeymap(
  const char* const keymap[8], uint8_t* const value
)
{
  bool key_pressed = false;
  bool value_found = false;

  for (uint8_t idx = 1; idx < 8; idx++)
  {
    if (kb_Data[idx])
    {
      if (key_pressed)
        return 1;

      key_pressed = true;

      if (only_one_bit_set(kb_Data[idx]))
      {
        *value = (keymap[idx - 1][bit_to_idx(kb_Data[idx])]);
        value_found = true;
      }
    }
  }

  if (value_found)
    return 0;

  return 2;
}


// Description: Retrieves an uppercase, lowercase, or numerical symbol based on
//              the given mode and the state of the keypad.
// Pre:         <keymap> should be one of ['A', 'a', '0'].
//              'A' = uppercase letters
//              'a' = lowercase letters
//              '0' = digits
// Post:        If valid <mode> and keypad state, ASCII <value> set and true
//              returned.
//              Otherwise, false returned.
bool keypad_ExclusiveASCII(uint8_t* const value, const char mode)
{
  const char** keymap = G_UPPERCASE_LETTERS_KEYMAP;

  if (mode == 'a')
    keymap = G_LOWERCASE_LETTERS_KEYMAP;
  else if (mode == '0')
    keymap = G_DIGITS_KEYMAP;

  if (!keypad_ExclusiveKeymap(keymap, value) && *value != 0x00)
    return true;

  return false;
}


bool keypad_ExclusiveNibble(uint8_t* const value)
{
  if (!keypad_ExclusiveKeymap(G_HEX_NIBBLES_KEYMAP, value))
  {
    if (*value == 0x00 && !kb_IsDown(kb_Key0))
      return false;

    return true;
  }

  return false;
}


bool keypad_SinglePressExclusive(kb_lkey_t key)
{
  bool key_pressed = false;

  do {
    kb_Scan();

    if (kb_IsDown(key))
      key_pressed = true;

    for (uint8_t idx = 1; idx < 8; idx++)
    {
      if (idx != (key >> 8) && kb_Data[idx])
        return false;
    }

  } while (kb_IsDown(key));

  return key_pressed;
}


void keypad_IdleKeypadBlock(void)
{
  do { kb_Scan(); } while (!kb_AnyKey());
  return;
}