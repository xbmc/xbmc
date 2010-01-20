/*
** Nofrendo (c) 1998-2000 Matthew Conte (matt@conte.com)
**
**
** This program is free software; you can redistribute it and/or
** modify it under the terms of version 2 of the GNU Library General 
** Public License as published by the Free Software Foundation.
**
** This program is distributed in the hope that it will be useful, 
** but WITHOUT ANY WARRANTY; without even the implied warranty of
** MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU 
** Library General Public License for more details.  To obtain a 
** copy of the GNU Library General Public License, write to the Free 
** Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
**
** Any permitted reproduction of these routines, in whole or in part,
** must bear this legend.
**
**
** nes6502.c
**
** NES custom 6502 (2A03) CPU implementation
** $Id: nes6502.c,v 1.6 2000/07/04 04:50:07 matt Exp $
*/


#include "../../types.h"
#include "nes6502.h"
#include "dis6502.h"
#include <stdio.h>


#define  ADD_CYCLES(x)     instruction_cycles += (x)
#define  INC_CYCLES()      instruction_cycles++
//#define  ADD_CYCLES(x)  remaining_cycles -= (x)
//#define  INC_CYCLES()   remaining_cycles--

/*
** Check to see if an index reg addition overflowed to next page
*/
#define CHECK_INDEX_OVERFLOW(addr, reg) \
{ \
   if ((uint8) (addr) < (reg)) \
      INC_CYCLES(); \
}

/*
** Addressing mode macros
*/

#define NO_READ(value)        /* empty */

#define IMMEDIATE_BYTE(value) \
{ \
   value = bank_readbyte(PC++); \
}


#define ABSOLUTE_ADDR(address) \
{ \
   address = bank_readaddress(PC); \
   PC += 2; \
}

#define ABSOLUTE(address, value) \
{ \
   ABSOLUTE_ADDR(address); \
   value = mem_read(address); \
}

#define ABSOLUTE_BYTE(value) \
{ \
   ABSOLUTE(temp, value); \
}

#define ABS_IND_X_ADDR(address) \
{ \
   address = (bank_readaddress(PC) + X) & 0xFFFF; \
   PC += 2; \
   CHECK_INDEX_OVERFLOW(address, X); \
}

#define ABS_IND_X(address, value) \
{ \
   ABS_IND_X_ADDR(address); \
   value = mem_read(address); \
}

#define ABS_IND_X_BYTE(value) \
{ \
   ABS_IND_X(temp, value); \
}

#define ABS_IND_Y_ADDR(address) \
{ \
   address = (bank_readaddress(PC) + Y) & 0xFFFF; \
   PC += 2; \
   CHECK_INDEX_OVERFLOW(address, Y); \
}

#define ABS_IND_Y(address, value) \
{ \
   ABS_IND_Y_ADDR(address); \
   value = mem_read(address); \
}

#define ABS_IND_Y_BYTE(value) \
{ \
   ABS_IND_Y(temp, value); \
}

#define ZERO_PAGE_ADDR(address) \
{ \
   IMMEDIATE_BYTE(address); \
}

#define ZERO_PAGE(address, value) \
{ \
   ZERO_PAGE_ADDR(address); \
   value = ZP_READ(address); \
}

#define ZERO_PAGE_BYTE(value) \
{ \
   ZERO_PAGE(btemp, value); \
}

/* Zero-page indexed Y doesn't really exist, just for LDX / STX */
#define ZP_IND_X_ADDR(address) \
{ \
   address = bank_readbyte(PC++) + X; \
}

#define ZP_IND_X(bAddr, value) \
{ \
   ZP_IND_X_ADDR(bAddr); \
   value = ZP_READ(bAddr); \
}

#define ZP_IND_X_BYTE(value) \
{ \
   ZP_IND_X(btemp, value); \
}

#define ZP_IND_Y_ADDR(address) \
{ \
   address = bank_readbyte(PC++) + Y; \
}

#define ZP_IND_Y(address, value) \
{ \
   ZP_IND_Y_ADDR(address); \
   value = ZP_READ(address); \
}

#define ZP_IND_Y_BYTE(value) \
{ \
   ZP_IND_Y(btemp, value); \
}  

/*
** For conditional jump relative instructions
** (BCC, BCS, BEQ, BMI, BNE, BPL, BVC, BVS)
*/
#define RELATIVE_JUMP(cond) \
{ \
   if (cond) \
   { \
      IMMEDIATE_BYTE(btemp); \
      if (((int8) btemp + (uint8) PC) & 0xFF00) \
         ADD_CYCLES(4); \
      else \
         ADD_CYCLES(3); \
      PC += ((int8) btemp); \
   } \
   else \
   { \
      PC++; \
      ADD_CYCLES(2); \
   } \
}

/*
** This is actually indexed indirect, but I call it
** indirect X to avoid confusion
*/
#define INDIR_X_ADDR(address) \
{ \
   btemp = bank_readbyte(PC++) + X; \
   address = zp_address(btemp); \
}

#define INDIR_X(address, value) \
{ \
   INDIR_X_ADDR(address); \
   value = mem_read(address); \
} 

#define INDIR_X_BYTE(value) \
{ \
   INDIR_X(temp, value); \
}

/*
** This is actually indirect indexed, but I call it
** indirect y to avoid confusion
*/
#define INDIR_Y_ADDR(address) \
{ \
   IMMEDIATE_BYTE(btemp); \
   address = (zp_address(btemp) + Y) & 0xFFFF; \
   /* ???? */ \
   CHECK_INDEX_OVERFLOW(address, Y); \
}

#define INDIR_Y(address, value) \
{ \
   INDIR_Y_ADDR(address); \
   value = mem_read(address); \
} 

#define INDIR_Y_BYTE(value) \
{ \
   /*IMMEDIATE_BYTE(btemp); \
   temp = zp_address(btemp) + Y; \
   CHECK_INDEX_OVERFLOW(temp, Y); \
   value = mem_read(temp);*/ \
   INDIR_Y(temp, value); \
}


#define  JUMP(address)  PC = bank_readaddress((address))

/*
** Interrupt macros
*/
#define NMI() \
{ \
   PUSH(PC >> 8); \
   PUSH(PC & 0xFF); \
   CLEAR_FLAG(B_FLAG); \
   PUSH(P); \
   SET_FLAG(I_FLAG); \
   JUMP(NMI_VECTOR); \
   int_pending &= ~NMI_MASK; \
   ADD_CYCLES(INT_CYCLES); \
}

#define IRQ() \
{ \
   PUSH(PC >> 8); \
   PUSH(PC & 0xFF); \
   CLEAR_FLAG(B_FLAG); \
   PUSH(P); \
   SET_FLAG(I_FLAG); \
   JUMP(IRQ_VECTOR); \
   int_pending &= ~IRQ_MASK; \
   ADD_CYCLES(INT_CYCLES); \
}

/*
** Instruction macros
*/

/* Warning! NES CPU has no decimal mode, so by default this does no BCD! */
#ifdef NES6502_DECIMAL
#define ADC(cycles, read_func) \
{ \
   read_func(data); \
   if (P & D_FLAG) \
   { \
      temp = (A & 0x0F) + (data & 0x0F) + (P & C_FLAG); \
      if (temp >= 10) \
         temp = (temp - 10) | 0x10; \
      temp += (A & 0xF0) + (data & 0xF0); \
      TEST_AND_FLAG(0 == ((A + data + (P & C_FLAG)) & 0xFF), Z_FLAG); \
      TEST_AND_FLAG(temp & 0x80, N_FLAG); \
      TEST_AND_FLAG(((~(A ^ data)) & (A ^ temp) & 0x80), V_FLAG); \
      if (temp > 0x9F) \
         temp += 0x60; \
      TEST_AND_FLAG(temp > 0xFF, C_FLAG); \
      A = (uint8) temp; \
   } \
   else \
   { \
      temp = A + data + (P & C_FLAG); \
      /* Set C on carry */ \
      TEST_AND_FLAG(temp > 0xFF, C_FLAG); \
      /* Set V on overflow */ \
      TEST_AND_FLAG(((~(A ^ data)) & (A ^ temp) & 0x80), V_FLAG); \
      A = (uint8) temp; \
      SET_NZ_FLAGS(A); \
   }\
   ADD_CYCLES(cycles); \
}
#else
#define ADC(cycles, read_func) \
{ \
   read_func(data); \
   temp = A + data + (P & C_FLAG); \
   /* Set C on carry */ \
   TEST_AND_FLAG(temp > 0xFF, C_FLAG); \
   /* Set V on overflow */ \
   TEST_AND_FLAG(((~(A ^ data)) & (A ^ temp) & 0x80), V_FLAG); \
   A = (uint8) temp; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}
