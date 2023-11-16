// Name:    Captain Calc
// File:    hevat.c
// Purpose: Defines the functions declared in hevat.h and any supporting static
//          functions.


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


#include <ti/vars.h>
#include <assert.h>
#include <ctype.h>
#include <fileioc.h>
#include <stdbool.h>
#include <string.h>

#include "ccdbg/ccdbg.h"
#include "asmutil.h"
#include "defines.h"
#include "hevat.h"


#define MAX_NUM_RECENTS          (15)
#define MAX_NUM_HEVAT_ENTRIES    (1015)


const char* HEVAT__GROUP_NAMES[HEVAT__NUM_GROUPS] = {
  "Recents...", "Appvar", "Prot Prgm", "Program", "Real", "Real List",
  "Matrix", "Equation", "String", "Picture", "GDB", "Complex", "Cplx List",
  "Group", "Other"
};


// Do NOT use these variables outside this file.
static void* g_hevat[MAX_NUM_HEVAT_ENTRIES] = { NULL };
static uint24_t g_num_vatptrs = 0;

// Holds the number of entries for a given group index.
static uint24_t g_num_entries[HEVAT__NUM_GROUPS] = { 0 };


// =============================================================================
// STATIC FUNCTION DECLARATIONS
// =============================================================================


// Description: Converts a TI-OS variable type into an HEVAT_GROUP_INDEX.
// Pre:         <os_var_type> must be valid.
// Post:        The HEVAT_GROUP_INDEX will be returned.
static uint8_t hevat_group_index(const uint24_t os_var_type);


static bool load_recents_entries(void);


// Description: Counts the number of VAT pointers present on the calculator
//              and tallies the number of them that point to each TI-OS
//              variable type.
// Pre:         None
// Post:        <g_num_vatptrs> and <g_num_entries> set.
static void count_vatptrs(void);


// Description: Determines where the given group starts in the HEVAT.
// Pre:         count_vatptrs() should have been called.
// Post:        Returns the offset in the HEVAT where the given group starts.
static uint24_t hevat_offset(const uint8_t hevat_group_idx);


// Description: Loads all of the TI-OS VAT pointers into the HEVAT.
// Pre:         count_vatptrs() should have been called.
// Post:        HEVAT contains VAT pointers sorted by variable type.
static void load_vatptr_entries(void);


// Description: Performs an alphabetical string comparision between the names
//              of the variables at each of the given TI-OS VAT pointers.
// Pre:         Both VAT pointers should be valid.
// Post:        If the name of the variable at <vatptr_one> (N1) occurs before
//              the namve of the variable at <vatptr_two> (N2) in alphabetical
//              order, -1 returned.
//              If N1 is equal to N2, 0 returned.
//              If N1 occurs after N2 in alphabetical order, 1 returned.
static int8_t entry_name_cmp(void* vatptr_one, void* vatptr_two);


// Description: Sorts the VAT pointers in the HEVAT alphabetically. Uses
//              insertion sort.
static void sort_hevat(void);


static char hex_char(uint8_t nibble)
{
  nibble &= 0x0F;
  return (nibble < 10 ? '0' + nibble : 'A' + nibble - 10);
}


static void hex_byte(char** dest, uint8_t byte)
{
  *(*dest)++ = hex_char(byte >> 4);
  *(*dest)++ = hex_char(byte >> 0);
  return;
}


// =============================================================================
// PUBLIC FUNCTION DEFINITIONS
// =============================================================================


bool hevat_Load(void)
{
CCDBG_BEGINBLOCK("hevat_Load");
CCDBG_DUMP_PTR(g_hevat);

  memset(g_hevat, '\0', sizeof g_hevat);
  g_num_vatptrs = 0;

  for (uint8_t idx = 0; idx < HEVAT__NUM_GROUPS; idx++)
    g_num_entries[idx] = 0;

  if (!load_recents_entries())
  {
CCDBG_PUTS("Could not load recent entries");
CCDBG_ENDBLOCK();

    return false;
  }

  count_vatptrs();
  load_vatptr_entries();
  sort_hevat();

CCDBG_ENDBLOCK();

  return true;
}


