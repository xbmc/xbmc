#include "stdafx.h"
/*
**********************************
**********************************
**      BROUGHT TO YOU BY:      **
**********************************
**********************************
**                              **
**       [TEAM ASSEMBLY]        **
**                              **
**     www.team-assembly.com    **
**                              **
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
**      XKSHA1.CPP - SHA1 and HMAC_SHA1 Class' Implementation
********************************************************************************************************
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
Date: 11/27/2004
By: Yoshihiro
Reason: Update for XBOX 1.6 Eeprom
--------------------------------------------------------------------------------------------------------

*/


#include "xksha1.h"

XKSHA1::XKSHA1(void)
{}

XKSHA1::~XKSHA1(void)
{}


int XKSHA1::SHA1Reset(SHA1Context* context)
{
  if (!context)
  {
    return shaNull;
  }

  context->Length_Low = 0;
  context->Length_High = 0;
  context->Message_Block_Index = 0;

  context->Intermediate_Hash[0] = 0x67452301;
  context->Intermediate_Hash[1] = 0xEFCDAB89;
  context->Intermediate_Hash[2] = 0x98BADCFE;
  context->Intermediate_Hash[3] = 0x10325476;
  context->Intermediate_Hash[4] = 0xC3D2E1F0;

  context->Computed = 0;
  context->Corrupted = 0;

  return shaSuccess;
}


int XKSHA1::SHA1Result( SHA1Context* context, UCHAR Message_Digest[SHA1HashSize])
{
  int i;

  if (!context || !Message_Digest)
  {
    return shaNull;
  }

  if (context->Corrupted)
  {
    return context->Corrupted;
  }

  if (!context->Computed)
  {
    SHA1PadMessage(context);
    for (i = 0; i < 64; ++i)
    {
      /* message may be sensitive, clear it out*/
      context->Message_Block[i] = 0;
    }

    context->Length_Low = 0;    /* and clear length */
    context->Length_High = 0;
    context->Computed = 1;

  }

  for (i = 0; i < SHA1HashSize; ++i)
  {
    Message_Digest[i] = (UCHAR) ((context->Intermediate_Hash[i >> 2] >> 8 * (3 - (i & 0x03))) & 0xff);
  }

  return shaSuccess;
}

int XKSHA1::SHA1Input(SHA1Context* context, const UCHAR* message_array, unsigned int length)
{
  if (!length)
  {
    return shaSuccess;
  }

  if (!context || !message_array)
  {
    return shaNull;
  }

  if (context->Computed)
  {
    context->Corrupted = shaStateError;

    return shaStateError;
  }

  if (context->Corrupted)
  {
    return context->Corrupted;
  }

  while (length-- && !context->Corrupted)
  {
    context->Message_Block[context->Message_Block_Index++] = (*message_array & 0xFF);

    context->Length_Low += 8;

    if (context->Length_Low == 0)
    {
      context->Length_High++;
      if (context->Length_High == 0)
      {
        /* Message is too long */
        context->Corrupted = 1;
      }
    }

    if (context->Message_Block_Index == 64)
    {
      SHA1ProcessMessageBlock(context);
    }

    message_array++;
  }

  return shaSuccess;
}



