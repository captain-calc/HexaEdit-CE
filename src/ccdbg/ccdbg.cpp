// Name:    Captain Calc
// File:    ccdbg.cpp
// Purpose: ccdbg provides macros for pretty-printing variable values.
// Version: 0.2.4


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


// NOTE
//
//     Changing this file's extension from ".cpp" to ".c" breaks running
// "make debug" in each of the test directories. According to the compiliation
// record produced by make, "ccdbg.c" will not be compiled, but "ccdbg.cpp"
// will be.
//


#include <stdint.h>
#include <stdio.h>
#include <string.h>  // For strlen()

#include "ccdbg.h"


#if USE_CCDBG

unsigned int g_ccdbg_chkpt_indent = 0;


void print_indent()
{
	for (unsigned int idx = 0; idx < g_ccdbg_chkpt_indent; idx++)
	{
		if (idx && !(idx % BLOCK_INDENT))
			sprintf(dbgout, "|");
		else
			sprintf(dbgout, "");

		sprintf(dbgout, " ");
	}

	sprintf(dbgout, "|");
	return;
}


void CCDBG_BEGINBLOCK(const char* name)
{
	print_indent();
  sprintf(dbgout, "\n");

  g_ccdbg_chkpt_indent += BLOCK_INDENT;
  print_indent();

  sprintf(dbgout, " %s\n", name);
  print_indent();

  for (unsigned int idx = VALUE_INDENT + 15; idx > 0; idx--)
    sprintf(dbgout, "-");

  sprintf(dbgout, "\n");

  return;
}


void CCDBG_ENDBLOCK()
{
  print_indent();

  for (int i = VALUE_INDENT + 15; i > 0; i--)
    sprintf(dbgout, "-");

  sprintf(dbgout, "\n");
  g_ccdbg_chkpt_indent -= BLOCK_INDENT;
  print_indent();
  sprintf(dbgout, "\n");
  return;
}


void CCDBG_PUTS(const char* s)
{
	print_indent();
	sprintf(dbgout, "  %s\n", s);
	return;
}


void CCDBG_DUMP_STR_func(const char* varname, const char* var)
{
	print_indent();
  sprintf(dbgout, "  %s", varname);

  for (int idx = VALUE_INDENT - strlen(varname); idx > 0; idx--)
    sprintf(dbgout, " ");

  sprintf(dbgout,"= %s\n", var);
  return;
}


void CCDBG_DUMP_PTR_func(const char* varname, const unsigned int var)
{
  print_indent();
  sprintf(dbgout, "  %s", varname);

  for (int idx = VALUE_INDENT - strlen(varname); idx > 0; idx--)
    sprintf(dbgout, " ");

  sprintf(dbgout,"= 0x%6x\n", var);
  return;
}


void CCDBG_DUMP_UINT_func(const char* varname, const unsigned int var)
{
  print_indent();
  sprintf(dbgout, "  %s", varname);

  for (int idx = VALUE_INDENT - strlen(varname); idx > 0; idx--)
    sprintf(dbgout, " ");

  sprintf(dbgout,"= %u\n", var);
  return;
}


void CCDBG_DUMP_INT_func(const char* varname, const unsigned int var)
{
  print_indent();
  sprintf(dbgout, "  %s", varname);

  for (int idx = VALUE_INDENT - strlen(varname); idx > 0; idx--)
    sprintf(dbgout, " ");

  sprintf(dbgout,"= %d\n", var);
  return;
}


void CCDBG_DUMP_UINT24_T_BIN_func(const char* varname, const uint24_t var)
{
  char b_char;

  print_indent();
  sprintf(dbgout, "  %s", varname);

  for (int i = VALUE_INDENT - strlen(varname); i > 0; i--)
    sprintf(dbgout, " ");

  sprintf(dbgout, "= ");

  for (int byte = 24; byte >= 0; byte--)
  {
    b_char = ((var & (1 << byte)) ? '1' : '0');
    sprintf(dbgout, "%c", b_char);

    if (!(byte % 4) && byte < 24)
      sprintf(dbgout, " ");
  }

  sprintf(dbgout,"\n");
  return;
}

#endif
