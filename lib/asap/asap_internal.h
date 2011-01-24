/*
 * asap_internal.h - private interface of the ASAP engine
 *
 * Copyright (C) 2005-2009  Piotr Fusik
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

#ifndef _ASAP_INTERNAL_H_
#define _ASAP_INTERNAL_H_

#include "anylang.h"

#ifndef C

#define ASAP_SONGS_MAX          32
#define ASAP_SAMPLE_RATE        44100

#endif

#ifdef JAVA

#define ASAP_FORMAT_U8          8
#define ASAP_FORMAT_S16_LE      16
#define ASAP_FORMAT_S16_BE      -16
#define ASAP_SampleFormat       int
#define ASAP_ParseDuration      parseDuration

#elif defined(CSHARP) || defined(JAVASCRIPT)

#define ASAP_FORMAT_U8          ASAP_SampleFormat.U8
#define ASAP_FORMAT_S16_LE      ASAP_SampleFormat.S16LE
#define ASAP_FORMAT_S16_BE      ASAP_SampleFormat.S16BE
#ifdef CSHARP
#define ASAP_ParseDuration      ParseDuration
#endif

#elif defined(ACTIONSCRIPT)

#define ASAP_ParseDuration      parseDuration

#else /* C */

#include "asap.h"

int ASAP_GetByte(ASAP_State *ast, int addr);
void ASAP_PutByte(ASAP_State *ast, int addr, int data);

void Cpu_RunScanlines(ASAP_State *ast, int scanlines);

void PokeySound_Initialize(ASAP_State *ast);
void PokeySound_StartFrame(ASAP_State *ast);
void PokeySound_PutByte(ASAP_State *ast, int addr, int data);
int PokeySound_GetRandom(ASAP_State *ast, int addr, int cycle);
void PokeySound_EndFrame(ASAP_State *ast, int cycle_limit);
int PokeySound_Generate(ASAP_State *ast, byte buffer[], int buffer_offset, int blocks, ASAP_SampleFormat format);
abool PokeySound_IsSilent(const PokeyState *pst);
void PokeySound_Mute(const ASAP_State *ast, PokeyState *pst, int mask);

#ifdef ASAPSCAN
abool call_6502_player(ASAP_State *ast);
extern int cpu_trace;
void trace_cpu(const ASAP_State *ast, int pc, int a, int x, int y, int s, int nz, int vdi, int c);
#endif

#endif /* C */

#define ASAP_MAIN_CLOCK         1773447

#define V_FLAG                  0x40
#define D_FLAG                  0x08
#define I_FLAG                  0x04
#define Z_FLAG                  0x02

#define NEVER                   0x800000

#define DELTA_SHIFT_POKEY       20
#define DELTA_SHIFT_GTIA        20
#define DELTA_SHIFT_COVOX       17

/* 6502 player types */
#define ASAP_TYPE_SAP_B         1
#define ASAP_TYPE_SAP_C         2
#define ASAP_TYPE_SAP_D         3
#define ASAP_TYPE_SAP_S         4
#define ASAP_TYPE_CMC           5
#define ASAP_TYPE_CM3           6
#define ASAP_TYPE_CMR           7
#define ASAP_TYPE_CMS           8
#define ASAP_TYPE_DLT           9
#define ASAP_TYPE_MPT           10
#define ASAP_TYPE_RMT           11
#define ASAP_TYPE_TMC           12
#define ASAP_TYPE_TM2           13

#define dGetByte(addr)          UBYTE(ast _ memory[addr])
#define dPutByte(addr, data)    ast _ memory[addr] = CAST(byte) (data)
#define dGetWord(addr)          (dGetByte(addr) + (dGetByte((addr) + 1) << 8))
#define GetByte(addr)           (((addr) & 0xf900) == 0xd000 ? ASAP_GetByte(ast, addr) : dGetByte(addr))
#define PutByte(addr, data)     do { if (((addr) & 0xf900) == 0xd000) ASAP_PutByte(ast, addr, data); else dPutByte(addr, data); } while (FALSE)
#define RMW_GetByte(dest, addr) do { if (((addr) >> 8) == 0xd2) { dest = ASAP_GetByte(ast, addr); ast _ cycle--; ASAP_PutByte(ast, addr, dest); ast _ cycle++; } else dest = dGetByte(addr); } while (FALSE)

#define CYCLE_TO_SAMPLE(cycle)  TO_INT(((cycle) * ASAP_SAMPLE_RATE + ast _ sample_offset) / ASAP_MAIN_CLOCK)

#endif /* _ASAP_INTERNAL_H_ */
