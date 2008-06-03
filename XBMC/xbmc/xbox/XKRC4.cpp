#include "stdafx.h" 
/*
**********************************
**********************************
**      BROUGHT TO YOU BY:  **
**********************************
**********************************
**        **
**    [TEAM ASSEMBLY]  **
**        **
**  www.team-assembly.com **
**        **
******************************************************************************************************
* This is free software; you can redistribute it and/or modify
* it under the terms of the GNU General Public License as published by
* the Free Software Foundation; either version 2 of the License, or
* (at your option) any later version.
* 
* This program is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
* GNU General Public License for more details.
*
* You should have received a copy of the GNU General Public License
* along with this program; if not, write to the Free Software
* Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
******************************************************************************************************
 
********************************************************************************************************
**      XKRC4.CPP - General RC4 Encryption Class' Implementation
********************************************************************************************************
**
** This is the Class Contains basic RC4 encryption functionality...
**
********************************************************************************************************
 
 
********************************************************************************************************
** CREDITS:
********************************************************************************************************
** SPEEDBUMP:
** ---------
**  My utmost greatfullness and admiration goes towards SpeedBump for all his hard work..
**  I used most of his code and converted to C++ objects etc..
**
** XBOX-LINUX TEAM:
** ---------------
**  Wow, you guys are awsome !!  I bow down to your greatness !!  
**  REFERENCE URL:  http://xbox-linux.sourceforge.net
**
********************************************************************************************************
 
UPDATE LOG:
--------------------------------------------------------------------------------------------------------
Date: 02/18/2003
By: UNDEAD [team-assembly]
Reason: Prepared 0.2 for Public Release
--------------------------------------------------------------------------------------------------------
Date: 01/06/2003
By: UNDEAD [team-assembly]
Reason: Prepared for Public Release
--------------------------------------------------------------------------------------------------------
 
*/
#include "XKRC4.h"

XKRC4::XKRC4(void)
{}

XKRC4::~XKRC4(void)
{}

void XKRC4::InitRC4Key(UCHAR* pRC4KeyData, int KeyLen, RC4KEY *pRC4Key)
{
  UCHAR index1;
  UCHAR index2;
  UCHAR* state;
  short counter;

  state = &pRC4Key->state[0];

  for (counter = 0; counter < 256; counter++)
    state[counter] = (UCHAR)counter;

  pRC4Key->x = 0;
  pRC4Key->y = 0;
  index1 = 0;
  index2 = 0;

  for (counter = 0; counter < 256; counter++)
  {
    index2 = (pRC4KeyData[index1] + state[counter] + index2) % 256;
    swap_byte(&state[counter], &state[index2]);
    index1 = (index1 + 1) % KeyLen;
  }
}

void XKRC4::RC4EnDecrypt(UCHAR* pData, int DataLen, RC4KEY *pRC4key)
{
  unsigned char x;
  unsigned char y;
  unsigned char* state;
  unsigned char xorIndex;
  long counter;

  x = pRC4key->x;
  y = pRC4key->y;
  state = &pRC4key->state[0];

  for (counter = 0; counter < DataLen; counter ++)
  {
    x = (x + 1) % 256;
    y = (state[x] + y) % 256;
    swap_byte(&state[x], &state[y]);
    xorIndex = (state[x] + state[y]) % 256;
    pData[counter] ^= state[xorIndex];
  }

  pRC4key->x = x;
  pRC4key->y = y;
}

void XKRC4::swap_byte(unsigned char *a, unsigned char *b)
{
  unsigned char swapByte;
  swapByte = *a;
  *a = *b;
  *b = swapByte;
}
