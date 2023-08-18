// Name:    Captain Calc
// File:    main_hl.h
// Purpose: Declares the API for HexaEdit's Headless Start, a way to spawn a GUI
//          editor without going through the main menu.


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


#ifndef MAIN_HL_H
#define MAIN_HL_H


#include "defines.h"

// HEADER
// +----------------+-----------------+
// | Description    | Size (in bytes) |
// +----------------+-----------------+
// | Header         | 8               |
// | Flags          | 1               |
// +----------------+-----------------+
//  Total           | 9               |
//                  +-----------------+
//

// After writing the HEADER, you should either write the MEMORY EDITOR block or
// the VARIABLE EDITOR block, but never both.

// MEMORY EDITOR
//
// +----------------+-----------------+
// | Description    | Size (in bytes) |
// +----------------+-----------------+
// | Flag           | 1               |
// | Cursor offset  | 3               |
// +----------------+-----------------+
//  Total           | 4               |
//                  +-----------------+
//

// VARIABLE EDITOR
//
// +----------------+-----------------+
// | Description    | Size (in bytes) |
// +----------------+-----------------+
// | Name           | 8               |
// | Name length    | 1               |
// | Variable type  | 1               |
// | Cursor offset  | 3               |
// +----------------+-----------------+
//  Total           | 13              |
//                  +-----------------+
//

void mainhl_SetMemEditor(void);

void mainhl_SetVarEditor(void);

bool mainhl_CheckAns(void);

int mainhl_RunEditor(s_editor* const editor);


#endif