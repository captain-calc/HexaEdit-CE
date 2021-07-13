// Name:    Captain Calc
// Date:    July 12, 2021
// File:    cutil.h
// Purpose: cutil (C-utilities) provides common C routines used by more than
//          one file.


#ifndef CUTIL_H
#define CUTIL_H


#include <stdint.h>  // For uint8_t


// FUNCTION DECLARATIONS
// ============================================================================


// Description: cutil_PtrSprintf() converts an uint8_t pointer into a string.
//              It replaces the spaces that sprintf() substitutes for NULLs
//              with zeros. Format: 0xXXXXXX.
// Pre:         <buffer> should be large enough to accomodate a memory pointer
//              plus the 2-byte prefix.
// Post:        <buffer> holds the newly formatted pointer string.
void cutil_PtrSprintf(char *buffer, uint8_t *ptr);


// Description: cutil_ToDecimal() converts a hexadecimal string to an uint24_t.
// Pre:         <hex> must only contain valid hexadecimal characters. Its
//              decimal representation must be less than the maximum value of
//              an uint24_t.
// Post:        Decimal equivalent of <hex> returned.
uint24_t cutil_ToDecimal(const char *hex);


#endif