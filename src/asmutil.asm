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