#endif /* NES6502_DECIMAL */

/* undocumented */
#define ANC(cycles, read_func) \
{ \
   read_func(data); \
   A &= data; \
   SET_NZ_FLAGS(A); \
   TEST_AND_FLAG(P & N_FLAG, C_FLAG); \
   ADD_CYCLES(cycles); \
}

#define AND(cycles, read_func) \
{ \
   read_func(data); \
   A &= data; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define ANE(cycles, read_func) \
{ \
   read_func(data); \
   A = (A | 0xEE) & X & data; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#ifdef NES6502_DECIMAL
#define ARR(cycles, read_func) \
{ \
   read_func(data); \
   data &= A; \
   if (P & D_FLAG) \
   { \
      temp = (data >> 1) | ((P & C_FLAG) << 7); \
      SET_NZ_FLAGS(temp); \
      TEST_AND_FLAG((temp ^ data) & 0x40, V_FLAG); \
      if (((data & 0x0F) + (data & 0x01)) > 5) \
         temp = (temp & 0xF0) | ((temp + 0x6) & 0x0F); \
      if (((data & 0xF0) + (data & 0x10)) > 0x50) \
      { \
         temp = (temp & 0x0F) | ((temp + 0x60) & 0xF0); \
         SET_FLAG(C_FLAG); \
      } \
      else \
         CLEAR_FLAG(C_FLAG); \
      A = (uint8) temp; \
   } \
   else \
   { \
      A = (data >> 1) | ((P & C_FLAG) << 7); \
      SET_NZ_FLAGS(A); \
      TEST_AND_FLAG(A & 0x40, C_FLAG); \
      TEST_AND_FLAG(((A >> 6) ^ (A >> 5)) & 1, V_FLAG); \
   }\
   ADD_CYCLES(cycles); \
}
#else
#define ARR(cycles, read_func) \
{ \
   read_func(data); \
   data &= A; \
   A = (data >> 1) | ((P & C_FLAG) << 7); \
   SET_NZ_FLAGS(A); \
   TEST_AND_FLAG(A & 0x40, C_FLAG); \
   TEST_AND_FLAG((A >> 6) ^ (A >> 5), V_FLAG); \
   ADD_CYCLES(cycles); \
}
#endif /* NES6502_DECIMAL */

#define ASL(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   TEST_AND_FLAG(data & 0x80, C_FLAG); \
   data <<= 1; \
   write_func(addr, data); \
   SET_NZ_FLAGS(data); \
   ADD_CYCLES(cycles); \
}

#define ASL_A() \
{ \
   TEST_AND_FLAG(A & 0x80, C_FLAG); \
   A <<= 1; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(2); \
}

/* undocumented */
#define ASR(cycles, read_func) \
{ \
   read_func(data); \
   data &= A; \
   TEST_AND_FLAG(data & 0x01, C_FLAG); \
   A = data >> 1; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

#define BCC() \
{ \
   RELATIVE_JUMP((IS_FLAG_CLEAR(C_FLAG))); \
}

#define BCS() \
{ \
   RELATIVE_JUMP((IS_FLAG_SET(C_FLAG))); \
}

#define BEQ() \
{ \
   RELATIVE_JUMP((IS_FLAG_SET(Z_FLAG))); \
}

#define BIT(cycles, read_func) \
{ \
   read_func(data); \
   TEST_AND_FLAG(0 == (data & A), Z_FLAG);\
   CLEAR_FLAG(N_FLAG | V_FLAG); \
   /* move bit 7/6 of data into N/V flags */ \
   SET_FLAG(data & (N_FLAG | V_FLAG)); \
   ADD_CYCLES(cycles); \
}

#define BMI() \
{ \
   RELATIVE_JUMP((IS_FLAG_SET(N_FLAG))); \
}

#define BNE() \
{ \
   RELATIVE_JUMP((IS_FLAG_CLEAR(Z_FLAG))); \
}

#define BPL() \
{ \
   RELATIVE_JUMP((IS_FLAG_CLEAR(N_FLAG))); \
}

/* Software interrupt type thang */
#define BRK() \
{ \
   PC++; \
   PUSH(PC >> 8); \
   PUSH(PC & 0xFF); \
   SET_FLAG(B_FLAG); \
   PUSH(P); \
   SET_FLAG(I_FLAG); \
   JUMP(IRQ_VECTOR); \
   ADD_CYCLES(7); \
}

#define BVC() \
{ \
   RELATIVE_JUMP((IS_FLAG_CLEAR(V_FLAG))); \
}

#define BVS() \
{ \
   RELATIVE_JUMP((IS_FLAG_SET(V_FLAG))); \
}

#define CLC() \
{ \
   CLEAR_FLAG(C_FLAG); \
   ADD_CYCLES(2); \
}

#define CLD() \
{ \
   CLEAR_FLAG(D_FLAG); \
   ADD_CYCLES(2); \
}

#define CLI() \
{ \
   CLEAR_FLAG(I_FLAG); \
   ADD_CYCLES(2); \
}

#define CLV() \
{ \
   CLEAR_FLAG(V_FLAG); \
   ADD_CYCLES(2); \
}

/* TODO: ick! */
#define _COMPARE(reg, value) \
{ \
   temp = (reg) - (value); \
   /* C is clear when data > A */ \
   TEST_AND_FLAG(0 == (temp & 0x8000), C_FLAG); \
   SET_NZ_FLAGS((uint8) temp); /* handles Z flag */ \
}

#define CMP(cycles, read_func) \
{ \
   read_func(data); \
   _COMPARE(A, data); \
   ADD_CYCLES(cycles); \
}

#define CPX(cycles, read_func) \
{ \
   read_func(data); \
   _COMPARE(X, data); \
   ADD_CYCLES(cycles); \
}

#define CPY(cycles, read_func) \
{ \
   read_func(data); \
   _COMPARE(Y, data); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define DCP(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   data--; \
   write_func(addr, data); \
   CMP(cycles, NO_READ); \
}

#define DEC(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   data--; \
   write_func(addr, data); \
   SET_NZ_FLAGS(data); \
   ADD_CYCLES(cycles); \
}

#define DEX() \
{ \
   X--; \
   SET_NZ_FLAGS(X); \
   ADD_CYCLES(2); \
}

#define DEY() \
{ \
   Y--; \
   SET_NZ_FLAGS(Y); \
   ADD_CYCLES(2); \
}

/* undocumented (double-NOP) */
#define DOP(cycles) \
{ \
   PC++; \
   ADD_CYCLES(cycles); \
}

#define EOR(cycles, read_func) \
{ \
   read_func(data); \
   A ^= data; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

#define INC(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   data++; \
   write_func(addr, data); \
   SET_NZ_FLAGS(data); \
   ADD_CYCLES(cycles); \
}

#define INX() \
{ \
   X++; \
   SET_NZ_FLAGS(X); \
   ADD_CYCLES(2); \
}

#define INY() \
{ \
   Y++; \
   SET_NZ_FLAGS(Y); \
   ADD_CYCLES(2); \
}

/* undocumented */
#define ISB(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   data++; \
   write_func(addr, data); \
   SBC(cycles, NO_READ); \
}

#ifdef NES6502_TESTOPS
#define JAM() \
{ \
   cpu_Jam(); \
}
#elif defined(NSF_PLAYER)
#define JAM() \
{ \
}
#else
#define JAM() \
{ \
   char jambuf[20]; \
   sprintf(jambuf, "JAM: PC=$%04X", PC); \
   ASSERT_MSG(jambuf); \
   ADD_CYCLES(2); \
}
#endif /* NES6502_TESTOPS */

#define JMP_INDIRECT() \
{ \
   temp = bank_readaddress(PC); \
   /* bug in crossing page boundaries */ \
   if (0xFF == (uint8) temp) \
      PC = (bank_readbyte(temp & ~0xFF) << 8) | bank_readbyte(temp); \
   else \
      JUMP(temp); \
   ADD_CYCLES(5); \
}

#define JMP_ABSOLUTE() \
{ \
   JUMP(PC); \
   ADD_CYCLES(3); \
}

#define JSR() \
{ \
   PC++; \
   PUSH(PC >> 8); \
   PUSH(PC & 0xFF); \
   JUMP(PC - 1); \
   ADD_CYCLES(6); \
}

/* undocumented */
#define LAS(cycles, read_func) \
{ \
   read_func(data); \
   A = X = S = (S & data); \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define LAX(cycles, read_func) \
{ \
   read_func(A); \
   X = A; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

#define LDA(cycles, read_func) \
{ \
   read_func(A); \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

#define LDX(cycles, read_func) \
{ \
   read_func(X); \
   SET_NZ_FLAGS(X);\
   ADD_CYCLES(cycles); \
}

#define LDY(cycles, read_func) \
{ \
   read_func(Y); \
   SET_NZ_FLAGS(Y);\
   ADD_CYCLES(cycles); \
}

#define LSR(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   TEST_AND_FLAG(data & 0x01, C_FLAG); \
   data >>= 1; \
   write_func(addr, data); \
   SET_NZ_FLAGS(data); \
   ADD_CYCLES(cycles); \
}

#define LSR_A() \
{ \
   TEST_AND_FLAG(A & 0x01, C_FLAG); \
   A >>= 1; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(2); \
}

/* undocumented */
#define LXA(cycles, read_func) \
{ \
   read_func(data); \
   A = X = ((A | 0xEE) & data); \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

#define NOP() \
{ \
   ADD_CYCLES(2); \
}

#define ORA(cycles, read_func) \
{ \
   read_func(data); \
   A |= data; \
   SET_NZ_FLAGS(A);\
   ADD_CYCLES(cycles); \
}

#define PHA() \
{ \
   PUSH(A); \
   ADD_CYCLES(3); \
}

#define PHP() \
{ \
   /* B flag is pushed on stack as well */ \
   PUSH((P | B_FLAG)); \
   ADD_CYCLES(3); \
}

#define PLA() \
{ \
   A = PULL(); \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(4); \
}

#define PLP() \
{ \
   P = PULL(); \
   SET_FLAG(R_FLAG); /* ensure reserved flag is set */ \
   ADD_CYCLES(4); \
}

/* undocumented */
#define RLA(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   if (P & C_FLAG) \
   { \
      TEST_AND_FLAG(data & 0x80, C_FLAG); \
      data = (data << 1) | 1; \
   } \
   else \
   { \
      TEST_AND_FLAG(data & 0x80, C_FLAG); \
      data <<= 1; \
   } \
   write_func(addr, data); \
   A &= data; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

/* 9-bit rotation (carry flag used for rollover) */
#define ROL(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   if (P & C_FLAG) \
   { \
      TEST_AND_FLAG(data & 0x80, C_FLAG); \
      data = (data << 1) | 1; \
   } \
   else \
   { \
      TEST_AND_FLAG(data & 0x80, C_FLAG); \
      data <<= 1; \
   } \
   write_func(addr, data); \
   SET_NZ_FLAGS(data); \
   ADD_CYCLES(cycles); \
}

#define ROL_A() \
{ \
   if (P & C_FLAG) \
   { \
      TEST_AND_FLAG(A & 0x80, C_FLAG); \
      A = (A << 1) | 1; \
   } \
   else \
   { \
      TEST_AND_FLAG(A & 0x80, C_FLAG); \
      A <<= 1; \
   } \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(2); \
}

#define ROR(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   if (P & C_FLAG) \
   { \
      TEST_AND_FLAG(data & 1, C_FLAG); \
      data = (data >> 1) | 0x80; \
   } \
   else \
   { \
      TEST_AND_FLAG(data & 1, C_FLAG); \
      data >>= 1; \
   } \
   write_func(addr, data); \
   SET_NZ_FLAGS(data); \
   ADD_CYCLES(cycles); \
}

#define ROR_A() \
{ \
   if (P & C_FLAG) \
   { \
      TEST_AND_FLAG(A & 1, C_FLAG); \
      A = (A >> 1) | 0x80; \
   } \
   else \
   { \
      TEST_AND_FLAG(A & 1, C_FLAG); \
      A >>= 1; \
   } \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(2); \
}

/* undocumented */
#define RRA(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   if (P & C_FLAG) \
   { \
      TEST_AND_FLAG(data & 1, C_FLAG); \
      data = (data >> 1) | 0x80; \
   } \
   else \
   { \
      TEST_AND_FLAG(data & 1, C_FLAG); \
      data >>= 1; \
   } \
   write_func(addr, data); \
   ADC(cycles, NO_READ); \
}

#define RTI() \
{ \
   P = PULL(); \
   SET_FLAG(R_FLAG); /* ensure reserved flag is set */ \
   PC = PULL(); \
   PC |= PULL() << 8; \
   ADD_CYCLES(6); \
}

#define RTS() \
{ \
   PC = PULL(); \
   PC = (PC | (PULL() << 8)) + 1; \
   ADD_CYCLES(6); \
}

/* undocumented */
#define SAX(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   data = A & X; \
   write_func(addr, data); \
   ADD_CYCLES(cycles); \
}

