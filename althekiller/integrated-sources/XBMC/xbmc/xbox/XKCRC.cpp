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
**      XKCRC.CPP - General CRC Class' Implementation
********************************************************************************************************
**
** Containss functions to do a simple CRC check on a block of data..  
** might be more ways of elegantly doing this, but for now this has to work...
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
#include "XKCRC.h"


XKCRC::XKCRC(void)
{}

XKCRC::~XKCRC(void)
{}


void XKCRC::QuickCRC(UCHAR* CRCVAL, UCHAR* inData, DWORD dataLen)
{

  LPBYTE CRC_Data = new BYTE[dataLen + 4];
  ZeroMemory(CRC_Data, dataLen + 4);
  memcpy(CRC_Data + 0x01 , inData, dataLen - 1); //We Circle shift the whole bunch 1 byte to the right
  memcpy(CRC_Data, inData + dataLen - 1, 0x01); //We Circle shift the whole bunch 1 byte to the right


  BYTE CRCVALUE[4];
  ZeroMemory(CRCVALUE, 4);

  for (int CRCPos = 0; CRCPos < 4; CRCPos++)
  {
    WORD CRCPosVal = 0xFFFF;

    for (DWORD l = CRCPos; l < dataLen; l += 4)
    {
      LPWORD tmpWrd = (LPWORD)(&CRC_Data[l]);
      WORD ival = *tmpWrd;
      CRCPosVal = CRCPosVal - ival;
    }

    CRCPosVal &= 0xFF00;
    CRCVALUE[CRCPos] = (BYTE) (CRCPosVal >> 8);
  }

  memcpy(CRCVAL, CRCVALUE, 4);
  delete[] CRC_Data;

}