bool hevat_SaveRecents(void)
{
CCDBG_BEGINBLOCK("tool_SaveRecents");

  s_calc_var var;
  uint8_t handle;

  if (!(handle = ti_Open(G_RECENTS_APPVAR_NAME, "r+")))
  {
CCDBG_PUTS("Cannot open recents appvar");
CCDBG_ENDBLOCK();

    return false;
  }

CCDBG_PUTS("Opened recents appvar");

  memset(ti_GetDataPtr(handle), '\0', ti_GetSize(handle));

  for (uint8_t idx = 0; idx < g_num_entries[HEVAT__RECENTS]; idx++)
  {
    var.vatptr = g_hevat[idx];
    hevat_GetVarInfoByVAT(&var);

assert(
  ti_Tell(handle) + sizeof var.type + sizeof var.name_length + var.name_length
  < ti_GetSize(handle)
);
CCDBG_PUTS(var.name);
CCDBG_DUMP_UINT(var.type);
CCDBG_DUMP_UINT(var.name_length);

    if (ti_Write(&var.type, sizeof var.type, 1, handle) != 1)
    {
CCDBG_PUTS("Cannot write type");
CCDBG_ENDBLOCK();

      return false;
    }

    if (ti_Write(&var.name_length, sizeof var.name_length, 1, handle) != 1)
    {
CCDBG_PUTS("Cannot write name length");
CCDBG_ENDBLOCK();

      return false;
    }

    if (ti_Write(var.name, var.name_length, 1, handle) != 1)
    {
CCDBG_PUTS("Cannot write name");
CCDBG_ENDBLOCK();

      return false;
    }
  }

  ti_Close(handle);

CCDBG_ENDBLOCK();

  return true;
}


void* hevat_Ptr(const uint8_t hevat_group_idx, const uint24_t offset)
{
  assert(g_num_entries[hevat_group_idx] > offset);

  return g_hevat[hevat_offset(hevat_group_idx) + offset];
}


void hevat_Name(
  char* name,
  uint24_t* name_length,
  const uint8_t hevat_group_idx,
  const uint24_t offset
)
{
  void** data = NULL;
  uint24_t type = 0;

  os_NextSymEntry(
    hevat_Ptr(hevat_group_idx, offset), &type, name_length, name, data
  );

  return;
}


uint24_t hevat_NumEntries(const uint8_t hevat_group_idx)
{
  return g_num_entries[hevat_group_idx];
}


void hevat_AddRecent(void* vatptr)
{
  uint8_t idx = 0;

  while (g_hevat[idx] && g_hevat[idx] != vatptr && idx < MAX_NUM_RECENTS)
    idx++;

  if (idx == MAX_NUM_RECENTS)
    idx--;
  else if (g_hevat[idx] != vatptr)
    g_num_entries[HEVAT__RECENTS]++;

  while (idx)
  {
    g_hevat[idx] = g_hevat[idx - 1];
    idx--;
  }

  g_hevat[0] = vatptr;
  return;
}


// Based on the CEmu core/vat.c code by the CE-Programming team.
bool hevat_GetVarInfoByVAT(s_calc_var* const var)
{
  const uint8_t* userMem  = (const uint8_t*)0xD1A881;
  const uint8_t* OPBase   = *(const uint8_t**)0xD02590;
  const uint8_t* pTemp    = *(const uint8_t**)0xD0259A;
  const uint8_t* progPtr  = *(const uint8_t**)0xD0259D;
  const uint8_t* symTable = (const uint8_t*)0xD3FFFF;

  uint8_t* orig_vatptr = var->vatptr;
  uint24_t data = 0;
  uint16_t size = 0;
  uint8_t idx;

  assert(var->vatptr);

  if (var->vatptr < userMem || var->vatptr <= OPBase || var->vatptr > symTable)
    return false;

  memset(var, 0x00, sizeof *var);
  var->vatptr = orig_vatptr;

  var->type_one = *var->vatptr--;
  var->type_two = *var->vatptr--;
  var->version  = *var->vatptr--;
  data          = *var->vatptr--;
  data          |= *var->vatptr << 8; var->vatptr--;
  data          |= *var->vatptr << 16; var->vatptr--;
  var->data     = (uint8_t*)data;

  if ((var->named = (var->vatptr > pTemp) && (var->vatptr <= progPtr)))
  {
    var->name_length = *var->vatptr;
    var->vatptr--;

    if (!var->name_length || var->name_length > 8)
      return false;
  }
  else
    var->name_length = 3;

  var->archived = (
    ((uint24_t)var->data > 0x0C0000) && ((uint24_t)var->data < 0x400000)
  );

  if (var->archived)
  {
    // Skip past the duplicate header.
    var->data += 9 + var->named + var->name_length;
  }
  else if (var->data < userMem || var->data > symTable)
    return false;

  var->type = (calc_var_type_t)(var->type_one & 0x3F);
  size = *((uint16_t*)var->data);

  switch(var->type)
  {
    case CALC_VAR_TYPE_REAL:
    case CALC_VAR_TYPE_REAL_FRAC:
    case CALC_VAR_TYPE_REAL_RADICAL:
    case CALC_VAR_TYPE_REAL_PI:
    case CALC_VAR_TYPE_REAL_PI_FRAC:
      var->size = 9;
      break;

    case CALC_VAR_TYPE_REAL_LIST:
      var->size = 2 + size * 9;
      break;

    case CALC_VAR_TYPE_MATRIX:
      var->size = 2 + size * size * 9;
      break;

    case CALC_VAR_TYPE_CPLX:
    case CALC_VAR_TYPE_CPLX_FRAC:
    case CALC_VAR_TYPE_CPLX_RADICAL:
    case CALC_VAR_TYPE_CPLX_PI:
    case CALC_VAR_TYPE_CPLX_PI_FRAC:
      var->size = 18;
      break;

    case CALC_VAR_TYPE_CPLX_LIST:
      var->size = 2 + size * 18;
      break;

    default:
      var->size = 2 + size;
      break;
  }

  for (idx = 0; idx != var->name_length; idx++)
    var->name[idx] = *var->vatptr--;

  memset(&var->name[idx], 0x00, sizeof var->name - idx);
  var->vatptr = orig_vatptr;
  return true;
}


