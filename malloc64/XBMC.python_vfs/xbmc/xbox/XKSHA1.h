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
**      XKSHA1.H - General SHA1 and HMAC_SHA1 Class' Header
********************************************************************************************************
**
** This is the Class Header, see the .CPP file for more comments and implementation details.
**
**      This file implements the Secure Hashing Algorithm 1 as
**      defined in FIPS PUB 180-1 published April 17, 1995.
**
**
**  Added Funcionality to NOT require the KEY when doing HMAC_SHA1 hashes 
**  as per xbox-Linux groups "Fridaym 13th Middle Message Hack" 
**
********************************************************************************************************


********************************************************************************************************
** CREDITS:
********************************************************************************************************
** SPEEDBUMP:
**  My utmost greatfullness and admiration goes towards SpeedBump for all his hard work..
**  I used most of his code and converted to C++ objects etc..
**
** XBOX-LINUX TEAM:
**  ---------------
**  In particular "Franz", Wow, you guys are awsome !!  I bow down to your greatness !!  
**  The "Friday 13th" Middle Message Hack really saved our butts !! 
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
 #include <stdlib.h>
#elif !defined (_LINUX)
 #error ERR: Have to Define _WINDOWS or _XBOX or _LINUX!!
#endif

#define SHA1HashSize 20
#define SHA1CircularShift(bits,word) \
                (((word) << (bits)) | ((word) >> (32-(bits))))

class XKSHA1
{
  struct SHA1Context
  {
    UINT32 Intermediate_Hash[SHA1HashSize / 4]; /* Message Digest  */

    UINT32 Length_Low;   /* Message length in bits      */
    UINT32 Length_High;   /* Message length in bits      */

    DWORD Message_Block_Index; /* Index into message block array   */
    UCHAR Message_Block[64];    /* 512-bit message blocks      */

    int Computed;    /* Is the digest computed?         */
    int Corrupted;    /* Is the message digest corrupted? */
  };

  enum
  {
    shaSuccess = 0,
    shaNull,             /* Null pointer parameter */
    shaInputTooLong,     /* input data too long */
    shaStateError       /* called Input after Result */
  };

public:
  XKSHA1(void);
  ~XKSHA1(void);

  void HMAC_SHA1(UCHAR* result, UCHAR* key, int key_length, UCHAR* text1, int text1_length, UCHAR* text2, int text2_length);
  void quick_SHA1( UCHAR* SHA1_result, ... );

  //Skip the Key used from eeprom.. Kudos franz@caos.at
  void XBOX_HMAC_SHA1(int version, UCHAR* result, ... );

private:
  int SHA1Reset(SHA1Context*);
  int SHA1Input(SHA1Context*, const UCHAR* , unsigned int);
  int SHA1Result(SHA1Context*, UCHAR Message_Digest[SHA1HashSize]);
  void SHA1ProcessMessageBlock(SHA1Context* context);
  void SHA1PadMessage(SHA1Context* context);

  //Skip the Key used from eeprom.. Kudos franz@caos.at
  int HMAC1Reset(int version, SHA1Context *context);
  int HMAC2Reset(int version, SHA1Context *context);

};