void XKSHA1::SHA1ProcessMessageBlock(SHA1Context* context)
{
  const UINT32 K[] =
    {       /* Constants defined in SHA-1   */
      0x5A827999,
      0x6ED9EBA1,
      0x8F1BBCDC,
      0xCA62C1D6
    };
  int t;                 /* Loop counter                */
  UINT32 temp;              /* Temporary word value        */
  UINT32 W[80];             /* Word sequence               */
  UINT32 A, B, C, D, E;     /* Word buffers                */

  /*
   *  Initialize the first 16 words in the array W
   */
  for (t = 0; t < 16; t++)
  {
    W[t] = context->Message_Block[t * 4] << 24;
    W[t] |= context->Message_Block[t * 4 + 1] << 16;
    W[t] |= context->Message_Block[t * 4 + 2] << 8;
    W[t] |= context->Message_Block[t * 4 + 3];
  }

  for (t = 16; t < 80; t++)
  {
    W[t] = SHA1CircularShift(1, W[t - 3] ^ W[t - 8] ^ W[t - 14] ^ W[t - 16]);
  }

  A = context->Intermediate_Hash[0];
  B = context->Intermediate_Hash[1];
  C = context->Intermediate_Hash[2];
  D = context->Intermediate_Hash[3];
  E = context->Intermediate_Hash[4];

  for (t = 0; t < 20; t++)
  {
    temp = SHA1CircularShift(5, A) +
           ((B & C) | ((~B) & D)) + E + W[t] + K[0];
    E = D;
    D = C;
    C = SHA1CircularShift(30, B);

    B = A;
    A = temp;
  }

  for (t = 20; t < 40; t++)
  {
    temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[1];
    E = D;
    D = C;
    C = SHA1CircularShift(30, B);
    B = A;
    A = temp;
  }

  for (t = 40; t < 60; t++)
  {
    temp = SHA1CircularShift(5, A) +
           ((B & C) | (B & D) | (C & D)) + E + W[t] + K[2];
    E = D;
    D = C;
    C = SHA1CircularShift(30, B);
    B = A;
    A = temp;
  }

  for (t = 60; t < 80; t++)
  {
    temp = SHA1CircularShift(5, A) + (B ^ C ^ D) + E + W[t] + K[3];
    E = D;
    D = C;
    C = SHA1CircularShift(30, B);
    B = A;
    A = temp;
  }

  context->Intermediate_Hash[0] += A;
  context->Intermediate_Hash[1] += B;
  context->Intermediate_Hash[2] += C;
  context->Intermediate_Hash[3] += D;
  context->Intermediate_Hash[4] += E;

  context->Message_Block_Index = 0;
}


void XKSHA1::SHA1PadMessage(SHA1Context* context)
{
  /*
   *  Check to see if the current message block is too small to hold
   *  the initial padding bits and length.  If so, we will pad the
   *  block, process it, and then continue padding into a second
   *  block.
   */
  if (context->Message_Block_Index > 55)
  {
    context->Message_Block[context->Message_Block_Index++] = 0x80;
    while (context->Message_Block_Index < 64)
    {
      context->Message_Block[context->Message_Block_Index++] = 0;
    }

    SHA1ProcessMessageBlock(context);

    while (context->Message_Block_Index < 56)
    {
      context->Message_Block[context->Message_Block_Index++] = 0;
    }
  }
  else
  {
    context->Message_Block[context->Message_Block_Index++] = 0x80;
    while (context->Message_Block_Index < 56)
    {

      context->Message_Block[context->Message_Block_Index++] = 0;
    }
  }

  /*
   *  Store the message length as the last 8 octets
   */
  context->Message_Block[56] = (UCHAR) (context->Length_High >> 24);
  context->Message_Block[57] = (UCHAR) (context->Length_High >> 16);
  context->Message_Block[58] = (UCHAR) (context->Length_High >> 8);
  context->Message_Block[59] = (UCHAR) (context->Length_High & 0xff);
  context->Message_Block[60] = (UCHAR) (context->Length_Low >> 24);
  context->Message_Block[61] = (UCHAR) (context->Length_Low >> 16);
  context->Message_Block[62] = (UCHAR) (context->Length_Low >> 8);
  context->Message_Block[63] = (UCHAR) (context->Length_Low & 0xff);

  SHA1ProcessMessageBlock(context);
}


void XKSHA1::quick_SHA1( UCHAR* SHA1_result, ... )
{
  va_list args;
  struct SHA1Context context;

  va_start(args, SHA1_result);

  SHA1Reset(&context);

  while (1)
  {
    UCHAR* buffer = va_arg(args, UCHAR* );
    int length;

    if (buffer == NULL) break;

    length = va_arg(args, int);

    SHA1Input(&context, buffer, length);
  }

  SHA1Result(&context, SHA1_result);

  va_end(args);
}


void XKSHA1::HMAC_SHA1(UCHAR* result, UCHAR* key, int key_length, UCHAR* text1, int text1_length, UCHAR* text2, int text2_length)
{
  UCHAR state1[0x40];
  UCHAR state2[0x40 + 0x14];
  int i;

  for (i = 0x40 - 1; i >= key_length;--i) state1[i] = 0x36;
  for (;i >= 0;--i) state1[i] = key[i] ^ 0x36;

  quick_SHA1 ( &state2[0x40],
               state1, 0x40,
               text1, text1_length,
               text2, text2_length,
               NULL );

  for (i = 0x40 - 1; i >= key_length;--i) state2[i] = 0x5C;
  for (;i >= 0;--i) state2[i] = key[i] ^ 0x5C;

  quick_SHA1 ( result,
               state2, 0x40 + 0x14,
               NULL );
}


