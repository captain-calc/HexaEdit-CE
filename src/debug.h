#ifndef DEBUG_H
#define DEBUG_H

#include <stdio.h>
#define dbgout ((char*)0xFB0000)
#define dbgerr ((char*)0xFC0000)
#define dbg_sprintf sprintf

#endif