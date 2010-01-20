/*
 * acpu.c - another 6502 CPU emulator
 *
 * Copyright (C) 2007-2009  Piotr Fusik
 *
 * This file is part of ASAP (Another Slight Atari Player),
 * see http://asap.sourceforge.net
 *
 * ASAP is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published
 * by the Free Software Foundation; either version 2 of the License,
 * or (at your option) any later version.
 *
 * ASAP is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty
 * of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with ASAP; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

/* How 6502 registers are stored in this emulator:
   All variables are int, because modern processors (and Java bytecode)
   tend to operate more effectively on these type than narrower ones.
   pc is really an unsigned 16-bit integer.
   a, x, y and s are unsigned 8-bit integers.
   Flags are decomposed into three variables for improved performance.
   c is either 0 or 1.
   nz contains 6502 flags N and Z.
   N is set if (nz >= 0x80). Z is set if ((nz & 0xff) == 0).
   Usually nz is simply assigned the unsigned 8-bit operation result.
   The are just a few operations (ADC in decimal mode, BIT, PLP and RTI)
   where both N and Z may be set. In these cases, N is reflected by the 8th
   (not 7th) bit of nz.
   vdi contains rarely used flags V, D and I, as a combination
   of V_FLAG, D_FLAG and I_FLAG. Other vdi bits are clear.

   "Unofficial" opcodes are not documented as "legal" 6502 opcodes.
   Their operation has been reverse-engineered on Atari 800XL and Atari 65XE.
   Unofficial opcodes are identical to C64's 6510, except for 0x8b and 0xab.
   The operation of "unstable" opcodes is partially uncertain.
   Explanation is welcome.

   Emulation of POKEY timer interrupts is included.
 
   Two preprocessor symbols may be used to strip the size of this emulator.
   Define ACPU_NO_DECIMAL to disable emulation of the BCD mode.
   Define ACPU_NO_UNOFFICIAL to disable emulation of unofficial opcodes. */

#include "asap_internal.h"

CONST_ARRAY(int, opcode_cycles)
/*	0  1  2  3  4  5  6  7  8  9  A  B  C  D  E  F */
	7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6, /* 0x */
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, /* 1x */
	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6, /* 2x */
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, /* 3x */
	6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6, /* 4x */
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, /* 5x */
	6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6, /* 6x */
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, /* 7x */
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, /* 8x */
	2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5, /* 9x */
	2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4, /* Ax */
	2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4, /* Bx */
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, /* Cx */
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7, /* Dx */
	2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6, /* Ex */
	2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7  /* Fx */
END_CONST_ARRAY;

#ifdef ACPU_NO_DECIMAL

#define DO_ADC \
	{ \
		/* binary mode */ \
		V(int, tmp) = a + data + c; \
		c = tmp >> 8; \
		vdi &= D_FLAG | I_FLAG; \
		if (((a ^ data) & 0x80) == 0 && ((data ^ tmp) & 0x80) != 0) \
			vdi += V_FLAG; \
		nz = a = tmp & 0xff; \
	}

#define DO_SBC \
	{ \
		/* binary mode */ \
		V(int, tmp) = a - data - 1 + c; \
		c = (tmp >= 0) ? 1 : 0; \
		vdi &= D_FLAG | I_FLAG; \
		if (((a ^ tmp) & 0x80) != 0 && ((a ^ data) & 0x80) != 0) \
			vdi += V_FLAG; \
		nz = a = tmp & 0xff; \
	}

#else /* ACPU_NO_DECIMAL */

#define DO_ADC \
	if ((vdi & D_FLAG) == 0) { \
		/* binary mode */ \
		V(int, tmp) = a + data + c; \
		c = tmp >> 8; \
		vdi &= D_FLAG | I_FLAG; \
		if (((a ^ data) & 0x80) == 0 && ((data ^ tmp) & 0x80) != 0) \
			vdi += V_FLAG; \
		nz = a = tmp & 0xff; \
	} \
	else { \
		/* decimal mode */ \
		V(int, tmp) = (a & 0x0f) + (data & 0x0f) + c; \
		if (tmp >= 10) \
			tmp = (tmp - 10) | 0x10; \
		tmp += (a & 0xf0) + (data & 0xf0); \
		nz = ((tmp & 0x80) << 1) + ((a + data + c) & 0xff); \
		vdi &= D_FLAG | I_FLAG; \
		if (((a ^ data) & 0x80) == 0 && ((data ^ tmp) & 0x80) != 0) \
			vdi += V_FLAG; \
		if (tmp > 0x9f) \
			tmp += 0x60; \
		c = (tmp > 0xff) ? 1 : 0; \
		a = tmp & 0xff; \
	}

#define DO_SBC \
	if ((vdi & D_FLAG) == 0) { \
		/* binary mode */ \
		V(int, tmp) = a - data - 1 + c; \
		c = (tmp >= 0) ? 1 : 0; \
		vdi &= D_FLAG | I_FLAG; \
		if (((a ^ tmp) & 0x80) != 0 && ((a ^ data) & 0x80) != 0) \
			vdi += V_FLAG; \
		nz = a = tmp & 0xff; \
	} \
	else { \
		/* decimal mode */ \
		V(int, tmp) = a - data - 1 + c; \
		V(int, al) = (a & 0x0f) - (data & 0x0f) - 1 + c; \
		V(int, ah) = (a >> 4) - (data >> 4); \
		if ((al & 0x10) != 0) { \
			al -= 6; \
			ah--; \
		} \
		if ((ah & 0x10) != 0) \
			ah -= 6; \
		c = tmp >= 0 ? 1 : 0; \
		vdi &= D_FLAG | I_FLAG; \
		if (((a ^ tmp) & 0x80) != 0 && ((a ^ data) & 0x80) != 0) \
			vdi += V_FLAG; \
		nz = tmp & 0xff; \
		a = ((ah & 0xf) << 4) + (al & 0x0f); \
	}

