// Name:    Captain Calc
// Date:    July 13, 2021
// File:    asmutil.h
// Purpose: asmutil provides common assembly routines used by more than one
//          file.


#ifndef ASMUTIL_H
#define ASMUTIL_H


#include <stdint.h>  // For uint8_t


// FUNCTION DECLARATIONS
// ============================================================================


// Description: asm_GetCSC() returns the sk_key_t code for a single keypress.
//              It combines the speed of a kb_Data register check with the
//              convenience of the single sk_key_t code. Note: This routine
//              cannot detect simultaneous keypresses. Use multiple kb_Data
//              checks to detect simultaneous keypresses.
// Pre:         None
// Post:        sk_key_t code returned if key pressed. If no key was pressed,
//              -1 returned.
int8_t asm_GetCSC(void);


// Description: asm_CopyData() copies <amount> bytes from <src> to <dest>.
//              If <copy_direction> == 0, the routine decrements the copy
//              address. If <copy_direction> == 1, the routine increments the
//              copy address.
// Pre:         All of the memory from <src> to <src> +/- <amount> and from
//              <dest> to <dest> +/- <amount> must be write-access memory,
//              depending on <copy_direction>.
// Post:        <amount> bytes copied from <src> to <dest> in given
//              <copy_direction>.
void asm_CopyData(
  void *src,
  void *dest,
  uint24_t amount,
  uint8_t copy_direction
);


// Description: Shifts low nibble of <nibble> into the high nibble of <nibble>,
//              e.g. 00001001 -> 10010000. High nibble is overwritten and low
//              nibble becomes 0000 in binary.
// Pre:         None
// Post:        Low nibble of <nibble> shifted to high nibble.
uint8_t asm_LowToHighNibble(uint8_t nibble);


// Description: Shifts high nibble of <nibble> into the low nibble of <nibble>,
//              e.g. 10010000 -> 00001001. Low nibble is overwritten and high
//              nibble becomes 0000 in binary.
// Pre:         None
// Post:        High nibble of <nibble> shifted to low nibble.      
uint8_t asm_HighToLowNibble(uint8_t nibble);


// Description: Finds all occurances of <phrase> in memory from <start> to
//              <end>. Pointers to the start of the match are written to
//              <matches>. When a match is found and recorded, searching
//              resumes from (pointer to match start) + <len>.
// Pre:         <start> and <end> must be valid memory pointers and <end>
//              must be greater than or equal to <start>. <len> must be the
//              length of phrase. <max_matches> must be the size of the
//              <matches> array.
// Post:        <matches> contains pointers to every occurance of <phrase>
//              found between <start> and <end>, inclusive.
uint8_t asm_BFind_All(
  const uint8_t *start,
  const uint8_t *end,
  const char phrase[],
  const uint8_t len,
  uint8_t **matches,
  uint8_t max_matches
);


#endif