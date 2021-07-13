// Name:    Captain Calc
// Date:    July 13, 2021
// File:    settings.h
// Purpose: settings provides functions for the Settings menu of HexaEdit and
//          accessing the global phrase search range.


#ifndef SETTINGS_H
#define SETTINGS_H


#include <stdint.h>  // For uint24_t
#include "editor.h"  // For *_MIN_ADDRESS and *_MAX_ADDRESS


// FILE DEFINES
// ============================================================================


#define ROM_SEARCH_RANGE   ((uint24_t)(ROM_MAX_ADDRESS - ROM_MIN_ADDRESS))
#define RAM_SEARCH_RANGE   ((uint24_t)(RAM_MAX_ADDRESS - RAM_MIN_ADDRESS))
#define PORTS_SEARCH_RANGE ((uint24_t)(PORTS_MAX_ADDRESS - PORTS_MIN_ADDRESS))


// FUNCTION DECLARATIONS
// ============================================================================


// Description: 
// Pre:         
// Post:        
bool settings_InitSettingsAppvar(void);


// Description: 
// Pre:         
// Post:        
uint24_t settings_GetPhraseSearchRange(void);


// Description: 
// Pre:         
// Post:        
void settings_Settings(void);


#endif