/* Warning! NES CPU has no decimal mode, so by default this does no BCD! */
#ifdef NES6502_DECIMAL
#define SBC(cycles, read_func) \
{ \
   read_func(data); \
   /* NOT(C) is considered borrow */ \
   temp = A - data - ((P & C_FLAG) ^ C_FLAG); \
   if (P & D_FLAG) \
   { \
      uint8 al, ah; \
      al = (A & 0x0F) - (data & 0x0F) - ((P & C_FLAG) ^ C_FLAG); \
      ah = (A >> 4) - (data >> 4); \
      if (al & 0x10) \
      { \
         al -= 6; \
         ah--; \
      } \
      if (ah & 0x10) \
         ah -= 6; \
      TEST_AND_FLAG(temp < 0x100, C_FLAG); \
      TEST_AND_FLAG(((A ^ temp) & 0x80) && ((A ^ data) & 0x80), V_FLAG); \
      SET_NZ_FLAGS(temp & 0xFF); \
      A = (ah << 4) | (al & 0x0F); \
   } \
   else \
   { \
      TEST_AND_FLAG(((A ^ temp) & 0x80) && ((A ^ data) & 0x80), V_FLAG); \
      TEST_AND_FLAG(temp < 0x100, C_FLAG); \
      A = (uint8) temp; \
      SET_NZ_FLAGS(A & 0xFF); \
   } \
   ADD_CYCLES(cycles); \
}
#else
#define SBC(cycles, read_func) \
{ \
   read_func(data); \
   temp = A - data - ((P & C_FLAG) ^ C_FLAG); \
   TEST_AND_FLAG(((A ^ data) & (A ^ temp) & 0x80), V_FLAG); \
   TEST_AND_FLAG(temp < 0x100, C_FLAG); \
   A = (uint8) temp; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}
#endif /* NES6502_DECIMAL */

/* undocumented */
#define SBX(cycles, read_func) \
{ \
   read_func(data); \
   temp = (A & X) - data; \
   TEST_AND_FLAG(temp < 0x100, C_FLAG); \
   X = temp & 0xFF; \
   SET_NZ_FLAGS(X); \
   ADD_CYCLES(cycles); \
}

