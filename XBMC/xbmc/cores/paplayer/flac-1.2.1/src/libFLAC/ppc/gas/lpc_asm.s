#  libFLAC - Free Lossless Audio Codec library
#  Copyright (C) 2004,2005,2006,2007  Josh Coalson
#
#  Redistribution and use in source and binary forms, with or without
#  modification, are permitted provided that the following conditions
#  are met:
#
#  - Redistributions of source code must retain the above copyright
#  notice, this list of conditions and the following disclaimer.
#
#  - Redistributions in binary form must reproduce the above copyright
#  notice, this list of conditions and the following disclaimer in the
#  documentation and/or other materials provided with the distribution.
#
#  - Neither the name of the Xiph.org Foundation nor the names of its
#  contributors may be used to endorse or promote products derived from
#  this software without specific prior written permission.
#
#  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
#  ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
#  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
#  A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE FOUNDATION OR
#  CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
#  EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
#  PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
#  PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
#  LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
#  NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
#  SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

.text
	.align 2
.globl _FLAC__lpc_restore_signal_asm_ppc_altivec_16
.type _FLAC__lpc_restore_signal_asm_ppc_altivec_16, @function

.globl _FLAC__lpc_restore_signal_asm_ppc_altivec_16_order8
.type _FLAC__lpc_restore_signal_asm_ppc_altivec_16_order8, @function

_FLAC__lpc_restore_signal_asm_ppc_altivec_16:
#	r3: residual[]
#	r4: data_len
#	r5: qlp_coeff[]
#	r6: order
#	r7: lp_quantization
#	r8: data[]

# see src/libFLAC/lpc.c:FLAC__lpc_restore_signal()
# these is a PowerPC/Altivec assembly version which requires bps<=16 (or actual
# bps<=15 for mid-side coding, since that uses an extra bit)

# these should be fast; the inner loop is unrolled (it takes no more than
# 3*(order%4) instructions, all of which are arithmetic), and all of the
# coefficients and all relevant history stay in registers, so the outer loop
# has only one load from memory (the residual)

# I have not yet run this through simg4, so there may be some avoidable stalls,
# and there may be a somewhat more clever way to do the outer loop

# the branch mechanism may prevent dynamic loading; I still need to examine
# this issue, and there may be a more elegant method

	stmw r31,-4(r1)

	addi r9,r1,-28
	li r31,0xf
	andc r9,r9,r31 # for quadword-aligned stack data

	slwi r6,r6,2 # adjust for word size
	slwi r4,r4,2
	add r4,r4,r8 # r4 = data+data_len

	mfspr r0,256 # cache old vrsave
	addis r31,0,0xffff
	ori r31,r31,0xfc00
	mtspr 256,r31 # declare VRs in vrsave

	cmplw cr0,r8,r4 # i<data_len
	bc 4,0,L1400

	# load coefficients into v0-v7 and initial history into v8-v15
	li r31,0xf
	and r31,r8,r31 # r31: data%4
	li r11,16
	subf r31,r31,r11 # r31: 4-(data%4)
	slwi r31,r31,3 # convert to bits for vsro
	li r10,-4
	stw r31,-4(r9)
	lvewx v0,r10,r9
	vspltisb v18,-1
	vsro v18,v18,v0 # v18: mask vector

	li r31,0x8
	lvsl v0,0,r31
	vsldoi v0,v0,v0,12
	li r31,0xc
	lvsl v1,0,r31
	vspltisb v2,0
	vspltisb v3,-1
	vmrglw v2,v2,v3
	vsel v0,v1,v0,v2 # v0: reversal permutation vector

	add r10,r5,r6
	lvsl v17,0,r5 # v17: coefficient alignment permutation vector
	vperm v17,v17,v17,v0 # v17: reversal coefficient alignment permutation vector

	mr r11,r8
	lvsl v16,0,r11 # v16: history alignment permutation vector

	lvx v0,0,r5
	addi r5,r5,16
	lvx v1,0,r5
	vperm v0,v0,v1,v17
	lvx v8,0,r11
	addi r11,r11,-16
	lvx v9,0,r11
	vperm v8,v9,v8,v16
	cmplw cr0,r5,r10
	bc 12,0,L1101
	vand v0,v0,v18
	addis r31,0,L1307@ha
	ori r31,r31,L1307@l
	b L1199

L1101:
	addi r5,r5,16
	lvx v2,0,r5
	vperm v1,v1,v2,v17
	addi r11,r11,-16
	lvx v10,0,r11
	vperm v9,v10,v9,v16
	cmplw cr0,r5,r10
	bc 12,0,L1102
	vand v1,v1,v18
	addis r31,0,L1306@ha
	ori r31,r31,L1306@l
	b L1199