int XKSHA1::HMAC1Reset(int version,SHA1Context *context)
{
  SHA1Reset(context);
  switch (version)
  {
    case 9:
      context->Intermediate_Hash[0] = 0x85F9E51A;
      context->Intermediate_Hash[1] = 0xE04613D2;
      context->Intermediate_Hash[2] = 0x6D86A50C;
      context->Intermediate_Hash[3] = 0x77C32E3C;
      context->Intermediate_Hash[4] = 0x4BD717A4;
        break;
    case 10:
      context->Intermediate_Hash[0] = 0x72127625;
      context->Intermediate_Hash[1] = 0x336472B9;
      context->Intermediate_Hash[2] = 0xBE609BEA;
      context->Intermediate_Hash[3] = 0xF55E226B;
      context->Intermediate_Hash[4] = 0x99958DAC;
      break;
    case 11:
      context->Intermediate_Hash[0] = 0x39B06E79;
      context->Intermediate_Hash[1] = 0xC9BD25E8;
      context->Intermediate_Hash[2] = 0xDBC6B498;
      context->Intermediate_Hash[3] = 0x40B4389D;
      context->Intermediate_Hash[4] = 0x86BBD7ED;
      break;
    case 12:
      context->Intermediate_Hash[0] = 0x8058763a;
      context->Intermediate_Hash[1] = 0xf97d4e0e;
      context->Intermediate_Hash[2] = 0x865a9762;
      context->Intermediate_Hash[3] = 0x8a3d920d;
      context->Intermediate_Hash[4] = 0x08995b2c;
      break;
  }

  context->Length_Low = 512;

  return shaSuccess;
}

int XKSHA1::HMAC2Reset(int version,SHA1Context *context)
{
  SHA1Reset(context);
  switch (version)
  {
    case 9:
      context->Intermediate_Hash[0] = 0x5D7A9C6B;
      context->Intermediate_Hash[1] = 0xE1922BEB;
      context->Intermediate_Hash[2] = 0xB82CCDBC;
      context->Intermediate_Hash[3] = 0x3137AB34;
      context->Intermediate_Hash[4] = 0x486B52B3;
      break;
    case 10:
      context->Intermediate_Hash[0] = 0x76441D41;
      context->Intermediate_Hash[1] = 0x4DE82659;
      context->Intermediate_Hash[2] = 0x2E8EF85E;
      context->Intermediate_Hash[3] = 0xB256FACA;
      context->Intermediate_Hash[4] = 0xC4FE2DE8;
      break;
    case 11:
      context->Intermediate_Hash[0] = 0x9B49BED3;
      context->Intermediate_Hash[1] = 0x84B430FC;
      context->Intermediate_Hash[2] = 0x6B8749CD;
      context->Intermediate_Hash[3] = 0xEBFE5FE5;
      context->Intermediate_Hash[4] = 0xD96E7393;
      break;
    case 12:
      context->Intermediate_Hash[0] = 0x01075307;
      context->Intermediate_Hash[1] = 0xa2f1e037;
      context->Intermediate_Hash[2] = 0x1186eeea;
      context->Intermediate_Hash[3] = 0x88da9992;
      context->Intermediate_Hash[4] = 0x168a5609;
      break;
  }
  context->Length_Low  = 512;
  return shaSuccess;
}


void XKSHA1::XBOX_HMAC_SHA1(int version, UCHAR* result, ... )
{
  va_list args;
  struct SHA1Context context;
  va_start(args, result);

  HMAC1Reset(version, &context);
  while (1)
  {
    unsigned char *buffer = va_arg(args, unsigned char *);
    int length;

    if (buffer == NULL) break;
    length = va_arg(args, int);
    SHA1Input(&context, buffer, length);

  }
  va_end(args);

  SHA1Result(&context, &context.Message_Block[0]);

  HMAC2Reset(version, &context);

  SHA1Input(&context, &context.Message_Block[0], 0x14);

  SHA1Result(&context, result);
}