#define SEC() \
{ \
   SET_FLAG(C_FLAG); \
   ADD_CYCLES(2); \
}

#define SED() \
{ \
   SET_FLAG(D_FLAG); \
   ADD_CYCLES(2); \
}

#define SEI() \
{ \
   SET_FLAG(I_FLAG); \
   ADD_CYCLES(2); \
}

/* undocumented */
#define SHA(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   data = A & X & ((uint8) ((addr >> 8) + 1)); \
   write_func(addr, data); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define SHS(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   S = A & X; \
   data = S & ((uint8) ((addr >> 8) + 1)); \
   write_func(addr, data); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define SHX(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   data = X & ((uint8) ((addr >> 8) + 1)); \
   write_func(addr, data); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define SHY(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   data = Y & ((uint8) ((addr >> 8 ) + 1)); \
   write_func(addr, data); \
   ADD_CYCLES(cycles); \
}

/* undocumented */
#define SLO(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   TEST_AND_FLAG(data & 0x80, C_FLAG); \
   data <<= 1; \
   write_func(addr, data); \
   A |= data; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

/* unoffical */
#define SRE(cycles, read_func, write_func, addr) \
{ \
   read_func(addr, data); \
   TEST_AND_FLAG(data & 1, C_FLAG); \
   data >>= 1; \
   write_func(addr, data); \
   A ^= data; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(cycles); \
}

#define STA(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   write_func(addr, A); \
   ADD_CYCLES(cycles); \
}

#define STX(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   write_func(addr, X); \
   ADD_CYCLES(cycles); \
}

#define STY(cycles, read_func, write_func, addr) \
{ \
   read_func(addr); \
   write_func(addr, Y); \
   ADD_CYCLES(cycles); \
}

#define TAX() \
{ \
   X = A; \
   SET_NZ_FLAGS(X);\
   ADD_CYCLES(2); \
}

#define TAY() \
{ \
   Y = A; \
   SET_NZ_FLAGS(Y);\
   ADD_CYCLES(2); \
}

/* undocumented (triple-NOP) */
#define TOP() \
{ \
   PC += 2; \
   ADD_CYCLES(4); \
}

#define TSX() \
{ \
   X = S; \
   SET_NZ_FLAGS(X);\
   ADD_CYCLES(2); \
}

#define TXA() \
{ \
   A = X; \
   SET_NZ_FLAGS(A);\
   ADD_CYCLES(2); \
}

#define TXS() \
{ \
   S = X; \
   ADD_CYCLES(2); \
}

#define TYA() \
{ \
   A = Y; \
   SET_NZ_FLAGS(A); \
   ADD_CYCLES(2); \
}


/*
** Stack and data fetching macros
*/

/* Set/clear/test bits in the flags register */
#define  SET_FLAG(mask)          P |= (mask)
#define  CLEAR_FLAG(mask)        P &= ~(mask)
#define  IS_FLAG_SET(mask)       (P & (mask))
#define  IS_FLAG_CLEAR(mask)     (0 == IS_FLAG_SET((mask)))

#define  TEST_AND_FLAG(test, mask) \
{ \
   if ((test)) \
      SET_FLAG((mask)); \
   else \
      CLEAR_FLAG((mask)); \
}


/*
** flag register helper macros
*/

/* register push/pull */
#define  PUSH(value)             stack_page[S--] = (uint8) (value)
#define  PULL()                  stack_page[++S]

/* Sets the Z and N flags based on given data, taken from precomputed table */
#define  SET_NZ_FLAGS(value)     P &= ~(N_FLAG | Z_FLAG); \
                                 P |= flag_table[(value)]

#define  GET_GLOBAL_REGS() \
{ \
   PC = reg_PC; \
   A = reg_A; \
   X = reg_X; \
   Y = reg_Y; \
   P = reg_P; \
   S = reg_S; \
}

#define SET_LOCAL_REGS() \
{ \
   reg_PC = PC; \
   reg_A = A; \
   reg_X = X; \
   reg_Y = Y; \
   reg_P = P; \
   reg_S = S; \
}


/* static data */
static nes6502_memread *pmem_read, *pmr;
static nes6502_memwrite *pmem_write, *pmw;

/* lookup table for N/Z flags */
static uint8 flag_table[256];

/* internal critical CPU vars */
static uint32 reg_PC;
static uint8 reg_A, reg_P, reg_X, reg_Y, reg_S;
static uint8 int_pending;
static int dma_cycles;

/* execution cycle count (can be reset by user) */
static uint32 total_cycles = 0;

/* memory region pointers */
static uint8 *nes6502_banks[NES6502_NUMBANKS];
static uint8 *ram = NULL;
static uint8 *stack_page = NULL;


/*
** Zero-page helper macros
*/

#define  ZP_READ(addr)           ram[(addr)]
#define  ZP_WRITE(addr, value)   ram[(addr)] = (uint8) (value)


INLINE uint8 bank_readbyte(register uint32 address)
{
   ASSERT(nes6502_banks[address >> NES6502_BANKSHIFT]);
   return nes6502_banks[address >> NES6502_BANKSHIFT][address & NES6502_BANKMASK];
}

INLINE void bank_writebyte(register uint32 address, register uint8 value)
{
   ASSERT(nes6502_banks[address >> NES6502_BANKSHIFT]);
   nes6502_banks[address >> NES6502_BANKSHIFT][address & NES6502_BANKMASK] = value;
}

INLINE uint32 zp_address(register uint8 address)
{
#ifdef HOST_LITTLE_ENDIAN
   /* TODO: this fails if src address is $xFFF */
   /* TODO: this fails if host architecture doesn't support byte alignment */
   return (uint32) (*(uint16 *)(ram + address));
#else
#ifdef TARGET_CPU_PPC
   return __lhbrx(ram, address);
#else
   uint32 x = (uint32) *(uint16 *)(ram + address);
   return (x << 8) | (x >> 8);
#endif /* TARGET_CPU_PPC */
#endif /* HOST_LITTLE_ENDIAN */
}

INLINE uint32 bank_readaddress(register uint32 address)
{
#ifdef HOST_LITTLE_ENDIAN
   /* TODO: this fails if src address is $xFFF */
   /* TODO: this fails if host architecture doesn't support byte alignment */
   return (uint32) (*(uint16 *)(nes6502_banks[address >> NES6502_BANKSHIFT] + (address & NES6502_BANKMASK)));
#else
#ifdef TARGET_CPU_PPC
   return __lhbrx(nes6502_banks[address >> NES6502_BANKSHIFT], address & NES6502_BANKMASK);
#else
   uint32 x = (uint32) *(uint16 *)(nes6502_banks[address >> NES6502_BANKSHIFT] + (address & NES6502_BANKMASK));
   return (x << 8) | (x >> 8);
#endif /* TARGET_CPU_PPC */
#endif /* HOST_LITTLE_ENDIAN */
}

/* read a byte of 6502 memory */
static uint8 mem_read(uint32 address)
{
   /* TODO: following cases are N2A03-specific */
   /* RAM */
   if (address < 0x800)
      return ram[address];
   /* always paged memory */
//   else if (address >= 0x6000)
   else if (address >= 0x8000)
      return bank_readbyte(address);
   /* check memory range handlers */
   else
   {
      for (pmr = pmem_read; pmr->min_range != 0xFFFFFFFF; pmr++)
      {
         if ((address >= pmr->min_range) && (address <= pmr->max_range))
            return pmr->read_func(address);
      }
   }

   /* return paged memory */
   return bank_readbyte(address);
}

/* write a byte of data to 6502 memory */
static void mem_write(uint32 address, uint8 value)
{
   /* RAM */
   if (address < 0x800)
   {
      ram[address] = value;
      return;
   }
   /* check memory range handlers */
   else
   {
      for (pmw = pmem_write; pmw->min_range != 0xFFFFFFFF; pmw++)
      {
         if ((address >= pmw->min_range) && (address <= pmw->max_range))
         {
            pmw->write_func(address, value);
            return;
         }
      }
   }

   /* write to paged memory */
   bank_writebyte(address, value);
}

