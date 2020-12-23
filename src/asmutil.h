#ifndef ASMUTIL_H
#define ASMUTIL_H

#include <stdint.h>

int8_t asm_GetCSC(void);
void asm_CopyData(void *from, void *to, uint24_t amount, uint8_t copy_direction);
uint8_t asm_LowToHighNibble(uint8_t nibble);
uint8_t asm_HighToLowNibble(uint8_t nibble);

#endif