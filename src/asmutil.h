// Name:    Captain Calc
// File:    asmutil.h
// Purpose: asmutil provides common assembly routines used by more than one
//          file.


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


#ifndef ASMUTIL_H
#define ASMUTIL_H


#include <stdint.h>  // For uint8_t

// ============================================================================
// PUBLIC FUNCTION DECLARATIONS
// ============================================================================


// Description: asmutil_IsNamedVar() determines if a variable has an ASCII name
//              based on its type.
// Pre:         <var_type> should be a valid variable type.
// Post:        If variable is named, true returned.
//              Otherwise, false returned.
bool asmutil_IsNamedVar(const uint8_t var_type);


// Description: asm_CopyData() copies <amount> bytes from <src> to <dest>.
//              If <copy_direction> == 0, the routine decrements the copy
//              address. If <copy_direction> == 1, the routine increments the
//              copy address.
// Pre:         All of the memory from <src> to <src> +/- <amount> and from
//              <dest> to <dest> +/- <amount> must be write-access memory,
//              depending on <copy_direction>.
// Post:        <amount> bytes copied from <src> to <dest> in given
//              <copy_direction>.
void asmutil_CopyData(
  void *src,
  void *dest,
  uint24_t amount,
  uint8_t copy_direction
);


// Description: Finds all occurances of <phrase> in memory from <start> to
//              <end>. Pointers to the start of the match are written to
//              <matches>. When a match is found and recorded, searching
//              resumes from (pointer to match start) + <length>.
// Pre:         <start> and <end> must be valid memory pointers.
//              <end> must be greater than or equal to <start>.
//              <length> must be the length of phrase.
//              <max_matches> must be the size of the <matches> array.
// Post:        Number of matches found returned.
//              <matches> contains pointers to every occurance of <phrase>
//              found between <start> and <end>, inclusive.
uint8_t asmutil_FindPhrase(
  const uint8_t* start,
  const uint8_t* end,
  const uint8_t phrase[],
  const uint8_t length,
  uint24_t matches[],
  uint8_t max_matches
);


#endif
