/***************************************************************************
 * Gens: TI SN76489 (PSG) emulator.                                        *
 *                                                                         *
 * Copyright (c) 1999-2002 by Stéphane Dallongeville                       *
 * Copyright (c) 2003-2004 by Stéphane Akhoun                              *
 * Copyright (c) 2008-2009 by David Korth                                  *
 *                                                                         *
 * This program is free software; you can redistribute it and/or modify it *
 * under the terms of the GNU General Public License as published by the   *
 * Free Software Foundation; either version 2 of the License, or (at your  *
 * option) any later version.                                              *
 *                                                                         *
 * This program is distributed in the hope that it will be useful, but     *
 * WITHOUT ANY WARRANTY; without even the implied warranty of              *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the           *
 * GNU General Public License for more details.                            *
 *                                                                         *
 * You should have received a copy of the GNU General Public License along *
 * with this program; if not, write to the Free Software Foundation, Inc., *
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.           *
 ***************************************************************************/

#ifndef _PSG_H
#define _PSG_H

#ifdef __cplusplus
extern "C" {
#endif


extern unsigned int PSG_Save[8];

struct _psg
{
	int Current_Channel;
	int Current_Register;
	int Register[8];
	unsigned int Counter[4];
	unsigned int CntStep[4];
	int Volume[4];
	unsigned int Noise_Type;
	unsigned int Noise;
};

extern struct _psg PSG;

/* Gens */

extern int PSG_Enable;
extern int PSG_Improv;
extern int *PSG_Buf[2];
extern int PSG_Len;

extern int PSG_Chan_Enable[4];

/* end */

void PSG_Write(int data);
void PSG_Update_SIN(int **buffer, int length);
void PSG_Update(int **buffer, int length);
void PSG_Init(int clock, int rate);
void PSG_Save_State(void);
void PSG_Restore_State(void);

/* Gens */

void PSG_Special_Update(void);

#ifdef __cplusplus
};
#endif

#endif
