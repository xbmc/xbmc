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
** nes6502.h
**
** NES custom 6502 CPU definitions / prototypes
** $Id: nes6502.h,v 1.4 2000/06/09 15:12:25 matt Exp $
*/

/* NOTE: 16-bit addresses avoided like the plague: use 32-bit values
**       wherever humanly possible
*/
#ifndef _NES6502_H_
#define _NES6502_H_

/* Define this to enable decimal mode in ADC / SBC (not needed in NES) */
/*#define  NES6502_DECIMAL*/

/* number of bank pointers the CPU emulation core handles */
#ifdef NSF_PLAYER
#define  NES6502_4KBANKS
#endif

#ifdef NES6502_4KBANKS
#define  NES6502_NUMBANKS  16
#define  NES6502_BANKSHIFT 12
#else
#define  NES6502_NUMBANKS  8
#define  NES6502_BANKSHIFT 13
#endif

#define  NES6502_BANKMASK  ((0x10000 / NES6502_NUMBANKS) - 1)


/* P (flag) register bitmasks */
#define  N_FLAG         0x80
#define  V_FLAG         0x40
#define  R_FLAG         0x20  /* Reserved, always 1 */
#define  B_FLAG         0x10
#define  D_FLAG         0x08
#define  I_FLAG         0x04
#define  Z_FLAG         0x02
#define  C_FLAG         0x01

/* Vector addresses */
#define  NMI_VECTOR     0xFFFA
#define  RESET_VECTOR   0xFFFC
#define  IRQ_VECTOR     0xFFFE

/* cycle counts for interrupts */
#define  INT_CYCLES     7
#define  RESET_CYCLES   6

#define  NMI_MASK       0x01
#define  IRQ_MASK       0x02

/* Stack is located on 6502 page 1 */
#define  STACK_OFFSET   0x0100

typedef struct
{
   uint32 min_range, max_range;
   uint8 (*read_func)(uint32 address);
} nes6502_memread;

typedef struct
{
   uint32 min_range, max_range;
   void (*write_func)(uint32 address, uint8 value);
} nes6502_memwrite;

typedef struct
{
   uint8 *mem_page[NES6502_NUMBANKS];  /* memory page pointers */
   nes6502_memread *read_handler;
   nes6502_memwrite *write_handler;
   int dma_cycles;
   uint32 pc_reg;
   uint8 a_reg, p_reg, x_reg, y_reg, s_reg;
   uint8 int_pending;
} nes6502_context;

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

/* Functions which govern the 6502's execution */
extern void nes6502_init(void);
extern void nes6502_reset(void);
extern int nes6502_execute(int total_cycles);
extern void nes6502_nmi(void);
extern void nes6502_irq(void);
extern uint8 nes6502_getbyte(uint32 address);
extern uint32 nes6502_getcycles(boolean reset_flag);
extern void nes6502_setdma(int cycles);

/* Context get/set */
extern void nes6502_setcontext(nes6502_context *cpu);
extern void nes6502_getcontext(nes6502_context *cpu);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* _NES6502_H_ */

/*
** $Log: nes6502.h,v $
** Revision 1.4  2000/06/09 15:12:25  matt
** initial revision
**
*/

