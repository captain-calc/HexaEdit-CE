#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>

#define ROM_SEARCH_RANGE    ((uint24_t)5000000)
#define RAM_SEARCH_RANGE    ((uint24_t)200000)

bool settings_InitSettingsAppvar(void);
uint24_t settings_GetPhraseSearchRange(void);
void settings_Settings(void);


#endif