/* set the current context */
void nes6502_setcontext(nes6502_context *cpu)
{
   int loop;

   ASSERT(cpu);
   
   /* Set the page pointers */
   for (loop = 0; loop < NES6502_NUMBANKS; loop++)
      nes6502_banks[loop] = cpu->mem_page[loop];

   ram = nes6502_banks[0];  /* quicker zero-page/RAM references */
   stack_page = ram + STACK_OFFSET;

   pmem_read = cpu->read_handler;
   pmem_write = cpu->write_handler;

   reg_PC = cpu->pc_reg;
   reg_A = cpu->a_reg;
   reg_P = cpu->p_reg;
   reg_X = cpu->x_reg;
   reg_Y = cpu->y_reg;
   reg_S = cpu->s_reg;
   int_pending = cpu->int_pending;
   dma_cycles = cpu->dma_cycles;
}

/* get the current context */
void nes6502_getcontext(nes6502_context *cpu)
{
   int loop;

   /* Set the page pointers */
   for (loop = 0; loop < NES6502_NUMBANKS; loop++)
      cpu->mem_page[loop] = nes6502_banks[loop];

   cpu->read_handler = pmem_read;
   cpu->write_handler = pmem_write;

   cpu->pc_reg = reg_PC;
   cpu->a_reg = reg_A;
   cpu->p_reg = reg_P;
   cpu->x_reg = reg_X;
   cpu->y_reg = reg_Y;
   cpu->s_reg = reg_S;
   cpu->int_pending = int_pending;
   cpu->dma_cycles = dma_cycles;
}

/* DMA a byte of data from ROM */
uint8 nes6502_getbyte(uint32 address)
{
   return bank_readbyte(address);
}

/* get number of elapsed cycles */
uint32 nes6502_getcycles(boolean reset_flag)
{
   uint32 cycles = total_cycles;

   if (reset_flag)
      total_cycles = 0;

   return cycles;
}


