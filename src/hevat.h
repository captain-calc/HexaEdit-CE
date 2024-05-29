// Name:    Captain Calc
// File:    hevat.h
// Purpose: Provides the HexaEdit VAT system (HEVAT) used to track all TI-OS
//          variables present on the calculator as well as those that have
//          been recently edited by HexaEdit. It is an interface to the TI-OS
//          VAT with a few extra features.


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


#ifndef HEVAT_H
#define HEVAT_H


#include <stdint.h>


// Based on the CEmu core/vat.c code by the CE-Programming team.
typedef enum calc_var_type : uint8_t
{
  CALC_VAR_TYPE_REAL = 0,
  CALC_VAR_TYPE_REAL_LIST,
  CALC_VAR_TYPE_MATRIX,
  CALC_VAR_TYPE_EQU,
  CALC_VAR_TYPE_STRING,
  CALC_VAR_TYPE_PROG,
  CALC_VAR_TYPE_PROT_PROG,
  CALC_VAR_TYPE_PICTURE,
  CALC_VAR_TYPE_GDB,
  CALC_VAR_TYPE_UNKNOWN,
  CALC_VAR_TYPE_UNKNOWN_EQU,
  CALC_VAR_TYPE_NEW_EQU,
  CALC_VAR_TYPE_CPLX,
  CALC_VAR_TYPE_CPLX_LIST,
  CALC_VAR_TYPE_UNDEF,
  CALC_VAR_TYPE_WINDOW,
  CALC_VAR_TYPE_RCL_WINDOW,
  CALC_VAR_TYPE_TABLE_RANGE,
  CALC_VAR_TYPE_LCD,
  CALC_VAR_TYPE_BACKUP,
  CALC_VAR_TYPE_APP,
  CALC_VAR_TYPE_APP_VAR,
  CALC_VAR_TYPE_TEMP_PROG,
  CALC_VAR_TYPE_GROUP,
  CALC_VAR_TYPE_REAL_FRAC,
  CALC_VAR_TYPE_UNKNOWN1,
  CALC_VAR_TYPE_IMAGE,
  CALC_VAR_TYPE_CPLX_FRAC,
  CALC_VAR_TYPE_REAL_RADICAL,
  CALC_VAR_TYPE_CPLX_RADICAL,
  CALC_VAR_TYPE_CPLX_PI,
  CALC_VAR_TYPE_CPLX_PI_FRAC,
  CALC_VAR_TYPE_REAL_PI,
  CALC_VAR_TYPE_REAL_PI_FRAC,
  CALC_VAR_TYPE_UNKNOWN2,
  CALC_VAR_TYPE_OPERATING_SYSTEM,
  CALC_VAR_TYPE_FLASH_APP,
  CALC_VAR_TYPE_CERTIFICATE,
  CALC_VAR_TYPE_UNKNOWN3,
  CALC_VAR_TYPE_CERTIFICATE_MEMORY,
  CALC_VAR_TYPE_UNKNOWN4,
  CALC_VAR_TYPE_CLOCK,
  CALC_VAR_TYPE_UNKNOWN5,
  CALC_VAR_TYPE_UNKNOWN6,
  CALC_VAR_TYPE_UNKNOWN7,
  CALC_VAR_TYPE_UNKNOWN8,
  CALC_VAR_TYPE_UNKNOWN9,
  CALC_VAR_TYPE_UNKNOWN10,
  CALC_VAR_TYPE_UNKNOWN11,
  CALC_VAR_TYPE_UNKNOWN12,
  CALC_VAR_TYPE_UNKNOWN13,
  CALC_VAR_TYPE_UNKNOWN14,
  CALC_VAR_TYPE_UNKNOWN15,
  CALC_VAR_TYPE_UNKNOWN16,
  CALC_VAR_TYPE_UNKNOWN17,
  CALC_VAR_TYPE_UNKNOWN18,
  CALC_VAR_TYPE_UNKNOWN19,
  CALC_VAR_TYPE_UNKNOWN20,
  CALC_VAR_TYPE_UNKNOWN21,
  CALC_VAR_TYPE_UNKNOWN22,
  CALC_VAR_TYPE_UNKNOWN23,
  CALC_VAR_TYPE_UNKNOWN24,
  CALC_VAR_TYPE_FLASH_LICENSE,
  CALC_VAR_TYPE_UNKNOWN25
} calc_var_type_t;


