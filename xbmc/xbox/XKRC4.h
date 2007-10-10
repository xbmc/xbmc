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
**      XKRC4.H - General RC4 Encryption Class' Header
********************************************************************************************************
**
** This is the Class Header, see the .CPP file for more comments and implementation details.
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
#pragma once
#if defined (_WINDOWS)
 #pragma message ("Compiling for WINDOWS: " __FILE__)
// #include <afxwin.h>         // MFC core and standard components
#elif defined (_XBOX)
 #pragma message ("Compiling for XBOX: " __FILE__)
 #include <xtl.h>
#elif !defined (_LINUX)
 #error ERR: Have to Define _WINDOWS or _XBOX or _LINUX!!
#endif

class XKRC4
{
protected:
  void swap_byte(UCHAR* a, UCHAR* b);

public:
  struct RC4KEY
  {
    UCHAR state[256];
    UCHAR x;
    UCHAR y;
  };

  XKRC4(void);
  virtual ~XKRC4(void);

  void InitRC4Key(UCHAR* pRC4KeyData, int KeyLen, RC4KEY* pRC4Key);
  void RC4EnDecrypt(UCHAR* pData, int DataLen, RC4KEY* pRC4key);


};