#endif /* ACPU_NO_DECIMAL */

#define zGetByte(addr)  dGetByte((addr) & 0xff)

#define PEEK         dGetByte(pc)
#define FETCH        dGetByte(pc++)

#define ABSOLUTE     addr = FETCH; addr += FETCH << 8
#define ABSOLUTE_X   addr = FETCH; addr = (addr + (FETCH << 8) + x) & 0xffff
#define ABSOLUTE_Y   addr = FETCH; addr = (addr + (FETCH << 8) + y) & 0xffff
#define ZPAGE        addr = FETCH
#define ZPAGE_X      addr = (FETCH + x) & 0xff
#define ZPAGE_Y      addr = (FETCH + y) & 0xff
#define INDIRECT_X   addr = (FETCH + x) & 0xff; addr = dGetByte(addr) + (zGetByte(addr + 1) << 8)
#define INDIRECT_Y   addr = FETCH; addr = (dGetByte(addr) + (zGetByte(addr + 1) << 8) + y) & 0xffff
#define NCYCLES_X    if ((addr & 0xff) < x) ast _ cycle++
#define NCYCLES_Y    if ((addr & 0xff) < y) ast _ cycle++

#define PL(dest)     s = (s + 1) & 0xff; dest = dGetByte(0x0100 + s)
#define PLP          PL(vdi); nz = ((vdi & 0x80) << 1) + (~vdi & Z_FLAG); c = vdi & 1; vdi &= V_FLAG | D_FLAG | I_FLAG
#define PH(data)     dPutByte(0x0100 + s, data); s = (s - 1) & 0xff
#define PHW(data)    PH((data) >> 8); PH(TO_BYTE(data))
#define PHP(bflag)   PH(((nz | (nz >> 1)) & 0x80) + vdi + ((nz & 0xff) == 0 ? Z_FLAG : 0) + c + bflag)
#define PHPB0        PHP(0x20)  /* push flags with B flag clear (NMI, IRQ) */
#define PHPB1        PHP(0x30)  /* push flags with B flag set (PHP, BRK) */
#define PHPC         PHW(pc)

#define LDA          nz = a = GetByte(addr)
#define LDA_ZP       nz = a = dGetByte(addr)
#define LDX          nz = x = GetByte(addr)
#define LDX_ZP       nz = x = dGetByte(addr)
#define LDY          nz = y = GetByte(addr)
#define LDY_ZP       nz = y = dGetByte(addr)
#define LAX          nz = x = a = GetByte(addr)
#define LAX_ZP       nz = x = a = dGetByte(addr)
#define STA          PutByte(addr, a)
#define STA_ZP       dPutByte(addr, a)
#define STX          PutByte(addr, x)
#define STX_ZP       dPutByte(addr, x)
#define STY          PutByte(addr, y)
#define STY_ZP       dPutByte(addr, y)
#define SAX          data = a & x; PutByte(addr, data)
#define SAX_ZP       data = a & x; dPutByte(addr, data)
#define CMP          nz = GetByte(addr); c = (a >= nz) ? 1 : 0; nz = (a - nz) & 0xff
#define CMP_ZP       nz = dGetByte(addr); c = (a >= nz) ? 1 : 0; nz = (a - nz) & 0xff
#define CPX          nz = GetByte(addr); c = (x >= nz) ? 1 : 0; nz = (x - nz) & 0xff
#define CPX_ZP       nz = dGetByte(addr); c = (x >= nz) ? 1 : 0; nz = (x - nz) & 0xff
#define CPY          nz = GetByte(addr); c = (y >= nz) ? 1 : 0; nz = (y - nz) & 0xff
#define CPY_ZP       nz = dGetByte(addr); c = (y >= nz) ? 1 : 0; nz = (y - nz) & 0xff
#define AND          nz = a &= GetByte(addr)
#define AND_ZP       nz = a &= dGetByte(addr)
#define ORA          nz = a |= GetByte(addr)
#define ORA_ZP       nz = a |= dGetByte(addr)
#define EOR          nz = a ^= GetByte(addr)
#define EOR_ZP       nz = a ^= dGetByte(addr)
#define ADC          data = GetByte(addr); DO_ADC
#define ADC_ZP       data = dGetByte(addr); DO_ADC
#define SBC          data = GetByte(addr); DO_SBC
#define SBC_ZP       data = dGetByte(addr); DO_SBC