// Based on the CEmu core/vat.c code by the CE-Programming team.
typedef struct
{
  uint8_t* vatptr;
  uint8_t* data;
  uint8_t type_one, type_two;
  uint8_t version;
  uint8_t name_length;
  char name[9];
  calc_var_type_t type;
  uint16_t size;
  bool archived;
  bool named;
} s_calc_var;


// This gives the position of each group of entries in the HEVAT.
enum HEVAT_GROUP_INDEX : uint8_t
{
  HEVAT__RECENTS = 0,
  HEVAT__APPVAR,
  HEVAT__PROT_PROGRAM,
  HEVAT__PROGRAM,
  HEVAT__REAL,
  HEVAT__LIST,
  HEVAT__MATRIX,
  HEVAT__EQUATION,
  HEVAT__STRING,
  HEVAT__PICTURE,
  HEVAT__GDB,
  HEVAT__COMPLEX,
  HEVAT__COMPLEX_LIST,
  HEVAT__GROUP,
  HEVAT__OTHER,
  HEVAT__NUM_GROUPS
};


extern const char* HEVAT__GROUP_NAMES[HEVAT__NUM_GROUPS];


bool hevat_Load(void);


bool hevat_SaveRecents(void);


// Description: Returns the pointer in the HEVAT for the given group index and
//              offset.
// Pre:         HEVAT should be loaded.
// Post:        Returns pointer in the HEVAT at the offset within the group.
void* hevat_Ptr(const uint8_t hevat_group_idx, const uint24_t offset);


void hevat_Name(
  char* name,
  uint24_t* name_length,
  const uint8_t hevat_group_idx,
  const uint24_t offset
);


uint24_t hevat_NumEntries(const uint8_t hevat_group_idx);


void hevat_AddRecent(void* vatptr);


// Pre: <var->vatptr> should be set and valid.
bool hevat_GetVarInfoByVAT(s_calc_var* const var);


void hevat_VarNameToASCII(char buffer[20], const uint8_t name[8], bool named);


bool hevat_GetVarInfoByNameAndType(
  s_calc_var* const var,
  const char name[8],
  const uint8_t name_length,
  const uint8_t var_type
);

void hevat_GetHEVATGroupNames(char buffer[20], uint24_t index);

void hevat_GetRecentsVariableName(char buffer[20], uint24_t index);

void hevat_GetAppvarVariableName(char buffer[20], uint24_t index);

void hevat_GetProtProgramVariableName(char buffer[20], uint24_t index);

void hevat_GetProgramVariableName(char buffer[20], uint24_t index);

void hevat_GetRealVariableName(char buffer[20], uint24_t index);

void hevat_GetListVariableName(char buffer[20], uint24_t index);

void hevat_GetMatrixVariableName(char buffer[20], uint24_t index);

void hevat_GetEquationVariableName(char buffer[20], uint24_t index);

void hevat_GetStringVariableName(char buffer[20], uint24_t index);

void hevat_GetPictureVariableName(char buffer[20], uint24_t index);

void hevat_GetGDBVariableName(char buffer[20], uint24_t index);

void hevat_GetComplexVariableName(char buffer[20], uint24_t index);

void hevat_GetComplexListVariableName(char buffer[20], uint24_t index);

void hevat_GetGroupVariableName(char buffer[20], uint24_t index);

void hevat_GetOtherVariableName(char buffer[20], uint24_t index);


#endif
