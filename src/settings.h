#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include "editor.h"  // For *_MIN_ADDRESS and *_MAX_ADDRESS

#define ROM_SEARCH_RANGE   ((uint24_t)(ROM_MAX_ADDRESS - ROM_MIN_ADDRESS))
#define RAM_SEARCH_RANGE   ((uint24_t)(RAM_MAX_ADDRESS - RAM_MIN_ADDRESS))
#define PORTS_SEARCH_RANGE ((uint24_t)(PORTS_MAX_ADDRESS - PORTS_MIN_ADDRESS))

bool settings_InitSettingsAppvar(void);
uint24_t settings_GetPhraseSearchRange(void);
void settings_Settings(void);


#endif