/* Execute instructions until count expires
**
** Returns the number of cycles *actually* executed
** (note that this can be from 0-6 cycles more than you wanted)
*/
int nes6502_execute(int remaining_cycles)
{
   int instruction_cycles, old_cycles = total_cycles;
   uint32 temp, addr; /* for macros */
   uint32 PC;
   uint8 A, X, Y, P, S;
   uint8 opcode, data;
   uint8 btemp, baddr; /* for macros */

   GET_GLOBAL_REGS();

   /* Continue until we run out of cycles */
   while (remaining_cycles > 0)
   {
      instruction_cycles = 0;

      /* check for DMA cycle burning */
      if (dma_cycles)
      {
         if (remaining_cycles <= dma_cycles)
         {
            dma_cycles -= remaining_cycles;
            total_cycles += remaining_cycles;
            goto _execute_done;
         }
         else
         {
            remaining_cycles -= dma_cycles;
            total_cycles += dma_cycles;
            dma_cycles = 0;
         }
      }

      if (int_pending)
      {
         /* NMI has highest priority */
         if (int_pending & NMI_MASK)
         {
            NMI();
         }
         /* IRQ has lowest priority */
         else /* if (int_pending & IRQ_MASK) */
         {
            if (IS_FLAG_CLEAR(I_FLAG))
               IRQ();
         }
      }

      /* Fetch instruction */
      //nes6502_disasm(PC, P, A, X, Y, S);

      opcode = bank_readbyte(PC++);

      /* Execute instruction */
      switch (opcode)
      {
      case 0x00:  /* BRK */
         BRK();
         break;

      case 0x01:  /* ORA ($nn,X) */
         ORA(6, INDIR_X_BYTE);
         break;

      /* JAM */
      case 0x02:  /* JAM */
      case 0x12:  /* JAM */
      case 0x22:  /* JAM */
      case 0x32:  /* JAM */
      case 0x42:  /* JAM */
      case 0x52:  /* JAM */
      case 0x62:  /* JAM */
      case 0x72:  /* JAM */
      case 0x92:  /* JAM */
      case 0xB2:  /* JAM */
      case 0xD2:  /* JAM */
      case 0xF2:  /* JAM */
         JAM();
         /* kill switch for CPU emulation */
         goto _execute_done;

      case 0x03:  /* SLO ($nn,X) */
         SLO(8, INDIR_X, mem_write, addr);
         break;

      case 0x04:  /* NOP $nn */
      case 0x44:  /* NOP $nn */
      case 0x64:  /* NOP $nn */
         DOP(3);
         break;

      case 0x05:  /* ORA $nn */
         ORA(3, ZERO_PAGE_BYTE); 
         break;

      case 0x06:  /* ASL $nn */
         ASL(5, ZERO_PAGE, ZP_WRITE, baddr);
         break;

      case 0x07:  /* SLO $nn */
         SLO(5, ZERO_PAGE, ZP_WRITE, baddr);
         break;

      case 0x08:  /* PHP */
         PHP(); 
         break;

      case 0x09:  /* ORA #$nn */
         ORA(2, IMMEDIATE_BYTE);
         break;

      case 0x0A:  /* ASL A */
         ASL_A();
         break;

      case 0x0B:  /* ANC #$nn */
         ANC(2, IMMEDIATE_BYTE);
         break;

      case 0x0C:  /* NOP $nnnn */
         TOP(); 
         break;

      case 0x0D:  /* ORA $nnnn */
         ORA(4, ABSOLUTE_BYTE);
         break;

      case 0x0E:  /* ASL $nnnn */
         ASL(6, ABSOLUTE, mem_write, addr);
         break;

      case 0x0F:  /* SLO $nnnn */
         SLO(6, ABSOLUTE, mem_write, addr);
         break;

      case 0x10:  /* BPL $nnnn */
         BPL();
         break;

      case 0x11:  /* ORA ($nn),Y */
         ORA(5, INDIR_Y_BYTE);
         break;
      
      case 0x13:  /* SLO ($nn),Y */
         SLO(8, INDIR_Y, mem_write, addr);
         break;

      case 0x14:  /* NOP $nn,X */
      case 0x34:  /* NOP */
      case 0x54:  /* NOP $nn,X */
      case 0x74:  /* NOP $nn,X */
      case 0xD4:  /* NOP $nn,X */
      case 0xF4:  /* NOP ($nn,X) */
         DOP(4);
         break;

      case 0x15:  /* ORA $nn,X */
         ORA(4, ZP_IND_X_BYTE);
         break;

      case 0x16:  /* ASL $nn,X */
         ASL(6, ZP_IND_X, ZP_WRITE, baddr);
         break;

      case 0x17:  /* SLO $nn,X */
         SLO(6, ZP_IND_X, ZP_WRITE, baddr);
         break;

      case 0x18:  /* CLC */
         CLC();
         break;

      case 0x19:  /* ORA $nnnn,Y */
         ORA(4, ABS_IND_Y_BYTE);
         break;
      
      case 0x1A:  /* NOP */
      case 0x3A:  /* NOP */
      case 0x5A:  /* NOP */
      case 0x7A:  /* NOP */
      case 0xDA:  /* NOP */
      case 0xFA:  /* NOP */
         NOP();
         break;

      case 0x1B:  /* SLO $nnnn,Y */
         SLO(7, ABS_IND_Y, mem_write, addr);
         break;

      case 0x1C:  /* NOP $nnnn,X */
      case 0x3C:  /* NOP $nnnn,X */
      case 0x5C:  /* NOP $nnnn,X */
      case 0x7C:  /* NOP $nnnn,X */
      case 0xDC:  /* NOP $nnnn,X */
      case 0xFC:  /* NOP $nnnn,X */
         TOP();
         break;

      case 0x1D:  /* ORA $nnnn,X */
         ORA(4, ABS_IND_X_BYTE);
         break;

      case 0x1E:  /* ASL $nnnn,X */
         ASL(7, ABS_IND_X, mem_write, addr);
         break;

      case 0x1F:  /* SLO $nnnn,X */
         SLO(7, ABS_IND_X, mem_write, addr);
         break;
      
      case 0x20:  /* JSR $nnnn */
         JSR();
         break;

      case 0x21:  /* AND ($nn,X) */
         AND(6, INDIR_X_BYTE);
         break;

      case 0x23:  /* RLA ($nn,X) */
         RLA(8, INDIR_X, mem_write, addr);
         break;

      case 0x24:  /* BIT $nn */
         BIT(3, ZERO_PAGE_BYTE);
         break;

      case 0x25:  /* AND $nn */
         AND(3, ZERO_PAGE_BYTE);
         break;

      case 0x26:  /* ROL $nn */
         ROL(5, ZERO_PAGE, ZP_WRITE, baddr);
         break;

      case 0x27:  /* RLA $nn */
         RLA(5, ZERO_PAGE, ZP_WRITE, baddr);
         break;

      case 0x28:  /* PLP */
         PLP();
         break;

      case 0x29:  /* AND #$nn */
         AND(2, IMMEDIATE_BYTE);
         break;

      case 0x2A:  /* ROL A */
         ROL_A();
         break;

      case 0x2B:  /* ANC #$nn */
         ANC(2, IMMEDIATE_BYTE);
         break;

      case 0x2C:  /* BIT $nnnn */
         BIT(4, ABSOLUTE_BYTE);
         break;

      case 0x2D:  /* AND $nnnn */
         AND(4, ABSOLUTE_BYTE);
         break;

      case 0x2E:  /* ROL $nnnn */
         ROL(6, ABSOLUTE, mem_write, addr);
         break;

      case 0x2F:  /* RLA $nnnn */
         RLA(6, ABSOLUTE, mem_write, addr);
         break;

      case 0x30:  /* BMI $nnnn */
         BMI();
         break;

      case 0x31:  /* AND ($nn),Y */
         AND(5, INDIR_Y_BYTE);
         break;

      case 0x33:  /* RLA ($nn),Y */
         RLA(8, INDIR_Y, mem_write, addr);
         break;

      case 0x35:  /* AND $nn,X */
         AND(4, ZP_IND_X_BYTE);
         break;

      case 0x36:  /* ROL $nn,X */
         ROL(6, ZP_IND_X, ZP_WRITE, baddr);
         break;

      case 0x37:  /* RLA $nn,X */
         RLA(6, ZP_IND_X, ZP_WRITE, baddr);
         break;

      case 0x38:  /* SEC */
         SEC();
         break;

      case 0x39:  /* AND $nnnn,Y */
         AND(4, ABS_IND_Y_BYTE);
         break;

      case 0x3B:  /* RLA $nnnn,Y */
         RLA(7, ABS_IND_Y, mem_write, addr);
         break;

      case 0x3D:  /* AND $nnnn,X */
         AND(4, ABS_IND_X_BYTE);
         break;

      case 0x3E:  /* ROL $nnnn,X */
         ROL(7, ABS_IND_X, mem_write, addr);
         break;

      case 0x3F:  /* RLA $nnnn,X */
         RLA(7, ABS_IND_X, mem_write, addr);
         break;

      case 0x40:  /* RTI */
         RTI();
         break;

      case 0x41:  /* EOR ($nn,X) */
         EOR(6, INDIR_X_BYTE);
         break;

      case 0x43:  /* SRE ($nn,X) */
         SRE(8, INDIR_X, mem_write, addr);
         break;

      case 0x45:  /* EOR $nn */
         EOR(3, ZERO_PAGE_BYTE);
         break;

      case 0x46:  /* LSR $nn */
         LSR(5, ZERO_PAGE, ZP_WRITE, baddr);
         break;

      case 0x47:  /* SRE $nn */
         SRE(5, ZERO_PAGE, ZP_WRITE, baddr);
         break;

      case 0x48:  /* PHA */
         PHA();
         break;

      case 0x49:  /* EOR #$nn */
         EOR(2, IMMEDIATE_BYTE);
         break;

      case 0x4A:  /* LSR A */
         LSR_A();
         break;

      case 0x4B:  /* ASR #$nn */
         ASR(2, IMMEDIATE_BYTE);
         break;

      case 0x4C:  /* JMP $nnnn */
         JMP_ABSOLUTE();
         break;

      case 0x4D:  /* EOR $nnnn */
         EOR(4, ABSOLUTE_BYTE);
         break;

      case 0x4E:  /* LSR $nnnn */
         LSR(6, ABSOLUTE, mem_write, addr);
         break;

      case 0x4F:  /* SRE $nnnn */
         SRE(6, ABSOLUTE, mem_write, addr);
         break;

      case 0x50:  /* BVC $nnnn */
         BVC();
         break;

      case 0x51:  /* EOR ($nn),Y */
         EOR(5, INDIR_Y_BYTE);
         break;

      case 0x53:  /* SRE ($nn),Y */
         SRE(8, INDIR_Y, mem_write, addr);
         break;

      case 0x55:  /* EOR $nn,X */
         EOR(4, ZP_IND_X_BYTE);
         break;

      case 0x56:  /* LSR $nn,X */
         LSR(6, ZP_IND_X, ZP_WRITE, baddr);
         break;

      case 0x57:  /* SRE $nn,X */
         SRE(6, ZP_IND_X, ZP_WRITE, baddr);
         break;

      case 0x58:  /* CLI */
         CLI();
         break;

      case 0x59:  /* EOR $nnnn,Y */
         EOR(4, ABS_IND_Y_BYTE);
         break;

      case 0x5B:  /* SRE $nnnn,Y */
         SRE(7, ABS_IND_Y, mem_write, addr);
         break;

      case 0x5D:  /* EOR $nnnn,X */
         EOR(4, ABS_IND_X_BYTE);
         break;

      case 0x5E:  /* LSR $nnnn,X */
         LSR(7, ABS_IND_X, mem_write, addr);
         break;

      case 0x5F:  /* SRE $nnnn,X */
         SRE(7, ABS_IND_X, mem_write, addr);
         break;

      case 0x60:  /* RTS */
         RTS();
         break;

      case 0x61:  /* ADC ($nn,X) */
         ADC(6, INDIR_X_BYTE);
         break;

      case 0x63:  /* RRA ($nn,X) */
         RRA(8, INDIR_X, mem_write, addr);
         break;

      case 0x65:  /* ADC $nn */
         ADC(3, ZERO_PAGE_BYTE);
         break;

      case 0x66:  /* ROR $nn */
         ROR(5, ZERO_PAGE, ZP_WRITE, baddr);
         break;

      case 0x67:  /* RRA $nn */
         RRA(5, ZERO_PAGE, ZP_WRITE, baddr);
         break;

      case 0x68:  /* PLA */
         PLA();
         break;

      case 0x69:  /* ADC #$nn */
         ADC(2, IMMEDIATE_BYTE);
         break;

      case 0x6A:  /* ROR A */
         ROR_A();
         break;

      case 0x6B:  /* ARR #$nn */
         ARR(2, IMMEDIATE_BYTE);
         break;

      case 0x6C:  /* JMP ($nnnn) */
         JMP_INDIRECT();
         break;

      case 0x6D:  /* ADC $nnnn */
         ADC(4, ABSOLUTE_BYTE);
         break;

      case 0x6E:  /* ROR $nnnn */
         ROR(6, ABSOLUTE, mem_write, addr);
         break;

      case 0x6F:  /* RRA $nnnn */
         RRA(6, ABSOLUTE, mem_write, addr);
         break;

      case 0x70:  /* BVS $nnnn */
         BVS();
         break;

      case 0x71:  /* ADC ($nn),Y */
         ADC(5, INDIR_Y_BYTE);
         break;

      case 0x73:  /* RRA ($nn),Y */
         RRA(8, INDIR_Y, mem_write, addr);
         break;

      case 0x75:  /* ADC $nn,X */
         ADC(4, ZP_IND_X_BYTE);
         break;

      case 0x76:  /* ROR $nn,X */
         ROR(6, ZP_IND_X, ZP_WRITE, baddr);
         break;

      case 0x77:  /* RRA $nn,X */
         RRA(6, ZP_IND_X, ZP_WRITE, baddr);
         break;

      case 0x78:  /* SEI */
         SEI();
         break;

      case 0x79:  /* ADC $nnnn,Y */
         ADC(4, ABS_IND_Y_BYTE);
         break;

      case 0x7B:  /* RRA $nnnn,Y */
         RRA(7, ABS_IND_Y, mem_write, addr);
         break;

      case 0x7D:  /* ADC $nnnn,X */
         ADC(4, ABS_IND_X_BYTE);
         break;

      case 0x7E:  /* ROR $nnnn,X */
         ROR(7, ABS_IND_X, mem_write, addr);
         break;

      case 0x7F:  /* RRA $nnnn,X */
         RRA(7, ABS_IND_X, mem_write, addr);
         break;

      case 0x80:  /* NOP #$nn */
      case 0x82:  /* NOP #$nn */
      case 0x89:  /* NOP #$nn */
      case 0xC2:  /* NOP #$nn */
      case 0xE2:  /* NOP #$nn */
         DOP(2);
         break;

      case 0x81:  /* STA ($nn,X) */
         STA(6, INDIR_X_ADDR, mem_write, addr);
         break;

      case 0x83:  /* SAX ($nn,X) */
         SAX(6, INDIR_X_ADDR, mem_write, addr);
         break;

      case 0x84:  /* STY $nn */
         STY(3, ZERO_PAGE_ADDR, ZP_WRITE, baddr);
         break;

      case 0x85:  /* STA $nn */
         STA(3, ZERO_PAGE_ADDR, ZP_WRITE, baddr);
         break;

      case 0x86:  /* STX $nn */
         STX(3, ZERO_PAGE_ADDR, ZP_WRITE, baddr);
         break;

      case 0x87:  /* SAX $nn */
         SAX(3, ZERO_PAGE_ADDR, ZP_WRITE, baddr);
         break;

      case 0x88:  /* DEY */
         DEY();
         break;

      case 0x8A:  /* TXA */
         TXA();
         break;

      case 0x8B:  /* ANE #$nn */
         ANE(2, IMMEDIATE_BYTE);
         break;

      case 0x8C:  /* STY $nnnn */
         STY(4, ABSOLUTE_ADDR, mem_write, addr);
         break;

      case 0x8D:  /* STA $nnnn */
         STA(4, ABSOLUTE_ADDR, mem_write, addr);
         break;

      case 0x8E:  /* STX $nnnn */
         STX(4, ABSOLUTE_ADDR, mem_write, addr);
         break;
      
      case 0x8F:  /* SAX $nnnn */
         SAX(4, ABSOLUTE_ADDR, mem_write, addr);
         break;

      case 0x90:  /* BCC $nnnn */
         BCC();
         break;

      case 0x91:  /* STA ($nn),Y */
         STA(6, INDIR_Y_ADDR, mem_write, addr);
         break;

      case 0x93:  /* SHA ($nn),Y */
         SHA(6, INDIR_Y_ADDR, mem_write, addr);
         break;

      case 0x94:  /* STY $nn,X */
         STY(4, ZP_IND_X_ADDR, ZP_WRITE, baddr);
         break;

      case 0x95:  /* STA $nn,X */
         STA(4, ZP_IND_X_ADDR, ZP_WRITE, baddr);
         break;

      case 0x96:  /* STX $nn,Y */
         STX(4, ZP_IND_Y_ADDR, ZP_WRITE, baddr);
         break;

      case 0x97:  /* SAX $nn,Y */
         SAX(4, ZP_IND_Y_ADDR, ZP_WRITE, baddr);
         break;

      case 0x98:  /* TYA */
         TYA();
         break;

      case 0x99:  /* STA $nnnn,Y */
         STA(5, ABS_IND_Y_ADDR, mem_write, addr);
         break;

      case 0x9A:  /* TXS */
         TXS();
         break;

      case 0x9B:  /* SHS $nnnn,Y */
         SHS(5, ABS_IND_Y_ADDR, mem_write, addr);
         break;

      case 0x9C:  /* SHY $nnnn,X */
         SHY(5, ABS_IND_X_ADDR, mem_write, addr);
         break;

      case 0x9D:  /* STA $nnnn,X */
         STA(5, ABS_IND_X_ADDR, mem_write, addr);
         break;

      case 0x9E:  /* SHX $nnnn,Y */
         SHX(5, ABS_IND_Y_ADDR, mem_write, addr);
         break;

      case 0x9F:  /* SHA $nnnn,Y */
         SHA(5, ABS_IND_Y_ADDR, mem_write, addr);
         break;
      
      case 0xA0:  /* LDY #$nn */
         LDY(2, IMMEDIATE_BYTE);
         break;

      case 0xA1:  /* LDA ($nn,X) */
         LDA(6, INDIR_X_BYTE);
         break;

      case 0xA2:  /* LDX #$nn */
         LDX(2, IMMEDIATE_BYTE);
         break;

      case 0xA3:  /* LAX ($nn,X) */
         LAX(6, INDIR_X_BYTE);
         break;

      case 0xA4:  /* LDY $nn */
         LDY(3, ZERO_PAGE_BYTE);
         break;

      case 0xA5:  /* LDA $nn */
         LDA(3, ZERO_PAGE_BYTE);
         break;

      case 0xA6:  /* LDX $nn */
         LDX(3, ZERO_PAGE_BYTE);
         break;

      case 0xA7:  /* LAX $nn */
         LAX(3, ZERO_PAGE_BYTE);
         break;

      case 0xA8:  /* TAY */
         TAY();
         break;

      case 0xA9:  /* LDA #$nn */
         LDA(2, IMMEDIATE_BYTE);
         break;

      case 0xAA:  /* TAX */
         TAX();
         break;

      case 0xAB:  /* LXA #$nn */
         LXA(2, IMMEDIATE_BYTE);
         break;

      case 0xAC:  /* LDY $nnnn */
         LDY(4, ABSOLUTE_BYTE);
         break;

      case 0xAD:  /* LDA $nnnn */
         LDA(4, ABSOLUTE_BYTE);
         break;
      
      case 0xAE:  /* LDX $nnnn */
         LDX(4, ABSOLUTE_BYTE);
         break;

      case 0xAF:  /* LAX $nnnn */
         LAX(4, ABSOLUTE_BYTE);
         break;

      case 0xB0:  /* BCS $nnnn */
         BCS();
         break;

      case 0xB1:  /* LDA ($nn),Y */
         LDA(5, INDIR_Y_BYTE);
         break;

      case 0xB3:  /* LAX ($nn),Y */
         LAX(5, INDIR_Y_BYTE);
         break;

      case 0xB4:  /* LDY $nn,X */
         LDY(4, ZP_IND_X_BYTE);
         break;

      case 0xB5:  /* LDA $nn,X */
         LDA(4, ZP_IND_X_BYTE);
         break;

      case 0xB6:  /* LDX $nn,Y */
         LDX(4, ZP_IND_Y_BYTE);
         break;

      case 0xB7:  /* LAX $nn,Y */
         LAX(4, ZP_IND_Y_BYTE);
         break;

      case 0xB8:  /* CLV */
         CLV();
         break;

      case 0xB9:  /* LDA $nnnn,Y */
         LDA(4, ABS_IND_Y_BYTE);
         break;

      case 0xBA:  /* TSX */
         TSX();
         break;

      case 0xBB:  /* LAS $nnnn,Y */
         LAS(4, ABS_IND_Y_BYTE);
         break;

      case 0xBC:  /* LDY $nnnn,X */
         LDY(4, ABS_IND_X_BYTE);
         break;

      case 0xBD:  /* LDA $nnnn,X */
         LDA(4, ABS_IND_X_BYTE);
         break;

      case 0xBE:  /* LDX $nnnn,Y */
         LDX(4, ABS_IND_Y_BYTE);
         break;

      case 0xBF:  /* LAX $nnnn,Y */
         LAX(4, ABS_IND_Y_BYTE);
         break;

      case 0xC0:  /* CPY #$nn */
         CPY(2, IMMEDIATE_BYTE);
         break;

      case 0xC1:  /* CMP ($nn,X) */
         CMP(6, INDIR_X_BYTE);
         break;

      case 0xC3:  /* DCP ($nn,X) */
         DCP(8, INDIR_X, mem_write, addr);
         break;

      case 0xC4:  /* CPY $nn */
         CPY(3, ZERO_PAGE_BYTE);
         break;

      case 0xC5:  /* CMP $nn */
         CMP(3, ZERO_PAGE_BYTE);
         break;

      case 0xC6:  /* DEC $nn */
         DEC(5, ZERO_PAGE, ZP_WRITE, baddr);
         break;

      case 0xC7:  /* DCP $nn */
         DCP(5, ZERO_PAGE, ZP_WRITE, baddr);
         break;

      case 0xC8:  /* INY */
         INY();
         break;

      case 0xC9:  /* CMP #$nn */
         CMP(2, IMMEDIATE_BYTE);
         break;

      case 0xCA:  /* DEX */
         DEX();
         break;

      case 0xCB:  /* SBX #$nn */
         SBX(2, IMMEDIATE_BYTE);
         break;

      case 0xCC:  /* CPY $nnnn */
         CPY(4, ABSOLUTE_BYTE);
         break;

      case 0xCD:  /* CMP $nnnn */
         CMP(4, ABSOLUTE_BYTE);
         break;

      case 0xCE:  /* DEC $nnnn */
         DEC(6, ABSOLUTE, mem_write, addr);
         break;

      case 0xCF:  /* DCP $nnnn */
         DCP(6, ABSOLUTE, mem_write, addr);
         break;
      
      case 0xD0:  /* BNE $nnnn */
         BNE();
         break;

      case 0xD1:  /* CMP ($nn),Y */
         CMP(5, INDIR_Y_BYTE);
         break;

      case 0xD3:  /* DCP ($nn),Y */
         DCP(8, INDIR_Y, mem_write, addr);
         break;

      case 0xD5:  /* CMP $nn,X */
         CMP(4, ZP_IND_X_BYTE);
         break;

      case 0xD6:  /* DEC $nn,X */
         DEC(6, ZP_IND_X, ZP_WRITE, baddr);
         break;

      case 0xD7:  /* DCP $nn,X */
         DCP(6, ZP_IND_X, ZP_WRITE, baddr);
         break;

      case 0xD8:  /* CLD */
         CLD();
         break;

      case 0xD9:  /* CMP $nnnn,Y */
         CMP(4, ABS_IND_Y_BYTE);
         break;

      case 0xDB:  /* DCP $nnnn,Y */
         DCP(7, ABS_IND_Y, mem_write, addr);
         break;                  

      case 0xDD:  /* CMP $nnnn,X */
         CMP(4, ABS_IND_X_BYTE);
         break;

      case 0xDE:  /* DEC $nnnn,X */
         DEC(7, ABS_IND_X, mem_write, addr);
         break;

      case 0xDF:  /* DCP $nnnn,X */
         DCP(7, ABS_IND_X, mem_write, addr);
         break;

      case 0xE0:  /* CPX #$nn */
         CPX(2, IMMEDIATE_BYTE);
         break;

      case 0xE1:  /* SBC ($nn,X) */
         SBC(6, INDIR_X_BYTE);
         break;

      case 0xE3:  /* ISB ($nn,X) */
         ISB(8, INDIR_X, mem_write, addr);
         break;

      case 0xE4:  /* CPX $nn */
         CPX(3, ZERO_PAGE_BYTE);
         break;

      case 0xE5:  /* SBC $nn */
         SBC(3, ZERO_PAGE_BYTE);
         break;

      case 0xE6:  /* INC $nn */
         INC(5, ZERO_PAGE, ZP_WRITE, baddr);
         break;

      case 0xE7:  /* ISB $nn */
         ISB(5, ZERO_PAGE, ZP_WRITE, baddr);
         break;

      case 0xE8:  /* INX */
         INX();
         break;

      case 0xE9:  /* SBC #$nn */
      case 0xEB:  /* USBC #$nn */
         SBC(2, IMMEDIATE_BYTE);
         break;

      case 0xEA:  /* NOP */
         NOP();
         break;

      case 0xEC:  /* CPX $nnnn */
         CPX(4, ABSOLUTE_BYTE);
         break;

      case 0xED:  /* SBC $nnnn */
         SBC(4, ABSOLUTE_BYTE);
         break;

      case 0xEE:  /* INC $nnnn */
         INC(6, ABSOLUTE, mem_write, addr);
         break;

      case 0xEF:  /* ISB $nnnn */
         ISB(6, ABSOLUTE, mem_write, addr);
         break;

      case 0xF0:  /* BEQ $nnnn */
         BEQ();
         break;

      case 0xF1:  /* SBC ($nn),Y */
         SBC(5, INDIR_Y_BYTE);
         break;

      case 0xF3:  /* ISB ($nn),Y */
         ISB(8, INDIR_Y, mem_write, addr);
         break;

      case 0xF5:  /* SBC $nn,X */
         SBC(4, ZP_IND_X_BYTE);
         break;

      case 0xF6:  /* INC $nn,X */
         INC(6, ZP_IND_X, ZP_WRITE, baddr);
         break;

      case 0xF7:  /* ISB $nn,X */
         ISB(6, ZP_IND_X, ZP_WRITE, baddr);
         break;

      case 0xF8:  /* SED */
         SED();
         break;

      case 0xF9:  /* SBC $nnnn,Y */
         SBC(4, ABS_IND_Y_BYTE);
         break;

      case 0xFB:  /* ISB $nnnn,Y */
         ISB(7, ABS_IND_Y, mem_write, addr);
         break;

      case 0xFD:  /* SBC $nnnn,X */
         SBC(4, ABS_IND_X_BYTE);
         break;

      case 0xFE:  /* INC $nnnn,X */
         INC(7, ABS_IND_X, mem_write, addr);
         break;

      case 0xFF:  /* ISB $nnnn,X */
         ISB(7, ABS_IND_X, mem_write, addr);
         break;
      }

      /* Calculate remaining/elapsed clock cycles */
      remaining_cycles -= instruction_cycles;
      total_cycles += instruction_cycles;
   }

_execute_done:

   /* restore local copy of regs */
   SET_LOCAL_REGS();

   /* Return our actual amount of executed cycles */
   return (total_cycles - old_cycles);
}

