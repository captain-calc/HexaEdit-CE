// Name:    Captain Calc
// File:    ccdbg.h
// Purpose: ccdbg provides macros for pretty-printing variable values.
// Version: 0.2.4


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


// NOTE ON SPRINTF()
//
// As of version 0.2.4, ccdbg can use the built-in sprintf() that the TI-OS has.
// If you add `HAS_PRINTF = NO` to your program's Makefile, you can remove about
// 8000 bytes from the debug binary.


// CHANGE LOG
//
//   Version    Description
//
//     0.1.0    Added CCDBG_DUMP_BIN()
//
//     0.1.1    Added USE_CCDBG. This flag removes the necessity to put
//              #if NDEBUG \ #endif commands around debug statements. This was
//              necessary in version 0.1.0 if the programmer wanted to run
//              make without debug and leave the debugging statements in the
//              program code.
//
//     0.2.0    Converted several macros into functions to reduce overall
//              program size when building with debug. Removed
//              CCDBG_PRINT_SECTION_HEADER. Renamed CCDBG_PRINT_MSG to
//              CCDBG_PUTS.
//
//     0.2.1    Removed "#define dbg_sprintf  sprintf". This allows programmers
//              to use debug.h and ccdbg.h simultaneously without redefinition
//              warnings.
//
//     0.2.2    Changed BLOCK_INDENT from 4 to 2.
//
//     0.2.3    Changed buggy CCDBG_DUMP_BIN to CCDBG_DUMP_UINT24_T_BIN.
//
//     0.2.4    Moved the code for all of the remaining macros into functions
//              to reduce program size when running `make debug`. Renamed
//              CCDBG_OPEN_CHECKPOINT() and CCDBG_CLOSE_CHECKPOINT() to
//              CCDBG_BEGINBLOCK() and CCDBG_ENDBLOCK(), respectively. Patched
//              buggy CCDBG_DUMP_STR().


#ifndef CCDBG_H
#define CCDBG_H

#ifdef __cplusplus
extern "C" {
#endif


#ifndef NDEBUG
  #define USE_CCDBG 1
#endif


#if USE_CCDBG

  #define dbgout ((char*)0xFB0000)
  #define dbgerr ((char*)0xFC0000)

  #define BLOCK_INDENT  2
  #define VALUE_INDENT 50
  extern unsigned int g_ccdbg_chkpt_indent;

  // These functions are not designed to be called directly. Access them
  // through their corresponding macros.
  void print_indent();
  void CCDBG_DUMP_STR_func(const char* varname, const char* var);
  void CCDBG_DUMP_PTR_func(const char* varname, const unsigned int var);
  void CCDBG_DUMP_UINT_func(const char* varname, const unsigned int var);
  void CCDBG_DUMP_INT_func(const char* varname, const unsigned int var);

  // LIBRARY FUNCTIONS
  // ===========================================================================

  void CCDBG_BEGINBLOCK(const char* name);
  
  void CCDBG_ENDBLOCK();

  void CCDBG_PUTS(const char* s);

  #define CCDBG_DUMP_PTR(varname) \
    do { CCDBG_DUMP_PTR_func(#varname, (unsigned int)varname); } while (0)

  #define CCDBG_DUMP_UINT(varname) \
    do { CCDBG_DUMP_UINT_func(#varname, varname); } while (0)

  #define CCDBG_DUMP_INT(varname) \
    do { CCDBG_DUMP_INT_func(#varname, varname); } while (0)

  #define CCDBG_DUMP_STR(varname) \
    do { CCDBG_DUMP_STR_func(#varname, varname); } while (0)

  #define CCDBG_DUMP_UINT24_T_BIN(varname) \
    do { CCDBG_DUMP_UINT24_T_BIN_func(#varname, varname); } while (0)

  // ===========================================================================

#else

  #define CCDBG_BEGINBLOCK(...)        ((void)0)
  #define CCDBG_ENDBLOCK(...)          ((void)0)
  #define CCDBG_PUTS(...)              ((void)0)
  #define CCDBG_DUMP_PTR(...)          ((void)0)
  #define CCDBG_DUMP_INT(...)          ((void)0)
  #define CCDBG_DUMP_UINT(...)         ((void)0)
  #define CCDBG_DUMP_STR(...)          ((void)0)
  #define CCDBG_DUMP_UINT24_T_BIN(...) ((void)0)

#endif

#if __cplusplus
}
#endif

#endif