#define ASL          RMW_GetByte(nz, addr); c = nz >> 7; nz = (nz << 1) & 0xff; PutByte(addr, nz)
#define ASL_ZP       nz = dGetByte(addr); c = nz >> 7; nz = (nz << 1) & 0xff; dPutByte(addr, nz)
#define ROL          RMW_GetByte(nz, addr); nz = (nz << 1) + c; c = nz >> 8; nz &= 0xff; PutByte(addr, nz)
#define ROL_ZP       nz = dGetByte(addr); nz = (nz << 1) + c; c = nz >> 8; nz &= 0xff; dPutByte(addr, nz)
#define LSR          RMW_GetByte(nz, addr); c = nz & 1; nz >>= 1; PutByte(addr, nz)
#define LSR_ZP       nz = dGetByte(addr); c = nz & 1; nz >>= 1; dPutByte(addr, nz)
#define ROR          RMW_GetByte(nz, addr); nz += c << 8; c = nz & 1; nz >>= 1; PutByte(addr, nz)
#define ROR_ZP       nz = dGetByte(addr) + (c << 8); c = nz & 1; nz >>= 1; dPutByte(addr, nz)
#define DEC          RMW_GetByte(nz, addr); nz = (nz - 1) & 0xff; PutByte(addr, nz)
#define DEC_ZP       nz = dGetByte(addr); nz = (nz - 1) & 0xff; dPutByte(addr, nz)
#define INC          RMW_GetByte(nz, addr); nz = (nz + 1) & 0xff; PutByte(addr, nz)
#define INC_ZP       nz = dGetByte(addr); nz = (nz + 1) & 0xff; dPutByte(addr, nz)

#define ASO          ASL; nz = a |= nz
#define ASO_ZP       ASL_ZP; nz = a |= nz
#define RLA          ROL; nz = a &= nz
#define RLA_ZP       ROL_ZP; nz = a &= nz
#define LSE          LSR; nz = a ^= nz
#define LSE_ZP       LSR_ZP; nz = a ^= nz
#define RRA          ROR; data = nz; DO_ADC
#define RRA_ZP       ROR_ZP; data = nz; DO_ADC
#define DCM          DEC; c = (a >= nz) ? 1 : 0; nz = (a - nz) & 0xff
#define DCM_ZP       DEC_ZP; c = (a >= nz) ? 1 : 0; nz = (a - nz) & 0xff
#define INS          INC; data = nz; DO_SBC
#define INS_ZP       INC_ZP; data = nz; DO_SBC

#define BRANCH(cond) \
	if (cond) { \
		addr = SBYTE(PEEK); \
		pc++; \
		addr += pc; \
		if ((addr ^ pc) >> 8 != 0) \
			ast _ cycle++; \
		ast _ cycle++; \
		pc = addr; \
		break; \
	} \
	pc++; \
	break

#define CHECK_IRQ \
	if ((vdi & I_FLAG) == 0 && ast _ irqst != 0xff) { \
		PHPC; \
		PHPB0; \
		vdi |= I_FLAG; \
		pc = dGetWord(0xfffe); \
		ast _ cycle += 7; \
	}

/* Runs 6502 emulation for the specified number of Atari scanlines.
   Each scanline is 114 cycles of which 9 is taken by ANTIC for memory refresh. */
