// Name:    Captain Calc
// Date:    July 12, 2021
// File:    cutil.c
// Purpose: Provides the definitions for the functions declared in cutil.h.


#include "cutil.h"

#include <stdio.h>   // For sprintf()
#include <string.h>  // For strlen()


void cutil_PtrSprintf(char *buffer, uint8_t *ptr)
{
	char *c;
	
	sprintf(buffer, "%6x", (unsigned int)ptr);
	c = buffer;
	
	while (*c != '\0')
	{
		if (*c == ' ')
			*c = '0';
		c++;
	};
	return;
}


uint24_t cutil_ToDecimal(const char *hex)
{
	const char *hex_chars = {"0123456789abcdef"};
	uint8_t i, j;
	uint24_t place = 1;
	uint24_t decimal = 0;
	
	
	i = strlen(hex);
	
	while (i > 0)
	{
		i--;
		
		for (j = 0; j < 16; j++)
		{
			if (*(hex + i) == hex_chars[j])
				decimal += place * j;
		};
		
		place *= 16;
	};
	
	return decimal;
}