L1102:
	addi r5,r5,16
	lvx v3,0,r5
	vperm v2,v2,v3,v17
	addi r11,r11,-16
	lvx v11,0,r11
	vperm v10,v11,v10,v16
	cmplw cr0,r5,r10
	bc 12,0,L1103
	vand v2,v2,v18
	lis r31,L1305@ha
	la r31,L1305@l(r31)
	b L1199

L1103:
	addi r5,r5,16
	lvx v4,0,r5
	vperm v3,v3,v4,v17
	addi r11,r11,-16
	lvx v12,0,r11
	vperm v11,v12,v11,v16
	cmplw cr0,r5,r10
	bc 12,0,L1104
	vand v3,v3,v18
	lis r31,L1304@ha
	la r31,L1304@l(r31)
	b L1199

L1104:
	addi r5,r5,16
	lvx v5,0,r5
	vperm v4,v4,v5,v17
	addi r11,r11,-16
	lvx v13,0,r11
	vperm v12,v13,v12,v16
	cmplw cr0,r5,r10
	bc 12,0,L1105
	vand v4,v4,v18
	lis r31,L1303@ha
	la r31,L1303@l(r31)
	b L1199

L1105:
	addi r5,r5,16
	lvx v6,0,r5
	vperm v5,v5,v6,v17
	addi r11,r11,-16
	lvx v14,0,r11
	vperm v13,v14,v13,v16
	cmplw cr0,r5,r10
	bc 12,0,L1106
	vand v5,v5,v18
	lis r31,L1302@ha
	la r31,L1302@l(r31)
	b L1199

L1106:
	addi r5,r5,16
	lvx v7,0,r5
	vperm v6,v6,v7,v17
	addi r11,r11,-16
	lvx v15,0,r11
	vperm v14,v15,v14,v16
	cmplw cr0,r5,r10
	bc 12,0,L1107
	vand v6,v6,v18
	lis r31,L1301@ha
	la r31,L1301@l(r31)
	b L1199

L1107:
	addi r5,r5,16
	lvx v19,0,r5
	vperm v7,v7,v19,v17
	addi r11,r11,-16
	lvx v19,0,r11
	vperm v15,v19,v15,v16
	vand v7,v7,v18
	lis r31,L1300@ha
	la r31,L1300@l(r31)

L1199:
	mtctr r31

	# set up invariant vectors
	vspltish v16,0 # v16: zero vector

	li r10,-12
	lvsr v17,r10,r8 # v17: result shift vector
	lvsl v18,r10,r3 # v18: residual shift back vector

	li r10,-4
	stw r7,-4(r9)
	lvewx v19,r10,r9 # v19: lp_quantization vector

L1200:
	vmulosh v20,v0,v8 # v20: sum vector
	bcctr 20,0

L1300:
	vmulosh v21,v7,v15
	vsldoi v15,v15,v14,4 # increment history
	vaddsws v20,v20,v21

L1301:
	vmulosh v21,v6,v14
	vsldoi v14,v14,v13,4
	vaddsws v20,v20,v21

L1302:
	vmulosh v21,v5,v13
	vsldoi v13,v13,v12,4
	vaddsws v20,v20,v21

L1303:
	vmulosh v21,v4,v12
	vsldoi v12,v12,v11,4
	vaddsws v20,v20,v21

L1304:
	vmulosh v21,v3,v11
	vsldoi v11,v11,v10,4
	vaddsws v20,v20,v21

L1305:
	vmulosh v21,v2,v10
	vsldoi v10,v10,v9,4
	vaddsws v20,v20,v21

L1306:
	vmulosh v21,v1,v9
	vsldoi v9,v9,v8,4
	vaddsws v20,v20,v21

L1307:
	vsumsws v20,v20,v16 # v20[3]: sum
	vsraw v20,v20,v19 # v20[3]: sum >> lp_quantization

	lvewx v21,0,r3 # v21[n]: *residual
	vperm v21,v21,v21,v18 # v21[3]: *residual
	vaddsws v20,v21,v20 # v20[3]: *residual + (sum >> lp_quantization)
	vsldoi v18,v18,v18,4 # increment shift vector

	vperm v21,v20,v20,v17 # v21[n]: shift for storage
	vsldoi v17,v17,v17,12 # increment shift vector
	stvewx v21,0,r8

	vsldoi v20,v20,v20,12
	vsldoi v8,v8,v20,4 # insert value onto history

	addi r3,r3,4
	addi r8,r8,4
	cmplw cr0,r8,r4 # i<data_len
	bc 12,0,L1200