// Based on the CEmu core/vat.c code by the CE-Programming team.
void hevat_VarNameToASCII(char buffer[20], const uint8_t name[8], bool named)
{
  char* dest = buffer;
  uint8_t idx = 0;

  // If the variable is a user-defined list, put the 'L' in and increment the
  // index so the for-loop can parse the list's ASCII name.
  if (name[0] == 0x5D)
  {
    *dest++ = 'L';
    idx++;
  }

  for (
    ;
    idx < 8 && (
      (name[idx] >= 'A' && name[idx] <= 'Z' + 1)
      || (idx && name[idx] >= 'a' && name[idx] <= 'z')
      || (idx && name[idx] >= '0' && name[idx] <= '9')
    );
    idx++
  )
  {
    // 'Z' + 1 is the TI-OS token for theta.
    if (name[idx] == 'Z' + 1)
      *dest++ = G_HEXAEDIT_THETA;
    else
      *dest++ = name[idx];
  }

  // In English: If the variable is a TI-OS list or is a non-ASCII name, do....
  if (!(idx - (name[0] == 0x5D)))
  {
    switch(name[0])
    {
      case '!':
      case '#':
      case '.':
      case '@':
        *dest++ = name[0];
        break;

      case '$':
        *dest++ = name[0];
        hex_byte(&dest, name[2]);
        hex_byte(&dest, name[1]);
        break;

      case 0x3C:
        *dest++ = 'I';
        *dest++ = 'm';
        *dest++ = 'a';
        *dest++ = 'g';
        *dest++ = 'e';
        *dest++ = '0' + (name[1] + 1) % 10;
        break;

      case 0x5C:
        *dest++ = '[';
        *dest++ = 'A' + name[1];
        *dest++ = ']';
        break;

      // We already have the 'L' in place.
      case 0x5D:
        *dest++ = '1' + name[1];
        break;

      case 0x5E:
        if (name[1] < 0x20)
        {
          *dest++ = 'Y';
          *dest++ = '0' + (name[1] % 0x10);
        }
        else if (name[1] < 0x40)
        {
          if (name[1] % 2 == 0)
            *dest++ = 'X';
          else
            *dest++ = 'Y';

          *dest++ = '0' + ((name[1] % 0x20) / 2);
          *dest++ = 'T';
        }
        else if (name[1] < 0x80)
        {
          *dest++ = 'r';
          *dest++ = '0' + (name[1] % 0x40);
        }
        else
        {
          *dest++ = 'u' + (name[1] & 3);
        }
        break;

      case 0x60:
        *dest++ = 'P';
        *dest++ = 'i';
        *dest++ = 'c';
        *dest++ = '0' + (name[1] + 1) % 10;
        break;

      case 0x61:
        *dest++ = 'G';
        *dest++ = 'D';
        *dest++ = 'B';
        *dest++ = '0' + (name[1] + 1) % 10;
        break;

      case 0x62:
        if (name[1] == 0x21)
          *dest++ = 'n';

        break;

      case 0xAA:
        *dest++ = 'S';
        *dest++ = 't';
        *dest++ = 'r';
        *dest++ = '0' + (name[1] + 1) % 10;
        break;

      case 0x72:
        if (!named) {
            *dest++ = 'A';
            *dest++ = 'n';
            *dest++ = 's';
            break;
        }

      default:
        idx = 0;

        // If a variable name is hidden, recover the mangled first byte of its
        // name.
        if (name[0] < 0x20)
        {
          *dest++ = name[0] | 0x40;
          idx++;
        }

        for (
          idx = 0;
          idx < 8 && (
            (name[idx] >= 'A' && name[idx] <= 'Z' + 1)
            || (name[idx] >= 'a' && name[idx] <= 'z')
            || (name[idx] >= '0' && name[idx] <= '9')
          );
          idx++
        )
        {
          if (name[idx] == 'Z' + 1)
            *dest++ = G_HEXAEDIT_THETA;
          else
            *dest++ = name[idx];
        }

        if (!idx)
        {
          for (; idx < 8 && name[idx]; idx++)
            hex_byte(&dest, name[idx]);
        }

        break;
    }
  }

  *dest = '\0';
  return;
}


