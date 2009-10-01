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
** dis6502.c
**
** 6502 disassembler based on code from John Saeger
** $Id: dis6502.c,v 1.4 2000/06/09 15:12:25 matt Exp $
*/

#include "../../types.h"
#include "../../log.h"
#include "nes6502.h"
#include "dis6502.h"

#ifdef NES6502_DEBUG

/* addressing modes */
enum { _imp, _acc, _rel, _imm, _abs, _abs_x, _abs_y, _zero, _zero_x, _zero_y, _ind, _ind_x, _ind_y };

/* keep a filthy local copy of PC to
** reduce the amount of parameter passing
*/
static uint32 pc_reg;


static uint8 dis_op8(void)
{
   return (nes6502_getbyte(pc_reg + 1));
}

static uint16 dis_op16(void)
{
   return (nes6502_getbyte(pc_reg + 1) + (nes6502_getbyte(pc_reg + 2) << 8));
}

static void dis_show_ind(void)
{
   log_printf("(%04X)  ", dis_op16());
}

static void dis_show_ind_x(void)
{
   log_printf("(%02X,x)  ", dis_op8());
}

static void dis_show_ind_y(void)
{
   log_printf("(%02X),y  ", dis_op8());
}

static void dis_show_zero_x(void)
{
   log_printf(" %02X,x   ", dis_op8());
}

static void dis_show_zero_y(void)
{
   log_printf(" %02X,y   ", dis_op8());
}

static void dis_show_abs_y(void)
{
   log_printf(" %04X,y ", dis_op16());
}

static void dis_show_abs_x(void)
{
   log_printf(" %04X,x ", dis_op16());
}

static void dis_show_zero(void)
{
   log_printf(" %02X     ", dis_op8());
}

static void dis_show_abs(void)
{
   log_printf(" %04X   ", dis_op16());
}

static void dis_show_immediate(void)
{
   log_printf("#%02X     ", dis_op8());
}

static void dis_show_acc(void)
{
   log_printf(" a      ");
}

static void dis_show_relative(void)
{
   int target;

   target = (int8) dis_op8();
   target += (pc_reg + 2);
   log_printf(" %04X   ", target);
}

static void dis_show_code(int optype)
{
   log_printf("%02X ", nes6502_getbyte(pc_reg));

   switch (optype)
   {
   case _imp:
   case _acc: 
      log_printf("      ");
      break;

   case _rel:
   case _imm:
   case _zero:
   case _zero_x:
      log_printf("%02X    ", nes6502_getbyte(pc_reg + 1));
      break;

   case _abs:
   case _abs_x:
   case _abs_y:
   case _ind:
   case _ind_x:
   case _ind_y:
      log_printf("%02X %02X ", nes6502_getbyte(pc_reg + 1), nes6502_getbyte(pc_reg + 2));
      break;
   }
}

static void dis_show_op(char *opstr, int optype)
{
   dis_show_code(optype);
   log_printf("%s ", opstr);

   switch(optype)
   {
   case _imp:     log_printf("        "); break;
   case _acc:     dis_show_acc();         break;
   case _rel:     dis_show_relative();    break;
   case _imm:     dis_show_immediate();   break;
   case _abs:     dis_show_abs();         break;
   case _abs_x:   dis_show_abs_x();       break;
   case _abs_y:   dis_show_abs_y();       break;
   case _zero:    dis_show_zero();        break;
   case _zero_x:  dis_show_zero_x();      break;
   case _ind:     dis_show_ind();         break;
   case _ind_x:   dis_show_ind_x();       break;
   case _ind_y:   dis_show_ind_y();       break;
   }
}