FUNC(void, Cpu_RunScanlines, (P(ASAP_State PTR, ast), P(int, scanlines)))
{
	/* copy registers from ASAP_State to local variables for improved performance */
	V(int, pc);
	V(int, nz);
	V(int, a);
	V(int, x);
	V(int, y);
	V(int, c);
	V(int, s);
	V(int, vdi);
	V(int, next_event_cycle);
	V(int, cycle_limit);
	pc = ast _ cpu_pc;
	nz = ast _ cpu_nz;
	a = ast _ cpu_a;
	x = ast _ cpu_x;
	y = ast _ cpu_y;
	c = ast _ cpu_c;
	s = ast _ cpu_s;
	vdi = ast _ cpu_vdi;
	ast _ next_scanline_cycle = 114;
	next_event_cycle = 114;
	cycle_limit = 114 * scanlines;
	if (next_event_cycle > ast _ timer1_cycle)
		next_event_cycle = ast _ timer1_cycle;
	if (next_event_cycle > ast _ timer2_cycle)
		next_event_cycle = ast _ timer2_cycle;
	if (next_event_cycle > ast _ timer4_cycle)
		next_event_cycle = ast _ timer4_cycle;
	ast _ nearest_event_cycle = next_event_cycle;
	for (;;) {
		V(int, cycle);
		V(int, addr);
		V(int, data);
		cycle = ast _ cycle;
		if (cycle >= ast _ nearest_event_cycle) {
			if (cycle >= ast _ next_scanline_cycle) {
				if (++ast _ scanline_number == 312)
					ast _ scanline_number = 0;
				ast _ cycle = cycle += 9;
				ast _ next_scanline_cycle += 114;
				if (--scanlines <= 0)
					break;
			}
			next_event_cycle = ast _ next_scanline_cycle;
#define CHECK_TIMER_IRQ(ch) \
			if (cycle >= ast _ timer##ch##_cycle) { \
				ast _ irqst &= ~ch; \
				ast _ timer##ch##_cycle = NEVER; \
			} \
			else if (next_event_cycle > ast _ timer##ch##_cycle) \
				next_event_cycle = ast _ timer##ch##_cycle;
			CHECK_TIMER_IRQ(1);
			CHECK_TIMER_IRQ(2);
			CHECK_TIMER_IRQ(4);
			ast _ nearest_event_cycle = next_event_cycle;
			CHECK_IRQ;
		}
#ifdef ASAPSCAN
		if (cpu_trace != 0)
			trace_cpu(ast, pc, a, x, y, s, nz, vdi, c);
#endif
		data = FETCH;
		ast _ cycle += opcode_cycles[data];
		switch (data) {
		case 0x00: /* BRK */
			pc++;
			PHPC;
			PHPB1;
			vdi |= I_FLAG;
			pc = dGetWord(0xfffe);
			break;
		case 0x01: /* ORA (ab,x) */
			INDIRECT_X;
			ORA;
			break;
		case 0x02: /* CIM [unofficial] */
		case 0x12:
		case 0x22:
		case 0x32:
		case 0x42:
		case 0x52:
		case 0x62:
		case 0x72:
		case 0x92:
		case 0xb2:
		case 0xd2:
		case 0xf2:
			ast _ scanline_number = (ast _ scanline_number + scanlines - 1) % 312;
			scanlines = 1;
			ast _ cycle = cycle_limit;
			break;
#ifndef ACPU_NO_UNOFFICIAL
		case 0x03: /* ASO (ab,x) [unofficial] */
			INDIRECT_X;
			ASO;
			break;
		case 0x04: /* NOP ab [unofficial] */
		case 0x44:
		case 0x64:
		case 0x14: /* NOP ab,x [unofficial] */
		case 0x34:
		case 0x54:
		case 0x74:
		case 0xd4:
		case 0xf4:
		case 0x80: /* NOP #ab [unofficial] */
		case 0x82:
		case 0x89:
		case 0xc2:
		case 0xe2:
			pc++;
			break;
		case 0x07: /* ASO ab [unofficial] */
			ZPAGE;
			ASO_ZP;
			break;
		case 0x0b: /* ANC #ab [unofficial] */
		case 0x2b:
			nz = a &= FETCH;
			c = nz >> 7;
			break;
		case 0x0c: /* NOP abcd [unofficial] */
			pc += 2;
			break;
		case 0x0f: /* ASO abcd [unofficial] */
			ABSOLUTE;
			ASO;
			break;
		case 0x13: /* ASO (ab),y [unofficial] */
			INDIRECT_Y;
			ASO;
			break;
		case 0x17: /* ASO ab,x [unofficial] */
			ZPAGE_X;
			ASO_ZP;
			break;
		case 0x1b: /* ASO abcd,y [unofficial] */
			ABSOLUTE_Y;
			ASO;
			break;
		case 0x1c: /* NOP abcd,x [unofficial] */
		case 0x3c:
		case 0x5c:
		case 0x7c:
		case 0xdc:
		case 0xfc:
			if (FETCH + x >= 0x100)
				ast _ cycle++;
			pc++;
			break;
		case 0x1f: /* ASO abcd,x [unofficial] */
			ABSOLUTE_X;
			ASO;
			break;
		case 0x23: /* RLA (ab,x) [unofficial] */
			INDIRECT_X;
			RLA;
			break;
		case 0x27: /* RLA ab [unofficial] */
			ZPAGE;
			RLA_ZP;
			break;
		case 0x2f: /* RLA abcd [unofficial] */
			ABSOLUTE;
			RLA;
			break;
		case 0x33: /* RLA (ab),y [unofficial] */
			INDIRECT_Y;
			RLA;
			break;
		case 0x37: /* RLA ab,x [unofficial] */
			ZPAGE_X;
			RLA_ZP;
			break;
		case 0x3b: /* RLA abcd,y [unofficial] */
			ABSOLUTE_Y;
			RLA;
			break;
		case 0x3f: /* RLA abcd,x [unofficial] */
			ABSOLUTE_X;
			RLA;
			break;
		case 0x43: /* LSE (ab,x) [unofficial] */
			INDIRECT_X;
			LSE;
			break;
		case 0x47: /* LSE ab [unofficial] */
			ZPAGE;
			LSE_ZP;
			break;
		case 0x4b: /* ALR #ab [unofficial] */
			a &= FETCH;
			c = a & 1;
			nz = a >>= 1;
			break;
		case 0x4f: /* LSE abcd [unofficial] */
			ABSOLUTE;
			LSE;
			break;
		case 0x53: /* LSE (ab),y [unofficial] */
			INDIRECT_Y;
			LSE;
			break;
		case 0x57: /* LSE ab,x [unofficial] */
			ZPAGE_X;
			LSE_ZP;
			break;
		case 0x5b: /* LSE abcd,y [unofficial] */
			ABSOLUTE_Y;
			LSE;
			break;
		case 0x5f: /* LSE abcd,x [unofficial] */
			ABSOLUTE_X;
			LSE;
			break;
		case 0x63: /* RRA (ab,x) [unofficial] */
			INDIRECT_X;
			RRA;
			break;
		case 0x67: /* RRA ab [unofficial] */
			ZPAGE;
			RRA_ZP;
			break;
		case 0x6b: /* ARR #ab [unofficial] */
			data = a & FETCH;
			nz = a = (data >> 1) + (c << 7);
			vdi = (vdi & (D_FLAG | I_FLAG)) + ((a ^ data) & V_FLAG);
#ifdef ACPU_NO_DECIMAL
			c = data >> 7;
#else
			if ((vdi & D_FLAG) == 0)
				c = data >> 7;
			else {
				if ((data & 0xf) >= 5)
					a = (a & 0xf0) + ((a + 6) & 0xf);
				if (data >= 0x50) {
					a = (a + 0x60) & 0xff;
					c = 1;
				}
				else
					c = 0;
			}
#endif
			break;
		case 0x6f: /* RRA abcd [unofficial] */
			ABSOLUTE;
			RRA;
			break;
		case 0x73: /* RRA (ab),y [unofficial] */
			INDIRECT_Y;
			RRA;
			break;
		case 0x77: /* RRA ab,x [unofficial] */
			ZPAGE_X;
			RRA_ZP;
			break;
		case 0x7b: /* RRA abcd,y [unofficial] */
			ABSOLUTE_Y;
			RRA;
			break;
		case 0x7f: /* RRA abcd,x [unofficial] */
			ABSOLUTE_X;
			RRA;
			break;
		case 0x83: /* SAX (ab,x) [unofficial] */
			INDIRECT_X;
			SAX;
			break;
		case 0x87: /* SAX ab [unofficial] */
			ZPAGE;
			SAX_ZP;
			break;
		case 0x8b: /* ANE #ab [unofficial] */
			data = FETCH;
			a &= x;
			nz = a & data;
			a &= data | 0xef;
			break;
		case 0x8f: /* SAX abcd [unofficial] */
			ABSOLUTE;
			SAX;
			break;
		case 0x93: /* SHA (ab),y [unofficial, unstable] */
			ZPAGE;
			data = zGetByte(addr + 1);
			addr = (dGetByte(addr) + (data << 8) + y) & 0xffff;
			data = a & x & (data + 1);
			PutByte(addr, data);
			break;
		case 0x97: /* SAX ab,y [unofficial] */
			ZPAGE_Y;
			SAX_ZP;
			break;
		case 0x9b: /* SHS abcd,y [unofficial, unstable] */
			/* S seems to be stable, only memory values vary */
			addr = FETCH;
			data = FETCH;
			addr = (addr + (data << 8) + y) & 0xffff;
			s = a & x;
			data = s & (data + 1);
			PutByte(addr, data);
			break;
		case 0x9c: /* SHY abcd,x [unofficial] */
			addr = FETCH;
			data = FETCH;
			addr = (addr + (data << 8) + x) & 0xffff;
			data = y & (data + 1);
			PutByte(addr, data);
			break;
		case 0x9e: /* SHX abcd,y [unofficial] */
			addr = FETCH;
			data = FETCH;
			addr = (addr + (data << 8) + y) & 0xffff;
			data = x & (data + 1);
			PutByte(addr, data);
			break;
		case 0x9f: /* SHA abcd,y [unofficial, unstable] */
			addr = FETCH;
			data = FETCH;
			addr = (addr + (data << 8) + y) & 0xffff;
			data = a & x & (data + 1);
			PutByte(addr, data);
			break;
		case 0xa3: /* LAX (ab,x) [unofficial] */
			INDIRECT_X;
			LAX;
			break;
		case 0xa7: /* LAX ab [unofficial] */
			ZPAGE;
			LAX_ZP;
			break;
		case 0xab: /* ANX #ab [unofficial] */
			nz = x = a &= FETCH;
			break;
		case 0xaf: /* LAX abcd [unofficial] */
			ABSOLUTE;
			LAX;
			break;
		case 0xb3: /* LAX (ab),y [unofficial] */
			INDIRECT_Y;
			NCYCLES_Y;
			LAX;
			break;
		case 0xb7: /* LAX ab,y [unofficial] */
			ZPAGE_Y;
			LAX_ZP;
			break;
		case 0xbb: /* LAS abcd,y [unofficial] */
			ABSOLUTE_Y;
			NCYCLES_Y;
			nz = x = a = s &= GetByte(addr);
			break;
		case 0xbf: /* LAX abcd,y [unofficial] */
			ABSOLUTE_Y;
			NCYCLES_Y;
			LAX;
			break;
		case 0xc3: /* DCM (ab,x) [unofficial] */
			INDIRECT_X;
			DCM;
			break;
		case 0xc7: /* DCM ab [unofficial] */
			ZPAGE;
			DCM_ZP;
			break;
		case 0xcb: /* SBX #ab [unofficial] */
			nz = FETCH;
			x &= a;
			c = (x >= nz) ? 1 : 0;
			nz = x = (x - nz) & 0xff;
			break;
		case 0xcf: /* DCM abcd [unofficial] */
			ABSOLUTE;
			DCM;
			break;
		case 0xd3: /* DCM (ab),y [unofficial] */
			INDIRECT_Y;
			DCM;
			break;
		case 0xd7: /* DCM ab,x [unofficial] */
			ZPAGE_X;
			DCM_ZP;
			break;
		case 0xdb: /* DCM abcd,y [unofficial] */
			ABSOLUTE_Y;
			DCM;
			break;
		case 0xdf: /* DCM abcd,x [unofficial] */
			ABSOLUTE_X;
			DCM;
			break;
		case 0xe3: /* INS (ab,x) [unofficial] */
			INDIRECT_X;
			INS;
			break;
		case 0xe7: /* INS ab [unofficial] */
			ZPAGE;
			INS_ZP;
			break;
		case 0xef: /* INS abcd [unofficial] */
			ABSOLUTE;
			INS;
			break;
		case 0xf3: /* INS (ab),y [unofficial] */
			INDIRECT_Y;
			INS;
			break;
		case 0xf7: /* INS ab,x [unofficial] */
			ZPAGE_X;
			INS_ZP;
			break;
		case 0xfb: /* INS abcd,y [unofficial] */
			ABSOLUTE_Y;
			INS;
			break;
		case 0xff: /* INS abcd,x [unofficial] */
			ABSOLUTE_X;
			INS;
			break;
#endif /* ACPU_NO_UNOFFICIAL */
		case 0x05: /* ORA ab */
			ZPAGE;
			ORA_ZP;
			break;
		case 0x06: /* ASL ab */
			ZPAGE;
			ASL_ZP;
			break;
		case 0x08: /* PHP */
			PHPB1;
			break;
		case 0x09: /* ORA #ab */
			nz = a |= FETCH;
			break;
		case 0x0a: /* ASL */
			c = a >> 7;
			nz = a = (a << 1) & 0xff;
			break;
		case 0x0d: /* ORA abcd */
			ABSOLUTE;
			ORA;
			break;
		case 0x0e: /* ASL abcd */
			ABSOLUTE;
			ASL;
			break;
		case 0x10: /* BPL */
			BRANCH(nz < 0x80);
		case 0x11: /* ORA (ab),y */
			INDIRECT_Y;
			NCYCLES_Y;
			ORA;
			break;
		case 0x15: /* ORA ab,x */
			ZPAGE_X;
			ORA_ZP;
			break;
		case 0x16: /* ASL ab,x */
			ZPAGE_X;
			ASL_ZP;
			break;
		case 0x18: /* CLC */
			c = 0;
			break;
		case 0x19: /* ORA abcd,y */
			ABSOLUTE_Y;
			NCYCLES_Y;
			ORA;
			break;
		case 0x1d: /* ORA abcd,x */
			ABSOLUTE_X;
			NCYCLES_X;
			ORA;
			break;
		case 0x1e: /* ASL abcd,x */
			ABSOLUTE_X;
			ASL;
			break;
		case 0x20: /* JSR abcd */
			addr = FETCH;
			PHPC;
			pc = addr + (PEEK << 8);
			break;
		case 0x21: /* AND (ab,x) */
			INDIRECT_X;
			AND;
			break;
		case 0x24: /* BIT ab */
			ZPAGE;
			nz = dGetByte(addr);
			vdi = (vdi & (D_FLAG | I_FLAG)) + (nz & V_FLAG);
			nz = ((nz & 0x80) << 1) + (nz & a);
			break;
		case 0x25: /* AND ab */
			ZPAGE;
			AND_ZP;
			break;
		case 0x26: /* ROL ab */
			ZPAGE;
			ROL_ZP;
			break;
		case 0x28: /* PLP */
			PLP;
			CHECK_IRQ;
			break;
		case 0x29: /* AND #ab */
			nz = a &= FETCH;
			break;
		case 0x2a: /* ROL */
			a = (a << 1) + c;
			c = a >> 8;
			nz = a &= 0xff;
			break;
		case 0x2c: /* BIT abcd */
			ABSOLUTE;
			nz = GetByte(addr);
			vdi = (vdi & (D_FLAG | I_FLAG)) + (nz & V_FLAG);
			nz = ((nz & 0x80) << 1) + (nz & a);
			break;
		case 0x2d: /* AND abcd */
			ABSOLUTE;
			AND;
			break;
		case 0x2e: /* ROL abcd */
			ABSOLUTE;
			ROL;
			break;
		case 0x30: /* BMI */
			BRANCH(nz >= 0x80);
		case 0x31: /* AND (ab),y */
			INDIRECT_Y;
			NCYCLES_Y;
			AND;
			break;
		case 0x35: /* AND ab,x */
			ZPAGE_X;
			AND_ZP;
			break;
		case 0x36: /* ROL ab,x */
			ZPAGE_X;
			ROL_ZP;
			break;
		case 0x38: /* SEC */
			c = 1;
			break;
		case 0x39: /* AND abcd,y */
			ABSOLUTE_Y;
			NCYCLES_Y;
			AND;
			break;
		case 0x3d: /* AND abcd,x */
			ABSOLUTE_X;
			NCYCLES_X;
			AND;
			break;
		case 0x3e: /* ROL abcd,x */
			ABSOLUTE_X;
			ROL;
			break;
		case 0x40: /* RTI */
			PLP;
			PL(pc);
			PL(addr);
			pc += addr << 8;
			CHECK_IRQ;
			break;
		case 0x41: /* EOR (ab,x) */
			INDIRECT_X;
			EOR;
			break;
		case 0x45: /* EOR ab */
			ZPAGE;
			EOR_ZP;
			break;
		case 0x46: /* LSR ab */
			ZPAGE;
			LSR_ZP;
			break;
		case 0x48: /* PHA */
			PH(a);
			break;
		case 0x49: /* EOR #ab */
			nz = a ^= FETCH;
			break;
		case 0x4a: /* LSR */
			c = a & 1;
			nz = a >>= 1;
			break;
		case 0x4c: /* JMP abcd */
			addr = FETCH;
			pc = addr + (PEEK << 8);
			break;
		case 0x4d: /* EOR abcd */
			ABSOLUTE;
			EOR;
			break;
		case 0x4e: /* LSR abcd */
			ABSOLUTE;
			LSR;
			break;
		case 0x50: /* BVC */
			BRANCH((vdi & V_FLAG) == 0);
		case 0x51: /* EOR (ab),y */
			INDIRECT_Y;
			NCYCLES_Y;
			EOR;
			break;
		case 0x55: /* EOR ab,x */
			ZPAGE_X;
			EOR_ZP;
			break;
		case 0x56: /* LSR ab,x */
			ZPAGE_X;
			LSR_ZP;
			break;
		case 0x58: /* CLI */
			vdi &= V_FLAG | D_FLAG;
			CHECK_IRQ;
			break;
		case 0x59: /* EOR abcd,y */
			ABSOLUTE_Y;
			NCYCLES_Y;
			EOR;
			break;
		case 0x5d: /* EOR abcd,x */
			ABSOLUTE_X;
			NCYCLES_X;
			EOR;
			break;
		case 0x5e: /* LSR abcd,x */
			ABSOLUTE_X;
			LSR;
			break;
		case 0x60: /* RTS */
			PL(pc);
			PL(addr);
			pc += (addr << 8) + 1;
			break;
		case 0x61: /* ADC (ab,x) */
			INDIRECT_X;
			ADC;
			break;
		case 0x65: /* ADC ab */
			ZPAGE;
			ADC_ZP;
			break;
		case 0x66: /* ROR ab */
			ZPAGE;
			ROR_ZP;
			break;
		case 0x68: /* PLA */
			PL(a);
			nz = a;
			break;
		case 0x69: /* ADC #ab */
			data = FETCH;
			DO_ADC;
			break;
		case 0x6a: /* ROR */
			nz = (c << 7) + (a >> 1);
			c = a & 1;
			a = nz;
			break;
		case 0x6c: /* JMP (abcd) */
			ABSOLUTE;
			if ((addr & 0xff) == 0xff)
				pc = (dGetByte(addr - 0xff) << 8) + dGetByte(addr);
			else
				pc = dGetWord(addr);
			break;
		case 0x6d: /* ADC abcd */
			ABSOLUTE;
			ADC;
			break;
		case 0x6e: /* ROR abcd */
			ABSOLUTE;
			ROR;
			break;
		case 0x70: /* BVS */
			BRANCH((vdi & V_FLAG) != 0);
		case 0x71: /* ADC (ab),y */
			INDIRECT_Y;
			NCYCLES_Y;
			ADC;
			break;
		case 0x75: /* ADC ab,x */
			ZPAGE_X;
			ADC_ZP;
			break;
		case 0x76: /* ROR ab,x */
			ZPAGE_X;
			ROR_ZP;
			break;
		case 0x78: /* SEI */
			vdi |= I_FLAG;
			break;
		case 0x79: /* ADC abcd,y */
			ABSOLUTE_Y;
			NCYCLES_Y;
			ADC;
			break;
		case 0x7d: /* ADC abcd,x */
			ABSOLUTE_X;
			NCYCLES_X;
			ADC;
			break;
		case 0x7e: /* ROR abcd,x */
			ABSOLUTE_X;
			ROR;
			break;
		case 0x81: /* STA (ab,x) */
			INDIRECT_X;
			STA;
			break;
		case 0x84: /* STY ab */
			ZPAGE;
			STY_ZP;
			break;
		case 0x85: /* STA ab */
			ZPAGE;
			STA_ZP;
			break;
		case 0x86: /* STX ab */
			ZPAGE;
			STX_ZP;
			break;
		case 0x88: /* DEY */
			nz = y = (y - 1) & 0xff;
			break;
		case 0x8a: /* TXA */
			nz = a = x;
			break;
		case 0x8c: /* STY abcd */
			ABSOLUTE;
			STY;
			break;
		case 0x8d: /* STA abcd */
			ABSOLUTE;
			STA;
			break;
		case 0x8e: /* STX abcd */
			ABSOLUTE;
			STX;
			break;
		case 0x90: /* BCC */
			BRANCH(c == 0);
		case 0x91: /* STA (ab),y */
			INDIRECT_Y;
			STA;
			break;
		case 0x94: /* STY ab,x */
			ZPAGE_X;
			STY_ZP;
			break;
		case 0x95: /* STA ab,x */
			ZPAGE_X;
			STA_ZP;
			break;
		case 0x96: /* STX ab,y */
			ZPAGE_Y;
			STX_ZP;
			break;
		case 0x98: /* TYA */
			nz = a = y;
			break;
		case 0x99: /* STA abcd,y */
			ABSOLUTE_Y;
			STA;
			break;
		case 0x9a: /* TXS */
			s = x;
			break;
		case 0x9d: /* STA abcd,x */
			ABSOLUTE_X;
			STA;
			break;
		case 0xa0: /* LDY #ab */
			nz = y = FETCH;
			break;
		case 0xa1: /* LDA (ab,x) */
			INDIRECT_X;
			LDA;
			break;
		case 0xa2: /* LDX #ab */
			nz = x = FETCH;
			break;
		case 0xa4: /* LDY ab */
			ZPAGE;
			LDY_ZP;
			break;
		case 0xa5: /* LDA ab */
			ZPAGE;
			LDA_ZP;
			break;
		case 0xa6: /* LDX ab */
			ZPAGE;
			LDX_ZP;
			break;
		case 0xa8: /* TAY */
			nz = y = a;
			break;
		case 0xa9: /* LDA #ab */
			nz = a = FETCH;
			break;
		case 0xaa: /* TAX */
			nz = x = a;
			break;
		case 0xac: /* LDY abcd */
			ABSOLUTE;
			LDY;
			break;
		case 0xad: /* LDA abcd */
			ABSOLUTE;
			LDA;
			break;
		case 0xae: /* LDX abcd */
			ABSOLUTE;
			LDX;
			break;
		case 0xb0: /* BCS */
			BRANCH(c != 0);
		case 0xb1: /* LDA (ab),y */
			INDIRECT_Y;
			NCYCLES_Y;
			LDA;
			break;
		case 0xb4: /* LDY ab,x */
			ZPAGE_X;
			LDY_ZP;
			break;
		case 0xb5: /* LDA ab,x */
			ZPAGE_X;
			LDA_ZP;
			break;
		case 0xb6: /* LDX ab,y */
			ZPAGE_Y;
			LDX_ZP;
			break;
		case 0xb8: /* CLV */
			vdi &= D_FLAG | I_FLAG;
			break;
		case 0xb9: /* LDA abcd,y */
			ABSOLUTE_Y;
			NCYCLES_Y;
			LDA;
			break;
		case 0xba: /* TSX */
			nz = x = s;
			break;
		case 0xbc: /* LDY abcd,x */
			ABSOLUTE_X;
			NCYCLES_X;
			LDY;
			break;
		case 0xbd: /* LDA abcd,x */
			ABSOLUTE_X;
			NCYCLES_X;
			LDA;
			break;
		case 0xbe: /* LDX abcd,y */
			ABSOLUTE_Y;
			NCYCLES_Y;
			LDX;
			break;
		case 0xc0: /* CPY #ab */
			nz = FETCH;
			c = (y >= nz) ? 1 : 0;
			nz = (y - nz) & 0xff;
			break;
		case 0xc1: /* CMP (ab,x) */
			INDIRECT_X;
			CMP;
			break;
		case 0xc4: /* CPY ab */
			ZPAGE;
			CPY_ZP;
			break;
		case 0xc5: /* CMP ab */
			ZPAGE;
			CMP_ZP;
			break;
		case 0xc6: /* DEC ab */
			ZPAGE;
			DEC_ZP;
			break;
		case 0xc8: /* INY */
			nz = y = (y + 1) & 0xff;
			break;
		case 0xc9: /* CMP #ab */
			nz = FETCH;
			c = (a >= nz) ? 1 : 0;
			nz = (a - nz) & 0xff;
			break;
		case 0xca: /* DEX */
			nz = x = (x - 1) & 0xff;
			break;
		case 0xcc: /* CPY abcd */
			ABSOLUTE;
			CPY;
			break;
		case 0xcd: /* CMP abcd */
			ABSOLUTE;
			CMP;
			break;
		case 0xce: /* DEC abcd */
			ABSOLUTE;
			DEC;
			break;
		case 0xd0: /* BNE */
			BRANCH((nz & 0xff) != 0);
		case 0xd1: /* CMP (ab),y */
			INDIRECT_Y;
			NCYCLES_Y;
			CMP;
			break;
		case 0xd5: /* CMP ab,x */
			ZPAGE_X;
			CMP_ZP;
			break;
		case 0xd6: /* DEC ab,x */
			ZPAGE_X;
			DEC_ZP;
			break;
		case 0xd8: /* CLD */
			vdi &= V_FLAG | I_FLAG;
			break;
		case 0xd9: /* CMP abcd,y */
			ABSOLUTE_Y;
			NCYCLES_Y;
			CMP;
			break;
		case 0xdd: /* CMP abcd,x */
			ABSOLUTE_X;
			NCYCLES_X;
			CMP;
			break;
		case 0xde: /* DEC abcd,x */
			ABSOLUTE_X;
			DEC;
			break;
		case 0xe0: /* CPX #ab */
			nz = FETCH;
			c = (x >= nz) ? 1 : 0;
			nz = (x - nz) & 0xff;
			break;
		case 0xe1: /* SBC (ab,x) */
			INDIRECT_X;
			SBC;
			break;
		case 0xe4: /* CPX ab */
			ZPAGE;
			CPX_ZP;
			break;
		case 0xe5: /* SBC ab */
			ZPAGE;
			SBC_ZP;
			break;
		case 0xe6: /* INC ab */
			ZPAGE;
			INC_ZP;
			break;
		case 0xe8: /* INX */
			nz = x = (x + 1) & 0xff;
			break;
		case 0xe9: /* SBC #ab */
		case 0xeb: /* SBC #ab [unofficial] */
			data = FETCH;
			DO_SBC;
			break;
		case 0xea: /* NOP */
		case 0x1a: /* NOP [unofficial] */
		case 0x3a:
		case 0x5a:
		case 0x7a:
		case 0xda:
		case 0xfa:
			break;
		case 0xec: /* CPX abcd */
			ABSOLUTE;
			CPX;
			break;
		case 0xed: /* SBC abcd */
			ABSOLUTE;
			SBC;
			break;
		case 0xee: /* INC abcd */
			ABSOLUTE;
			INC;
			break;
		case 0xf0: /* BEQ */
			BRANCH((nz & 0xff) == 0);
		case 0xf1: /* SBC (ab),y */
			INDIRECT_Y;
			NCYCLES_Y;
			SBC;
			break;
		case 0xf5: /* SBC ab,x */
			ZPAGE_X;
			SBC_ZP;
			break;
		case 0xf6: /* INC ab,x */
			ZPAGE_X;
			INC_ZP;
			break;
		case 0xf8: /* SED */
			vdi |= D_FLAG;
			break;
		case 0xf9: /* SBC abcd,y */
			ABSOLUTE_Y;
			NCYCLES_Y;
			SBC;
			break;
		case 0xfd: /* SBC abcd,x */
			ABSOLUTE_X;
			NCYCLES_X;
			SBC;
			break;
		case 0xfe: /* INC abcd,x */
			ABSOLUTE_X;
			INC;
			break;
		}
	}
	ast _ cpu_pc = pc;
	ast _ cpu_nz = nz;
	ast _ cpu_a = a;
	ast _ cpu_x = x;
	ast _ cpu_y = y;
	ast _ cpu_c = c;
	ast _ cpu_s = s;
	ast _ cpu_vdi = vdi;
	ast _ cycle -= cycle_limit;
	if (ast _ timer1_cycle != NEVER)
		ast _ timer1_cycle -= cycle_limit;
	if (ast _ timer2_cycle != NEVER)
		ast _ timer2_cycle -= cycle_limit;
	if (ast _ timer4_cycle != NEVER)
		ast _ timer4_cycle -= cycle_limit;
}