bool hevat_GetVarInfoByNameAndType(
  s_calc_var* const var,
  const char name[8],
  const uint8_t name_length,
  const uint8_t var_type
)
{
  void* last_entry = os_GetSymTablePtr();
  void* entry;
  uint24_t vat_type = 0;
  uint24_t vat_name_length = 0;
  char vat_name[9];
  void** data = NULL;

  while (last_entry != NULL)
  {
    entry = os_NextSymEntry(
      last_entry, &vat_type, &vat_name_length, vat_name, data
    );
    var->vatptr = last_entry;
    hevat_GetVarInfoByVAT(var);
    
    if (
      var->type == var_type
      && var->name_length == name_length
      && !memcmp(var->name, name, min(var->name_length, name_length))
    )
    {
      return true;
    }

    last_entry = entry;
  };

  return false;
}


// =============================================================================
// STATIC FUNCTION DEFINITIONS
// =============================================================================


static uint8_t hevat_group_index(const uint24_t os_var_type)
{
  const uint24_t OS_TO_HEVAT_LUT[26] = {
    OS_TYPE_REAL,      HEVAT__REAL,
    OS_TYPE_REAL_LIST, HEVAT__LIST,
    OS_TYPE_MATRIX,    HEVAT__MATRIX,
    OS_TYPE_EQU,       HEVAT__EQUATION,
    OS_TYPE_STR,       HEVAT__STRING,
    OS_TYPE_PRGM,      HEVAT__PROGRAM,
    OS_TYPE_PROT_PRGM, HEVAT__PROT_PROGRAM,
    OS_TYPE_PICT,      HEVAT__PICTURE,
    OS_TYPE_GDB,       HEVAT__GDB,
    OS_TYPE_CPLX,      HEVAT__COMPLEX,
    OS_TYPE_CPLX_LIST, HEVAT__COMPLEX_LIST,
    OS_TYPE_APPVAR,    HEVAT__APPVAR,
    OS_TYPE_GROUP,     HEVAT__GROUP
  };

  for (uint8_t idx = 0; idx < 26; idx += 2)
  {
    if (os_var_type == OS_TO_HEVAT_LUT[idx])
      return OS_TO_HEVAT_LUT[idx + 1];
  }

  return HEVAT__OTHER;
}


static bool load_recents_entries(void)
{
CCDBG_BEGINBLOCK("load_recents_entries");

  s_calc_var var;
  char name[8];
  uint8_t type;
  uint8_t name_length;
  uint8_t handle;
  uint8_t idx = 0;

  if (!(handle = ti_Open(G_RECENTS_APPVAR_NAME, "r")))
  {
CCDBG_PUTS("Could not open recents appvar");
CCDBG_ENDBLOCK();

    return false;
  }

  while ((ti_Read(&type, sizeof type, 1, handle)) == 1)
  {
    if ((ti_Read(&name_length, sizeof name_length, 1, handle)) != 1)
    {
CCDBG_PUTS("Could not read name length");
CCDBG_ENDBLOCK();

      return false;
    }

    if (name_length == 0)
      break;

    if ((ti_Read(name, name_length, 1, handle)) != 1)
    {
CCDBG_PUTS("Could not read name");
CCDBG_ENDBLOCK();

      return false;
    }

    if (hevat_GetVarInfoByNameAndType(&var, name, name_length, type))
      g_hevat[idx++] = var.vatptr;
  }

  g_num_entries[HEVAT__RECENTS] = idx;
  ti_Close(handle);

CCDBG_ENDBLOCK();

  return true;
}


