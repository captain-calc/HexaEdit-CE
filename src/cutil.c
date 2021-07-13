// Name:    Captain Calc
// Date:    July 12, 2021
// File:    cutil.c
// Purpose: Provides the definitions for the functions declared in cutil.h.


#include "cutil.h"

#include <stdio.h>  // For sprintf()


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