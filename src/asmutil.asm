; Name:    Captain Calc
; Date:    July 13, 2021
; File:    asmutil.asm
; Purpose: Provides the definitions of the functions declared in asmutil.h.
;          Also defines any internal routines used by the public functions.


	public _asm_GetCSC
_asm_GetCSC:
; Arguments:
;   None
; Returns:
;   A == os_GetCSC offset if key pressed; A == -1 if no key pressed
; Destroys:
;   BC and HL

	ld	hl,$f50010
	xor	a,a
	ld	b,0
.scanGroup:
	inc	b
	bit	3,b
	jr	nz,.notFound
	inc	hl
	inc	hl
	or	a,(hl)
	jr	z,.scanGroup

	ld	c,255
.getKey:
	inc	c
	srl	a
	jr	nz,.getKey
	ld	a,c

; Return 57 - 8 * B + A
.finish:
	ld	c,8
	mlt	bc
	add	a,57
	sub	a,c
	ret

.notFound:
	ld	a,-1
	ret


	public	_asm_CopyData
_asm_CopyData:
; Arguments:
;   arg0 = pointer to copy data from
;   arg1 = pointer to copy data to
;   arg2 = amount of data
;   arg3 = copy direction (0 = copy data and DECREMENT offset)
;                         (1 = copy data and INCREMENT offset)
; Returns:
;   None
; Destroys:
;   A, BC, DE, HL, IY

	ld	iy, 0
	add	iy, sp
	ld	hl, (iy + 3)
	ld	de, (iy + 6)
	ld	bc, (iy + 9)
	push	hl
	ld	hl, (iy + 12)
	xor	a, a
	cp	a, l
	pop	hl
	jr	z, .decrement
	ldir
	ret
.decrement:
	lddr
	ret


	public _asm_LowToHighNibble
_asm_LowToHighNibble:
; Arguments:
;   arg0 = nibble
; Returns:
;   A = byte with low nibble in high (NNNN 0000)
; Destroys:
;   A, DE, HL

	pop	de
	pop	hl
	push	hl
	push	de
	ld	a,l
	sla	a
	sla	a
	sla	a
	sla	a
	ret


	public _asm_HighToLowNibble
_asm_HighToLowNibble:
; Arguments:
;   arg0 = nibble
; Returns:
;   A = byte with high nibble in low (0000 NNNN)
; Destroys:
;   A, DE, HL

	pop	de
	pop	hl
	push	hl
	push	de
	ld	a,l
	srl	a
	srl	a
	srl	a
	srl	a
	ret


; Arguments:
;   arg0 = start address
;   arg1 = end address
;   arg2 = phrase
;   arg3 = phrase length
; Returns:
;   HL == pointer to first match found
;   Carry flag set if match found; reset if no match found
; Destroys:
;   A, BC, DE, IY

_bfind:
	ld	iy,0
	add	iy,sp
	ld	bc,(iy + 3)
	ld	hl,(iy + 6)
	push	hl

; If start > end, exit
  push  hl
  xor a,a
  sbc hl,bc
  pop hl
  jr  nc,.cont
  xor a,a             ; Reset carry flag
  pop hl
  ret

.cont:
  ld  de,0            ; E = phrase offset

.compare:
  ld  a,(bc)
  ld  hl,(iy + 9)
  add hl,de
  cp  a,(hl)
  jr  z,.match

; Comparison failed:
  ld  e,0             ; Reset phrase offset

; If end of range reached, exit
.endCheck:
  pop hl
  push  hl
  xor a,a
  sbc hl,bc
  jr  z,.exitFast     ; Carry flag is still reset if Z is set
  
  inc bc              ; Increment current address
  jr  .compare
  
.match:
  inc e               ; Increment phrase offset counter
  ld  a,(iy + 12)
  xor a,e
  jr  nz,.endCheck

; Full match found:
  push  bc
  pop hl
  dec e
  sbc hl,de           ; Carry reset by XOR A,E
  scf                 ; We found a match
.exitFast:
  pop bc              ; Remove the stack local
  ret


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

  public _asm_BFind_All
_asm_BFind_All:
  ld  iy,0
  add iy,sp
  ld  hl,(iy + 12)
  push  hl
  ld  hl,(iy + 9)
  push  hl
  ld  hl,(iy + 6)
  push  hl
  
  ld  hl,(iy + 15)    ; HL -> match pointer array
  ld  bc,0
.lastMatchInArray:=$-3
  ld  (.lastMatchInArray),hl
  ld  bc,0
  ld  c,(iy + 18)     ; C  == max number of match pointers
  ld  b,3             ; 3 bytes is the size of a pointer
  mlt bc
  add hl,bc
  ld  bc,0
.endOfArray:=$-3
  ld  (.endOfArray),hl
  ld  a,0
.numMatchesFound:=$-1
  xor a,a
  ld  (.numMatchesFound),a
  
  ld  hl,(iy + 3)

.compare:
  push  hl            ; HL == start address
  call  _bfind
  pop bc
  push  hl

; If bfind does not return a match, exit
  sbc hl,hl           ; If carry flag is reset, this will not set it again
  jr  nc,.finish

; Check for room in the match array
  ld  hl,(.endOfArray)
  ld  de,(.lastMatchInArray)
  xor a,a
  sbc hl,de
  jr  z,.finish

; Increment numMatchesFound
  ld  a,(.numMatchesFound)
  inc a
  ld  (.numMatchesFound),a

; Write new match address to array
  ex  de,hl
  pop de              ; DE == bfind return value
  ld  (hl),de
  inc hl
  inc hl
  inc hl
  ld  (.lastMatchInArray),hl

; Add phrase length to new match address to get new start address
  ld  hl,0
  ld  l,(iy + 12)
  add hl,de
  jr  .compare

.finish:
    pop hl            ; bfind return value
    pop hl            ; phrase length
    pop hl            ; phrase
    pop hl            ; end address
    ld  a,(.numMatchesFound)
    ret