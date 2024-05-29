// Name:    Captain Calc
// File:    cutil.c
// Purpose: Contains definitions of the functions declared in cutil.h.


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
#include <graphx.h>
#include <string.h>

#include "cutil.h"
#include "defines.h"


// =============================================================================
// PUBLIC FUNCTION DEFINITIONS
// =============================================================================


void cutil_UintToHex(char* const buffer, uint24_t number)
{
  const char* const CHARS = "0123456789abcdef";

  for (int8_t idx = 5; idx >= 0; idx--)
  {
    buffer[idx] = CHARS[number % 16];
    number /= 16;
  }

  return;
}


uint24_t cutil_HexToUint(const char* const buffer)
{
  const char *hex_chars = { "0123456789abcdef" };
  uint8_t length = strlen(buffer);
  uint24_t place = 1;
  uint24_t integer = 0;
  
  while (length > 0)
  {
    length--;
    
    for (uint8_t idx = 0; idx < 16; idx++)
    {
      if (*(buffer + length) == hex_chars[idx])
      {
        integer += place * idx;
      };
    };
    
    place *= 16;
  };
  
  return integer;
}


uint24_t cutil_Log10(uint24_t value)
{
  uint24_t log = 1;

  while (value) { value /= 10; if (value) log++; }

  return log;
}


bool cutil_IsVarHidden(const char* const name)
{
  if (name[0] <= 0x20)
    return true;

  return false;
}
