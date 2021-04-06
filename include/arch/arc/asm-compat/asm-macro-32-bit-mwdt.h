/* SPDX-License-Identifier: GPL-2.0-only OR Apache-2.0 */
/*
 * Copyright (C) 2021 Synopsys, Inc. (www.synopsys.com)
 *
 * ALU/Memory instructions pseudo-mnemonics for ARCv2 and ARC32 ISA
 */

.macro MOVR, d, s
	mov d, s
.endm

.macro MOVR.hi, d, s
	mov.hi d, s
.endm

.macro MOVR.nz, d, s
	mov.nz d, s
.endm

.macro LDR, d, s, off
	.if $narg == 2
		ld  d, [s]
	.else
		ld  d, [s, off]
	.endif
.endm

.macro LDR.ab, d, s, off
	.if $narg == 2
		ld.ab  d, [s]
	.else
		ld.ab  d, [s, off]
	.endif
.endm

.macro LDR.as, d, s, off
	.if $narg == 2
		ld.as  d, [s]
	.else
		ld.as  d, [s, off]
	.endif
.endm

.macro LDR.aw, d, s, off
	.if $narg == 2
		ld.aw  d, [s]
	.else
		ld.aw  d, [s, off]
	.endif
.endm

.macro STR, d, s, off
	.if $narg == 2
		st  d, [s]
	.else
		st  d, [s, off]
	.endif
.endm

.macro STR.ab, d, s, off
	.if $narg == 2
		st.ab  d, [s]
	.else
		st.ab  d, [s, off]
	.endif
.endm

.macro STR.as, d, s, off
	.if $narg == 2
		st.as  d, [s]
	.else
		st.as  d, [s, off]
	.endif
.endm

.macro STR.aw, d, s, off
	.if $narg == 2
		st.aw  d, [s]
	.else
		st.aw  d, [s, off]
	.endif
.endm

// .macro STR, d, s, off=0
// 	; workaround assembler barfing for ST r, [@symb, 0]
// 	.if     off == 0
// 		st  d, [s]
// 	.else
// 		st  d, [s, off]
// 	.endif
// .endm
//
// .macro STR.ab, d, s, off=0
// 	; workaround assembler barfing for ST r, [@symb, 0]
// 	.if     off == 0
// 		st.ab  d, [s]
// 	.else
// 		st.ab  d, [s, off]
// 	.endif
// .endm
//
// .macro STR.as, d, s, off=0
// 	; workaround assembler barfing for ST r, [@symb, 0]
// 	.if     off == 0
// 		st.as  d, [s]
// 	.else
// 		st.as  d, [s, off]
// 	.endif
// .endm


.macro PUSHR, r
	push r
.endm

.macro POPR, r
	pop r
.endm

.macro LRR, d, aux
	lr d, aux
.endm

.macro SRR, d, aux
	sr d, aux
.endm


.macro ADDR, d, s, v
	add d, s, v
.endm

.macro ADDR.nz, d, s, v
	add.nz d, s, v
.endm


.macro ADD2R, d, s, v
	add2 d, s, v
.endm

.macro ADD2R.nz, d, s, v
	add2.nz d, s, v
.endm


.macro ADD3R, d, s, v
	add3 d, s, v
.endm

.macro SUBR, d, s, v
	sub d, s, v
.endm

.macro BMSKNR, d, s, v
	bmskn d, s, v
.endm

.macro LSRR, d, s, v
	lsr d, s, v
.endm

.macro ASLR, d, s, v
	asl d, s, v
.endm

.macro BRR.ne, d, s, lbl
	br.ne d, s, lbl
.endm

.macro BRR.eq, d, s, lbl
	br.eq d, s, lbl
.endm

.macro CMPR, op1, op2
	cmp op1, op2
.endm