static void count_vatptrs()
{
CCDBG_BEGINBLOCK("count_vatptrs()");

  void* entry = os_GetSymTablePtr();
  uint24_t os_var_type = 0;
  uint24_t name_length = 0;
  char name[9];
  void* data = NULL;

  while (entry != NULL)
  {
    entry = os_NextSymEntry(entry, &os_var_type, &name_length, name, &data);
    g_num_entries[hevat_group_index(os_var_type)]++;
    g_num_vatptrs++;
  };

  // Decrement the number of VAT pointers and the number of appvar HEVAT entries
  // by one because we are not counting HexaEdit's edit buffer appvar.
  g_num_vatptrs--;
  g_num_entries[HEVAT__APPVAR]--;

CCDBG_ENDBLOCK();

  return;
}


static uint24_t hevat_offset(const uint8_t hevat_group_idx)
{
  uint24_t offset = 0;

  for (uint8_t idx = 0; idx < hevat_group_idx; idx++)
  {
    if (!idx)
      offset = MAX_NUM_RECENTS;
    else
      offset += g_num_entries[idx];
  }

  return offset;
}


static void load_vatptr_entries()
{
CCDBG_BEGINBLOCK("load_vatptr_entries");

  void* entry = os_GetSymTablePtr();
  void* last_entry;
  uint24_t os_var_type = 0;
  uint24_t name_length = 0;
  char name[9];
  void* data = NULL;

  uint24_t curr_num_entries[MAX_NUM_HEVAT_ENTRIES] = { 0 };
  uint8_t hevat_group_idx;
  uint24_t offset;
  uint24_t num_loaded = 0;

  while (entry != NULL && num_loaded < MAX_NUM_HEVAT_ENTRIES)
  {
    last_entry = entry;
    entry = os_NextSymEntry(entry, &os_var_type, &name_length, name, &data);

    // Do not load the VAT entry for HexaEdit's edit buffer appvar.
    if (
      os_var_type == OS_TYPE_APPVAR
      && !strncmp(G_EDIT_BUFFER_APPVAR_NAME, name, min(8, name_length))
    )
    {
      continue;
    }

    hevat_group_idx = hevat_group_index(os_var_type);
    offset = hevat_offset(hevat_group_idx);

    g_hevat[offset + curr_num_entries[hevat_group_idx]] = last_entry;

    curr_num_entries[hevat_group_idx]++;
    num_loaded++;
  }

CCDBG_ENDBLOCK();
  return;
}


static int8_t entry_name_cmp(void* vatptr_one, void* vatptr_two)
{
  assert(vatptr_one != NULL);
  assert(vatptr_two != NULL);

  uint24_t type_one = 0;
  uint24_t type_two = 0;
  uint24_t name_one_len = 0;
  uint24_t name_two_len = 0;
  char src_name_one[9] = { '\0' };
  char src_name_two[9] = { '\0' };
  char name_one[20] = { '\0' };
  char name_two[20] = { '\0' };
  void* data = NULL;
  bool named = false;

  os_NextSymEntry(vatptr_one, &type_one, &name_one_len, src_name_one, &data);
  os_NextSymEntry(vatptr_two, &type_two, &name_two_len, src_name_two, &data);

  assert(type_one == type_two);

  named = asmutil_IsNamedVar(type_one);

  hevat_VarNameToASCII(name_one, (const uint8_t*)src_name_one, named);
  hevat_VarNameToASCII(name_two, (const uint8_t*)src_name_two, named);

  for (uint8_t idx = 0; idx < name_one_len; idx++)
  {
    if (name_one[idx] >= 'a')
      name_one[idx] = name_one[idx] - 'a' + 'A';
  }

  for (uint8_t idx = 0; idx < name_two_len; idx++)
  {
    if (name_two[idx] >= 'a')
      name_two[idx] = name_two[idx] - 'a' + 'A';
  }

  return strncmp(name_one, name_two, min(name_one_len, name_two_len));
}


static void sort_hevat()
{
CCDBG_BEGINBLOCK("sort_hevat()");

  for (
    uint8_t group_idx = HEVAT__APPVAR;
    group_idx < HEVAT__NUM_GROUPS;
    group_idx++
  )
  {
    int24_t offset = (int24_t)hevat_offset(group_idx);
    uint24_t num_entries = g_num_entries[group_idx];

    for (uint24_t idx_one = 1; idx_one < num_entries; idx_one++)
    {
      void* key = g_hevat[offset + idx_one];

      int24_t idx_two = idx_one - 1;

      while (
        (idx_two >= 0)
        && (entry_name_cmp(g_hevat[offset + idx_two], key) > 0)
      )
      {
        g_hevat[offset + idx_two + 1] = g_hevat[offset + idx_two];
        idx_two--;
      }

      g_hevat[offset + idx_two + 1] = key;
    }
  }

CCDBG_ENDBLOCK();

  return;
}
