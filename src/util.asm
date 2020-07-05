	.def	_copy_data
_copy_data:
; Arguments:
;   arg0 = pointer to copy data from
;   arg1 = pointer to copy data to
;   arg2 = amount of data
;   arg3 = copy direction (0 = copy data and DECREMENT offset)
;                         (1 = copy data and INCREMENT offset)

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

	.def	_get_min_ram_address
_get_min_ram_address:
; Arguments:
;   None
; Returns:
;   Pointer to lowest valid RAM address

	ld	hl, $D00000
	ret

	.def	_get_max_ram_address
_get_max_ram_address:
; Arguments:
;   None
; Returns:
;   Pointer to highest valid RAM address

	ld	hl, $D65800
	ret