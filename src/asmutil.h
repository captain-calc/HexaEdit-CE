#ifndef ASMUTIL_H
#define ASMUTIL_H

#include <stdint.h>

void asm_CopyVRAM(uint24_t x_src, uint8_t y_src, uint24_t width, uint8_t height, uint24_t x_dest, uint8_t y_dest);

int8_t asm_GetCSC(void);
void asm_CopyData(void *from, void *to, uint24_t amount, uint8_t copy_direction);
uint8_t asm_LowToHighNibble(uint8_t nibble);
uint8_t asm_HighToLowNibble(uint8_t nibble);

uint8_t asm_BFind_All(const uint8_t *start, const uint8_t *end, const char phrase[], const uint8_t len, uint8_t **matches, uint8_t max_matches);


#endif