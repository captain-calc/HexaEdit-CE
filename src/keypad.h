// Name:    Captain Calc
// File:    keypad.h
// Purpose: Provides functions for scanning the keypad and acknowledging
//          keypresses.


/*
BSD 3-Clause License

Copyright (c) 2024, Caleb "Captain Calc" Arant
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/


#ifndef KEYMAP_H
#define KEYMAP_H


#include <keypadc.h>


uint8_t keypad_ExclusiveKeymap(
  const char* const keymap[8], uint8_t* const value
);
bool keypad_ExclusiveASCII(uint8_t* const value, const char mode);
bool keypad_ExclusiveNibble(uint8_t* const value);


// Description: Checks if a key is pressed and if so, blocks until it is
//              released. If any other key is pressed during the block, the
//              function will block until all keys are released and not register
//              any keypress.
// Pre:         <key> must be a valid long keycode.
// Post:        Returns true when <key> is released if no other key was pressed
//              during the time <key> was pressed.
//              Returns false if <key> was not pressed or if any other key was
//              pressed while <key> was pressed.
bool keypad_SinglePressExclusive(kb_lkey_t key);


// Description: Determines if a key is pressed or held. The <threshold>
//              parameter adjusts the delay between the first press and the held
//              state.
// Pre:         <key> must be a valid long keycode.
// Post:        Returns true when key is first pressed or if the key has been
//              held for <threshold> number of calls to the function.
//              Returns false if key is not pressed or if the function has been
//              called between 1 and <threshold> times while the key is held.
bool keypad_KeyPressedOrHeld(kb_lkey_t key, uint8_t threshold);


// Description: Loops until a key is pressed.
// Pre:         None
// Post:        If a key has not been pressed for one cycle, it resets the
//              counter for each key used by keypad_KeyPressedOrHeld().
void keypad_IdleKeypadBlock(void);


#endif
