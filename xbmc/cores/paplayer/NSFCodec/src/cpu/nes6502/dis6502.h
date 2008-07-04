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
** dis6502.h
**
** 6502 disassembler header
** $Id: dis6502.h,v 1.4 2000/06/09 15:12:25 matt Exp $
*/

#ifndef _DIS6502_H_
#define _DIS6502_H_

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

extern void nes6502_disasm(uint32 PC, uint8 P, uint8 A, uint8 X, uint8 Y, uint8 S);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* !_DIS6502_H_ */

/*
** $Log: dis6502.h,v $
** Revision 1.4  2000/06/09 15:12:25  matt
** initial revision
**
*/

