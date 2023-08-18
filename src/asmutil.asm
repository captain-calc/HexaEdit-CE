; Name:    Captain Calc
; File:    asmutil.asm
; Purpose: Provides the definitions of the functions declared in asmutil.h.
;          Also defines any internal routines used by the public functions.


; BSD 3-Clause License
;
; Copyright (c) 2023, Caleb "Captain Calc" Arant
; All rights reserved.
;
; Redistribution and use in source and binary forms, with or without
; modification, are permitted provided that the following conditions are met:
;
; 1. Redistributions of source code must retain the above copyright notice, this
;    list of conditions and the following disclaimer.
;
; 2. Redistributions in binary form must reproduce the above copyright notice,
;    this list of conditions and the following disclaimer in the documentation
;    and/or other materials provided with the distribution.
;
; 3. Neither the name of the copyright holder nor the names of its
;    contributors may be used to endorse or promote products derived from
;    this software without specific prior written permission.
;
; THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
; AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
; IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
; DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
; FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
; DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
; SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
; CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
; OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
; OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


assume ADL=1

section .text


  public  _asmutil_IsNamedVar
_asmutil_IsNamedVar:
; Arguments:
;   arg0 = variable type
; Returns:
;   True if variable has an ASCII name. False otherwise.
; Destroys:
;   A, DE, HL
; Notes:
;   Table of variable types:
;   https://wikiti.brandonw.net/index.php?title=83Plus:OS:System_Table


  pop  de
  pop  hl
  push hl
  push de
  ld   a,l
  cp   a,$05            ; PRGM
  ret  z
  cp   a,$06            ; PROT PRGM
  ret  z
  cp   a,$15            ; APPVAR
  ret  z
  cp   a,$17            ; GROUP
  ret  z
  xor  a,a
  ret

; -----------------------------------------------------------------------------

	public	_asmutil_CopyData
_asmutil_CopyData:
; Arguments:
;   arg0 == pointer to copy data from
;   arg1 == pointer to copy data to
;   arg2 == amount of data
;   arg3 == copy direction (0 = copy data and DECREMENT offset)
;                          (1 = copy data and INCREMENT offset)
; Returns:
;   None
; Destroys:
;   A, BC, DE, HL, IY


  ld    iy,0
  add   iy,sp
  ld    hl,(iy + 3)
  ld    de,(iy + 6)
  ld    bc,(iy + 9)
  push  hl
  xor   a,a
  ld    hl,0
  sbc   hl,bc
  jr    nc,.bc_is_zero
  ld    hl,(iy + 12)
  cp    a,l
  pop   hl
  jr    z,.decrement
  ldir
  ret
.decrement:
  lddr
  ret
.bc_is_zero:
  pop hl
  ret

; -----------------------------------------------------------------------------

_find_phrase:
; Arguments:
;   arg0 == start address
;   arg1 == end address
;   arg2 == phrase
;   arg3 == phrase length
; Returns:
;   HL == pointer to first match found
;   Carry flag set if match found; reset if no match found
; Destroys:
;   A, BC, DE, IY


	ld	  iy,0
	add	  iy,sp
	ld	  bc,(iy + 3)
	ld	  hl,(iy + 6)
	push	hl

; If start > end, exit
  push  hl
  xor   a,a
  sbc   hl,bc
  pop   hl
  jr    nc,.cont
  xor   a,a             ; Reset carry flag
  pop   hl
  ret

.cont:
  ld    de,0            ; E = phrase offset

.compare:
  ld    a,(bc)
  ld    hl,(iy + 9)
  add   hl,de
  cp    a,(hl)
  jr    z,.match

; Comparison failed:
  ld    e,0             ; Reset phrase offset

; If end of range reached, exit
.endCheck:
  pop   hl
  push  hl
  xor   a,a
  sbc   hl,bc
  jr    z,.exitFast     ; Carry flag is still reset if Z is set
  
  inc   bc              ; Increment current address
  jr    .compare
  
.match:
  inc   e               ; Increment phrase offset counter
  ld    a,(iy + 12)
  xor   a,e
  jr    nz,.endCheck

; Full match found:
  push  bc
  pop   hl
  dec   e
  sbc   hl,de           ; Carry reset by XOR A,E
  scf                   ; We found a match
.exitFast:
  pop   bc              ; Remove the stack local
  ret

; -----------------------------------------------------------------------------

  public _asmutil_FindPhrase
_asmutil_FindPhrase:
; Arguments:
;   arg0 == start address
;   arg1 == end address
;   arg2 -> phrase
;   arg3 == phrase length
;   arg4 -> pointer array for match pointers
;   arg5 == number of match pointers allowed
; Returns:
;   A == number of matches found
; Destroys:
;   BC, DE, HL


  ld    iy,0
  add   iy,sp
  ld    hl,(iy + 12)
  push  hl
  ld    hl,(iy + 9)
  push  hl
  ld    hl,(iy + 6)
  push  hl
  
  ld    hl,(iy + 15)    ; HL -> match pointer array
  ld    bc,0
.lastMatchInArray:=$-3
  ld    (.lastMatchInArray),hl
  ld    bc,0
  ld    c,(iy + 18)     ; C  == max number of match pointers
  ld    b,3             ; 3 bytes is the size of a pointer
  mlt   bc
  add   hl,bc
  ld    bc,0
.endOfArray:=$-3
  ld    (.endOfArray),hl
  ld    a,0
.numMatchesFound:=$-1
  xor   a,a
  ld    (.numMatchesFound),a
  
  ld  hl,(iy + 3)

.compare:
  push  hl            ; HL == start address
  call  _find_phrase
  pop   bc
  push  hl

; If bfind does not return a match, exit
  sbc   hl,hl           ; If carry flag is reset, this will not set it again
  jr    nc,.finish

; Check for room in the match array
  ld    hl,(.endOfArray)
  ld    de,(.lastMatchInArray)
  xor   a,a
  sbc   hl,de
  jr    z,.finish

; Increment numMatchesFound
  ld    a,(.numMatchesFound)
  inc   a
  ld    (.numMatchesFound),a

; Write new match address to array
  ex    de,hl
  pop   de              ; DE == _find_phrase return value
  ld    (hl),de
  inc   hl
  inc   hl
  inc   hl
  ld    (.lastMatchInArray),hl

; Add phrase length to new match address to get new start address
  ld    hl,0
  ld    l,(iy + 12)
  add   hl,de
  jr    .compare

.finish:
  pop   hl            ; _find_phrase return value
  pop   hl            ; phrase length
  pop   hl            ; phrase
  pop   hl            ; end address
  ld    a,(.numMatchesFound)
  ret