/* Initialize tables, etc. */
void nes6502_init(void)
{
   int index;

   /* Build the N / Z flag lookup table */
   flag_table[0] = Z_FLAG;

   for (index = 1; index < 256; index++)
      flag_table[index] = (index & 0x80) ? N_FLAG : 0;

   reg_A = reg_X = reg_Y = 0;
   reg_S = 0xFF;                             /* Stack grows down */
}


/* Issue a CPU Reset */
void nes6502_reset(void)
{
   reg_P = Z_FLAG | R_FLAG | I_FLAG;         /* Reserved bit always 1 */
   int_pending = dma_cycles = 0;             /* No pending interrupts */
   reg_PC = bank_readaddress(RESET_VECTOR);  /* Fetch reset vector */
   /* TODO: 6 cycles for RESET? */
}

/* Non-maskable interrupt */
void nes6502_nmi(void)
{
   int_pending |= NMI_MASK;
}

/* Interrupt request */
void nes6502_irq(void)
{
   int_pending |= IRQ_MASK;
}

/* Set dma period (in cycles) */
void nes6502_setdma(int cycles)
{
   dma_cycles += cycles;
}

/*
** $Log: nes6502.c,v $
** Revision 1.6  2000/07/04 04:50:07  matt
** minor change to includes
**
** Revision 1.5  2000/07/03 02:18:16  matt
** added a few notes about potential failure cases
**
** Revision 1.4  2000/06/09 15:12:25  matt
** initial revision
**
*/