void nes6502_disasm(uint32 PC, uint8 P, uint8 A, uint8 X, uint8 Y, uint8 S)
{
   pc_reg = PC;

   log_printf("%04X: ", pc_reg);

   switch(nes6502_getbyte(pc_reg))
   {
   case 0x00: dis_show_op("brk",_imp);    break;
   case 0x01: dis_show_op("ora",_ind_x);  break;
   case 0x02: dis_show_op("jam",_imp);    break;
   case 0x03: dis_show_op("slo",_ind_x);  break;
   case 0x04: dis_show_op("nop",_zero);   break;
   case 0x05: dis_show_op("ora",_zero);   break;
   case 0x06: dis_show_op("asl",_zero);   break;
   case 0x07: dis_show_op("slo",_zero);   break;
   case 0x08: dis_show_op("php",_imp);    break;
   case 0x09: dis_show_op("ora",_imm);    break;
   case 0x0a: dis_show_op("asl",_acc);    break;
   case 0x0b: dis_show_op("anc",_imm);    break;
   case 0x0c: dis_show_op("nop",_abs);    break;
   case 0x0d: dis_show_op("ora",_abs);    break;
   case 0x0e: dis_show_op("asl",_abs);    break;
   case 0x0f: dis_show_op("slo",_abs);    break;

   case 0x10: dis_show_op("bpl",_rel);    break;
   case 0x11: dis_show_op("ora",_ind_y);  break;
   case 0x12: dis_show_op("jam",_imp);    break;
   case 0x13: dis_show_op("slo",_ind_y);  break;
   case 0x14: dis_show_op("nop",_zero_x); break;
   case 0x15: dis_show_op("ora",_zero_x); break;
   case 0x16: dis_show_op("asl",_zero_x); break;
   case 0x17: dis_show_op("slo",_zero_x); break;
   case 0x18: dis_show_op("clc",_imp);    break;
   case 0x19: dis_show_op("ora",_abs_y);  break;
   case 0x1a: dis_show_op("nop",_imp);    break;
   case 0x1b: dis_show_op("slo",_abs_y);  break;
   case 0x1c: dis_show_op("nop",_abs_x);  break;
   case 0x1d: dis_show_op("ora",_abs_x);  break;
   case 0x1e: dis_show_op("asl",_abs_x);  break;
   case 0x1f: dis_show_op("slo",_abs_x);  break;

   case 0x20: dis_show_op("jsr",_abs);    break;
   case 0x21: dis_show_op("and",_ind_x);  break;
   case 0x22: dis_show_op("jam",_imp);    break;
   case 0x23: dis_show_op("rla",_ind_x);  break;
   case 0x24: dis_show_op("bit",_zero);   break;
   case 0x25: dis_show_op("and",_zero);   break;
   case 0x26: dis_show_op("rol",_zero);   break;
   case 0x27: dis_show_op("rla",_zero);   break;
   case 0x28: dis_show_op("plp",_imp);    break;
   case 0x29: dis_show_op("and",_imm);    break;
   case 0x2a: dis_show_op("rol",_acc);    break;
   case 0x2b: dis_show_op("anc",_imm);    break;
   case 0x2c: dis_show_op("bit",_abs);    break;
   case 0x2d: dis_show_op("and",_abs);    break;
   case 0x2e: dis_show_op("rol",_abs);    break;
   case 0x2f: dis_show_op("rla",_abs);    break;

   case 0x30: dis_show_op("bmi",_rel);    break;
   case 0x31: dis_show_op("and",_ind_y);  break;
   case 0x32: dis_show_op("jam",_imp);    break;
   case 0x33: dis_show_op("rla",_ind_y);  break;
/*   case 0x34: dis_show_op("nop",_zero);   break;*/
   case 0x34: dis_show_op("nop",_imp);    break;
   case 0x35: dis_show_op("and",_zero_x); break;
   case 0x36: dis_show_op("rol",_zero_x); break;
   case 0x37: dis_show_op("rla",_zero_x); break;
   case 0x38: dis_show_op("sec",_imp);    break;
   case 0x39: dis_show_op("and",_abs_y);  break;
   case 0x3a: dis_show_op("nop",_imp);    break;
   case 0x3b: dis_show_op("rla",_abs_y);  break;
/*   case 0x3c: dis_show_op("nop",_imp);    break;*/
   case 0x3c: dis_show_op("nop",_abs_x);  break;
   case 0x3d: dis_show_op("and",_abs_x);  break;
   case 0x3e: dis_show_op("rol",_abs_x);  break;
   case 0x3f: dis_show_op("rla",_abs_x);  break;

   case 0x40: dis_show_op("rti",_imp);    break;
   case 0x41: dis_show_op("eor",_ind_x);  break;
   case 0x42: dis_show_op("jam",_imp);    break;
   case 0x43: dis_show_op("sre",_ind_x);  break;
   case 0x44: dis_show_op("nop",_zero);   break;
   case 0x45: dis_show_op("eor",_zero);   break;
   case 0x46: dis_show_op("lsr",_zero);   break;
   case 0x47: dis_show_op("sre",_zero);   break;
   case 0x48: dis_show_op("pha",_imp);    break;
   case 0x49: dis_show_op("eor",_imm);    break;
   case 0x4a: dis_show_op("lsr",_acc);    break;
   case 0x4b: dis_show_op("asr",_imm);    break;
   case 0x4c: dis_show_op("jmp",_abs);    break;
   case 0x4d: dis_show_op("eor",_abs);    break;
   case 0x4e: dis_show_op("lsr",_abs);    break;
   case 0x4f: dis_show_op("sre",_abs);    break;

   case 0x50: dis_show_op("bvc",_rel);    break;
   case 0x51: dis_show_op("eor",_ind_y);  break;
   case 0x52: dis_show_op("jam",_imp);    break;
   case 0x53: dis_show_op("sre",_ind_y);  break;
   case 0x54: dis_show_op("nop",_zero_x); break;
   case 0x55: dis_show_op("eor",_zero_x); break;
   case 0x56: dis_show_op("lsr",_zero_x); break;
   case 0x57: dis_show_op("sre",_zero_x); break;
   case 0x58: dis_show_op("cli",_imp);    break;
   case 0x59: dis_show_op("eor",_abs_y);  break;
   case 0x5a: dis_show_op("nop",_imp);    break;
   case 0x5b: dis_show_op("sre",_abs_y);  break;
   case 0x5c: dis_show_op("nop",_abs_x);  break;
   case 0x5d: dis_show_op("eor",_abs_x);  break;
   case 0x5e: dis_show_op("lsr",_abs_x);  break;
   case 0x5f: dis_show_op("sre",_abs_x);  break;
 
   case 0x60: dis_show_op("rts",_imp);    break;
   case 0x61: dis_show_op("adc",_ind_x);  break;
   case 0x62: dis_show_op("jam",_imp);    break;
   case 0x63: dis_show_op("rra",_ind_x);  break;
   case 0x64: dis_show_op("nop",_zero);   break;
   case 0x65: dis_show_op("adc",_zero);   break;
   case 0x66: dis_show_op("ror",_zero);   break;
   case 0x67: dis_show_op("rra",_zero);   break;
   case 0x68: dis_show_op("pla",_imp);    break;
   case 0x69: dis_show_op("adc",_imm);    break;
   case 0x6a: dis_show_op("ror",_acc);    break;
   case 0x6b: dis_show_op("arr",_imm);    break;
   case 0x6c: dis_show_op("jmp",_ind);    break;
   case 0x6d: dis_show_op("adc",_abs);    break;
   case 0x6e: dis_show_op("ror",_abs);    break;
   case 0x6f: dis_show_op("rra",_abs);    break;

   case 0x70: dis_show_op("bvs",_rel);    break;
   case 0x71: dis_show_op("adc",_ind_y);  break;
   case 0x72: dis_show_op("jam",_imp);    break;
   case 0x73: dis_show_op("rra",_ind_y);  break;
   case 0x74: dis_show_op("nop",_zero_x); break;
   case 0x75: dis_show_op("adc",_zero_x); break;
   case 0x76: dis_show_op("ror",_zero_x); break;
   case 0x77: dis_show_op("rra",_zero_x); break;
   case 0x78: dis_show_op("sei",_imp);    break;
   case 0x79: dis_show_op("adc",_abs_y);  break;
   case 0x7a: dis_show_op("nop",_imp);    break;
   case 0x7b: dis_show_op("rra",_abs_y);  break;
   case 0x7c: dis_show_op("nop",_abs_x);  break;
   case 0x7d: dis_show_op("adc",_abs_x);  break;
   case 0x7e: dis_show_op("ror",_abs_x);  break;
   case 0x7f: dis_show_op("rra",_abs_x);  break;

   case 0x80: dis_show_op("nop",_imm);    break;
   case 0x81: dis_show_op("sta",_ind_x);  break;
   case 0x82: dis_show_op("nop",_imm);    break;
   case 0x83: dis_show_op("sax",_ind_x);  break;
   case 0x84: dis_show_op("sty",_zero);   break;
   case 0x85: dis_show_op("sta",_zero);   break;
   case 0x86: dis_show_op("stx",_zero);   break;
   case 0x87: dis_show_op("sax",_zero);   break;
   case 0x88: dis_show_op("dey",_imp);    break;
   case 0x89: dis_show_op("nop",_imm);    break;
   case 0x8a: dis_show_op("txa",_imp);    break;
   case 0x8b: dis_show_op("ane",_imm);    break;
   case 0x8c: dis_show_op("sty",_abs);    break;
   case 0x8d: dis_show_op("sta",_abs);    break;
   case 0x8e: dis_show_op("stx",_abs);    break;
   case 0x8f: dis_show_op("sax",_abs);    break;

   case 0x90: dis_show_op("bcc",_rel);    break;
   case 0x91: dis_show_op("sta",_ind_y);  break;
   case 0x92: dis_show_op("jam",_imp);    break;
   case 0x93: dis_show_op("sha",_ind_y);  break;
   case 0x94: dis_show_op("sty",_zero_x); break;
   case 0x95: dis_show_op("sta",_zero_x); break;
   case 0x96: dis_show_op("stx",_zero_y); break;
   case 0x97: dis_show_op("sax",_zero_y); break;
   case 0x98: dis_show_op("tya",_imp);    break;
   case 0x99: dis_show_op("sta",_abs_y);  break;
   case 0x9a: dis_show_op("txs",_imp);    break;
   case 0x9b: dis_show_op("shs",_abs_y);  break;
   case 0x9c: dis_show_op("shy",_abs_x);  break;
   case 0x9d: dis_show_op("sta",_abs_x);  break;
   case 0x9e: dis_show_op("shx",_abs_y);  break;
   case 0x9f: dis_show_op("sha",_abs_y);  break;

   case 0xa0: dis_show_op("ldy",_imm);    break;
   case 0xa1: dis_show_op("lda",_ind_x);  break;
   case 0xa2: dis_show_op("ldx",_imm);    break;
   case 0xa3: dis_show_op("lax",_ind_x);  break;
   case 0xa4: dis_show_op("ldy",_zero);   break;
   case 0xa5: dis_show_op("lda",_zero);   break;
   case 0xa6: dis_show_op("ldx",_zero);   break;
   case 0xa7: dis_show_op("lax",_zero);   break;
   case 0xa8: dis_show_op("tay",_imp);    break;
   case 0xa9: dis_show_op("lda",_imm);    break;
   case 0xaa: dis_show_op("tax",_imp);    break;
   case 0xab: dis_show_op("lxa",_imm);    break;
   case 0xac: dis_show_op("ldy",_abs);    break;
   case 0xad: dis_show_op("lda",_abs);    break;
   case 0xae: dis_show_op("ldx",_abs);    break;
   case 0xaf: dis_show_op("lax",_abs);    break;

   case 0xb0: dis_show_op("bcs",_rel);    break;
   case 0xb1: dis_show_op("lda",_ind_y);  break;
   case 0xb2: dis_show_op("jam",_imp);    break;
   case 0xb3: dis_show_op("lax",_ind_y);  break;
   case 0xb4: dis_show_op("ldy",_zero_x); break;
   case 0xb5: dis_show_op("lda",_zero_x); break;
   case 0xb6: dis_show_op("ldx",_zero_y); break;
   case 0xb7: dis_show_op("lax",_zero_y); break;
   case 0xb8: dis_show_op("clv",_imp);    break;
   case 0xb9: dis_show_op("lda",_abs_y);  break;
   case 0xba: dis_show_op("tsx",_imp);    break;
   case 0xbb: dis_show_op("las",_abs_y);  break;
   case 0xbc: dis_show_op("ldy",_abs_x);  break;
   case 0xbd: dis_show_op("lda",_abs_x);  break;
   case 0xbe: dis_show_op("ldx",_abs_y);  break;
   case 0xbf: dis_show_op("lax",_abs_y);  break;

   case 0xc0: dis_show_op("cpy",_imm);    break;
   case 0xc1: dis_show_op("cmp",_ind_x);  break;
   case 0xc2: dis_show_op("nop",_imm);    break;
   case 0xc3: dis_show_op("dcp",_ind_x);  break;
   case 0xc4: dis_show_op("cpy",_zero);   break;
   case 0xc5: dis_show_op("cmp",_zero);   break;
   case 0xc6: dis_show_op("dec",_zero);   break;
   case 0xc7: dis_show_op("dcp",_zero);   break;
   case 0xc8: dis_show_op("iny",_imp);    break;
   case 0xc9: dis_show_op("cmp",_imm);    break;
   case 0xca: dis_show_op("dex",_imp);    break;
   case 0xcb: dis_show_op("sbx",_imm);    break;
   case 0xcc: dis_show_op("cpy",_abs);    break;
   case 0xcd: dis_show_op("cmp",_abs);    break;
   case 0xce: dis_show_op("dec",_abs);    break;
   case 0xcf: dis_show_op("dcp",_abs);    break;

   case 0xd0: dis_show_op("bne",_rel);    break;
   case 0xd1: dis_show_op("cmp",_ind_y);  break;
   case 0xd2: dis_show_op("jam",_imp);    break;
   case 0xd3: dis_show_op("dcp",_ind_y);  break;
   case 0xd4: dis_show_op("nop",_zero_x); break;
   case 0xd5: dis_show_op("cmp",_zero_x); break;
   case 0xd6: dis_show_op("dec",_zero_x); break;
   case 0xd7: dis_show_op("dcp",_zero_x); break;
   case 0xd8: dis_show_op("cld",_imp);    break;
   case 0xd9: dis_show_op("cmp",_abs_y);  break;
   case 0xda: dis_show_op("nop",_imp);    break;
   case 0xdb: dis_show_op("dcp",_abs_y);  break;
   case 0xdc: dis_show_op("nop",_abs_x);  break;
   case 0xdd: dis_show_op("cmp",_abs_x);  break;
   case 0xde: dis_show_op("dec",_abs_x);  break;
   case 0xdf: dis_show_op("dcp",_abs_x);  break;

   case 0xe0: dis_show_op("cpx",_imm);    break;
   case 0xe1: dis_show_op("sbc",_ind_x);  break;
   case 0xe2: dis_show_op("nop",_imm);    break;
   case 0xe3: dis_show_op("isb",_ind_x);  break;
   case 0xe4: dis_show_op("cpx",_zero);   break;
   case 0xe5: dis_show_op("sbc",_zero);   break;
   case 0xe6: dis_show_op("inc",_zero);   break;
   case 0xe7: dis_show_op("isb",_zero);   break;
   case 0xe8: dis_show_op("inx",_imp);    break;
   case 0xe9: dis_show_op("sbc",_imm);    break;
   case 0xea: dis_show_op("nop",_imp);    break;
   case 0xeb: dis_show_op("sbc",_imm);    break;
   case 0xec: dis_show_op("cpx",_abs);    break;
   case 0xed: dis_show_op("sbc",_abs);    break;
   case 0xee: dis_show_op("inc",_abs);    break;
   case 0xef: dis_show_op("isb",_abs);    break;

   case 0xf0: dis_show_op("beq",_rel);    break;
   case 0xf1: dis_show_op("sbc",_ind_y);  break;
   case 0xf2: dis_show_op("jam",_imp);    break;
   case 0xf3: dis_show_op("isb",_ind_y);  break;
   case 0xf4: dis_show_op("nop",_zero_x); break;
   case 0xf5: dis_show_op("sbc",_zero_x); break;
   case 0xf6: dis_show_op("inc",_zero_x); break;
   case 0xf7: dis_show_op("isb",_zero_x); break;
   case 0xf8: dis_show_op("sed",_imp);    break;
   case 0xf9: dis_show_op("sbc",_abs_y);  break;
   case 0xfa: dis_show_op("nop",_imp);    break;
   case 0xfb: dis_show_op("isb",_abs_y);  break;
   case 0xfc: dis_show_op("nop",_abs_x);  break;
   case 0xfd: dis_show_op("sbc",_abs_x);  break;
   case 0xfe: dis_show_op("inc",_abs_x);  break;
   case 0xff: dis_show_op("isb",_abs_x);  break;
   }

   log_printf("%c%c1%c%c%c%c%c %02X %02X %02X %02X\n",
      (P & N_FLAG) ? 'N' : 'n',
      (P & V_FLAG) ? 'V' : 'v',
      (P & V_FLAG) ? 'B' : 'b',
      (P & V_FLAG) ? 'D' : 'd',
      (P & V_FLAG) ? 'I' : 'i',
      (P & V_FLAG) ? 'Z' : 'z',
      (P & V_FLAG) ? 'C' : 'c',
      A, X, Y, S);
}

#endif /* NES6502_DEBUG */

/*
** $Log: dis6502.c,v $
** Revision 1.4  2000/06/09 15:12:25  matt
** initial revision
**
*/

