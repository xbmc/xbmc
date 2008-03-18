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
**      XKGENERAL.H - General Utility and Helper function Class' Header
********************************************************************************************************
**
** This is the Class Header, see the .CPP file for more comments and implementation details.
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

#ifndef __GNUC__
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
#endif

#ifndef FILENAME_MAX
 #define FILENAME_MAX 260
#endif

class XKGeneral
{
protected:
#define HEX_VALUES {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'}

public:
  XKGeneral(void);
  ~XKGeneral(void);

  //Generic conversion functions..
  static void BytesToHexStr(LPBYTE SrcBytes, DWORD byteCount, LPSTR DstString);
  static void BytesToHexStr(LPBYTE SrcBytes, DWORD byteCount, LPSTR DstString, UCHAR Seperator);
  static void MixedStrToDecStr(LPSTR StringData, LPDWORD StrLen, CHAR Base, BOOL RemoveInvalid);
  static void HexStrToBytes(LPBYTE StringData, LPDWORD pBufferLen, BOOL RemoveInvalid);
  static DWORD HexStrToDWORD(LPBYTE StringData, LPDWORD pBufferLen, BOOL RemoveInvalid, BOOL FlipByteOrder);

  //Read items and values from a .INI file..
  static BOOL ReadINIFileItem(LPCSTR INIFileName, LPCSTR INISection, LPCSTR INIItem, LPSTR ItemValue, LPDWORD ValueLen);

  //Strip Quotes from a string..
  static void StripEnclosedChars(LPSTR sString, LPDWORD strLen, CHAR EncloseChar);
  static void StripQuotes(LPSTR sString, LPDWORD strLen);
};