L1400:
	mtspr 256,r0 # restore old vrsave
	lmw r31,-4(r1)
	blr

_FLAC__lpc_restore_signal_asm_ppc_altivec_16_order8:
#	r3: residual[]
#	r4: data_len
#	r5: qlp_coeff[]
#	r6: order
#	r7: lp_quantization
#	r8: data[]

# see _FLAC__lpc_restore_signal_asm_ppc_altivec_16() above
# this version assumes order<=8; it uses fewer vector registers, which should
# save time in context switches, and has less code, which may improve
# instruction caching

	stmw r31,-4(r1)

	addi r9,r1,-28
	li r31,0xf
	andc r9,r9,r31 # for quadword-aligned stack data

	slwi r6,r6,2 # adjust for word size
	slwi r4,r4,2
	add r4,r4,r8 # r4 = data+data_len

	mfspr r0,256 # cache old vrsave
	addis r31,0,0xffc0
	ori r31,r31,0x0000
	mtspr 256,r31 # declare VRs in vrsave

	cmplw cr0,r8,r4 # i<data_len
	bc 4,0,L2400

	# load coefficients into v0-v1 and initial history into v2-v3
	li r31,0xf
	and r31,r8,r31 # r31: data%4
	li r11,16
	subf r31,r31,r11 # r31: 4-(data%4)
	slwi r31,r31,3 # convert to bits for vsro
	li r10,-4
	stw r31,-4(r9)
	lvewx v0,r10,r9
	vspltisb v6,-1
	vsro v6,v6,v0 # v6: mask vector

	li r31,0x8
	lvsl v0,0,r31
	vsldoi v0,v0,v0,12
	li r31,0xc
	lvsl v1,0,r31
	vspltisb v2,0
	vspltisb v3,-1
	vmrglw v2,v2,v3
	vsel v0,v1,v0,v2 # v0: reversal permutation vector

	add r10,r5,r6
	lvsl v5,0,r5 # v5: coefficient alignment permutation vector
	vperm v5,v5,v5,v0 # v5: reversal coefficient alignment permutation vector

	mr r11,r8
	lvsl v4,0,r11 # v4: history alignment permutation vector

	lvx v0,0,r5
	addi r5,r5,16
	lvx v1,0,r5
	vperm v0,v0,v1,v5
	lvx v2,0,r11
	addi r11,r11,-16
	lvx v3,0,r11
	vperm v2,v3,v2,v4
	cmplw cr0,r5,r10
	bc 12,0,L2101
	vand v0,v0,v6
	lis r31,L2301@ha
	la r31,L2301@l(r31)
	b L2199

L2101:
	addi r5,r5,16
	lvx v7,0,r5
	vperm v1,v1,v7,v5
	addi r11,r11,-16
	lvx v7,0,r11
	vperm v3,v7,v3,v4
	vand v1,v1,v6
	lis r31,L2300@ha
	la r31,L2300@l(r31)

L2199:
	mtctr r31

	# set up invariant vectors
	vspltish v4,0 # v4: zero vector

	li r10,-12
	lvsr v5,r10,r8 # v5: result shift vector
	lvsl v6,r10,r3 # v6: residual shift back vector

	li r10,-4
	stw r7,-4(r9)
	lvewx v7,r10,r9 # v7: lp_quantization vector

L2200:
	vmulosh v8,v0,v2 # v8: sum vector
	bcctr 20,0

L2300:
	vmulosh v9,v1,v3
	vsldoi v3,v3,v2,4
	vaddsws v8,v8,v9

L2301:
	vsumsws v8,v8,v4 # v8[3]: sum
	vsraw v8,v8,v7 # v8[3]: sum >> lp_quantization

	lvewx v9,0,r3 # v9[n]: *residual
	vperm v9,v9,v9,v6 # v9[3]: *residual
	vaddsws v8,v9,v8 # v8[3]: *residual + (sum >> lp_quantization)
	vsldoi v6,v6,v6,4 # increment shift vector

	vperm v9,v8,v8,v5 # v9[n]: shift for storage
	vsldoi v5,v5,v5,12 # increment shift vector
	stvewx v9,0,r8

	vsldoi v8,v8,v8,12
	vsldoi v2,v2,v8,4 # insert value onto history

	addi r3,r3,4
	addi r8,r8,4
	cmplw cr0,r8,r4 # i<data_len
	bc 12,0,L2200

L2400:
	mtspr 256,r0 # restore old vrsave
	lmw r31,-4(r1)
	blr
