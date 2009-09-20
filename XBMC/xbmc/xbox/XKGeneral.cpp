#include <algorithm>
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
**      XKGENERAL.CPP - General Utility and Helper function Implementation      
********************************************************************************************************
**
** This Class contains mostly static helper functions to do all kinds of data/type conversions
** and read from INI files etc..  This is a work in progress and will continue to grow..
**
********************************************************************************************************
 
UPDATE LOG:
--------------------------------------------------------------------------------------------------------
Date: 02/18/2003
By: UNDEAD [team-assembly]
Reason: Prepared 0.2 for Public Release
--------------------------------------------------------------------------------------------------------
Date: 01/08/2003
By: UNDEAD [team-assembly]
Reason: Fixed two Memory leaks  - Thanks to tommyhl for spotting it!!!
--------------------------------------------------------------------------------------------------------
Date: 01/06/2003
By: UNDEAD [team-assembly]
Reason: Prepared for Public Release
--------------------------------------------------------------------------------------------------------
 
*/


#include "XKGeneral.h"


XKGeneral::XKGeneral(void)
{}

XKGeneral::~XKGeneral(void)
{}

//This Funciton converts BYTES to a String representation of the HEX value of the Bytes
void XKGeneral::BytesToHexStr(LPBYTE SrcBytes, DWORD byteCount, LPSTR DstString)
{
  BytesToHexStr(SrcBytes, byteCount, DstString, 0x00);
}

//This Funciton converts BYTES to a String representation of the HEX value of the Bytes
//Includes a paramter to specify a seperator or Field delimiter between Hex BYTE values
void XKGeneral::BytesToHexStr(LPBYTE SrcBytes, DWORD byteCount, LPSTR DstString, UCHAR Seperator)
{
  USHORT Inc = (Seperator == 0x00) ? 2 : 3;
  ULONG i;
  for (i = 0; i < byteCount; i++)
  {
    BYTE nybble = (SrcBytes[i] >> 4) & 0x0f;
    if (nybble > 9)
      nybble += ('A'-0x0A); // change for lowercase 'a' to make lowercase
    else
      nybble += '0';
    DstString[i*Inc] = nybble;

    nybble = (SrcBytes[i]) & 0x0f;
    if (nybble > 9)
      nybble += ('A'-0x0A); // change for lowercase 'a' to make lowercase
    else
      nybble += '0';
    DstString[i*Inc + 1] = nybble;
  }

  if (Inc == 3)
    DstString[i*Inc - 1] = 0;
  else
    DstString[i*Inc] = 0;

  if (Seperator != 0x00)
  {
    for (ULONG i = 1; i < byteCount; i++)
      *(DstString + i*Inc - 1) = Seperator;
  }

}

//This Function converts a String that Contains anyting to into
//a String that containts ONLY valid numerical values..
//Specify the base, eg. Hex = 16, Decimal=10 etc.
//RemoveInvalid indacates if invalid values should be Zero or removed..
void XKGeneral::MixedStrToDecStr(LPSTR StringData, LPDWORD StrLen, CHAR Base, BOOL RemoveInvalid)
{
  UCHAR DecChars[16] = HEX_VALUES;

  DWORD dwSize = *StrLen;
  DWORD currentOffset = 0;
  LPSTR sData = new CHAR[dwSize + 1];
  ZeroMemory(sData, dwSize + 1);
  strncpy(sData, StringData, dwSize);

  if (!RemoveInvalid)
    memset(StringData, '0', dwSize);
  else
    ZeroMemory(StringData, dwSize);

  strupr(sData);

  for (DWORD i = 0; i < dwSize; i++)
  {
    LPCSTR ChrOffset = strchr((LPCSTR) & DecChars, *(sData + i));
    if ((ChrOffset != NULL) && ((ChrOffset - (LPSTR)&DecChars) <= Base - 1))
    {
      memcpy(StringData + currentOffset, ChrOffset, 1);
      currentOffset++;
    }
    else if (!RemoveInvalid)
    {
      currentOffset++;
    }
  }

  delete[] sData;
  *StrLen = (DWORD)strlen(StringData);

}

DWORD XKGeneral::HexStrToDWORD(LPBYTE StringData, LPDWORD pBufferLen, BOOL RemoveInvalid, BOOL FlipByteOrder)
{
  DWORD retVal = 0;
  BYTE temp[4];


  HexStrToBytes(StringData, pBufferLen, RemoveInvalid);

  if (FlipByteOrder)
  {
    memcpy(temp, StringData, 4);
    StringData[3] = temp[0];
    StringData[2] = temp[1];
    StringData[1] = temp[2];
    StringData[0] = temp[3];
  }

  retVal = *((LPDWORD) StringData);
  return retVal;
}

//This Funciton converts String representation of the HEX value into BYTES
//RemoveInvalid indacates if invalid values should be Zero or removed..
void XKGeneral::HexStrToBytes(LPBYTE StringData, LPDWORD pBufferLen, BOOL RemoveInvalid)
{
  DWORD dwSize = *pBufferLen;
  LPSTR sData = new CHAR[dwSize + 1];

  ZeroMemory(sData, dwSize + 1);
  strncpy(sData, (LPCSTR)StringData, dwSize);
  ZeroMemory(StringData, dwSize);

  MixedStrToDecStr(sData, &dwSize, 16, RemoveInvalid);

  for (DWORD i = 0; i < dwSize / 2; i++)
  {
    UCHAR tmpstr[3];
    ZeroMemory(tmpstr, 3);
    memcpy(tmpstr, (LPCSTR)(sData + (i*2)), 2);
    memset(StringData + i, (UCHAR)strtol((LPCSTR)&tmpstr, NULL, 16), 1);
  }

  delete[] sData;
  *pBufferLen = dwSize / 2;
}

//This Funciton can read from an INI file and parse out certain values from the file
//
// Example .INI file can contain:
//
// [SECTION_MAIN]
// Value1 = "My Value"
//
BOOL XKGeneral::ReadINIFileItem(LPCSTR INIFileName, LPCSTR INISection, LPCSTR INIItem, LPSTR ItemValue, LPDWORD ValueLen)
{
  BOOL retVal = FALSE;
  LONG retSize = 0;

  HANDLE iniFile = CreateFile(INIFileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

  if (iniFile != INVALID_HANDLE_VALUE)
  {
    LPCSTR sTempSection = NULL;
    LPCSTR sNextSection = NULL;
    LPCSTR sTempItem = NULL;
    ULONG sectionOffset = 0;
    DWORD filesize = GetFileSize(iniFile, NULL);
    BOOL foundSection = FALSE, foundItem = FALSE;

    LPBYTE iniData = new BYTE[filesize + 2];
    ZeroMemory(iniData, filesize + 2);
    memset(iniData, '\n', filesize + 2); //This is a cheezy trick to help not to search past end of the file..

    ReadFile(iniFile, iniData, filesize, &filesize, NULL);

    do
    {
      sTempSection = (char *)strchr((LPCSTR)iniData + sectionOffset, '[');
      if ((sTempSection != NULL) && (strncmp(sTempSection + 1, INISection, strlen(INISection)) == 0))
      {
        sNextSection = (char *)strchr((LPCSTR)sTempSection + strlen(INISection), '[');
        sNextSection = (sNextSection != NULL) ? sNextSection : (LPSTR)iniData + filesize;

        foundSection = TRUE;
        break;
      }
      sectionOffset = (ULONG)((LPBYTE)sTempSection - iniData) + 1;
    }
    while (sTempSection != NULL);

    if (foundSection)
      for (LPCSTR finditem = sTempSection + strlen(INISection) + 2; finditem < sNextSection - 1; finditem = std::min((LPCSTR)strchr(finditem, '\n') + 1, sNextSection))
      {
        LPCSTR snextitem = std::min((LPCSTR)strchr(finditem, '\n') + 1, sNextSection);
        LPCSTR spossible = strchr(finditem, *INIItem);
        LPCSTR scomment = strchr(finditem, '#');

        if ((spossible != NULL) && (scomment != NULL) && (scomment < spossible))
          continue; //found a Commented Line..

        if ((spossible != NULL) && (spossible < snextitem))
          finditem = spossible; //found possible match

        //Check if its a sure match...
        if (strncmp(finditem, INIItem, strlen(INIItem)) == 0)
        {
          foundItem = TRUE;

          sTempItem = strchr(finditem + strlen(INIItem), '=') + 1;
          LPCSTR sEnd = std::min((LPCSTR)strchr(sTempItem, '\n') - 1, sNextSection);
          LONG sLen = (LONG)(sEnd - sTempItem);
          LONG sLen2 = (LONG)std::min((LONG)strlen(sTempItem), sLen);

          if (*ValueLen >= (DWORD)sLen2)
          {
            ULONG startpos = 0;
            ULONG endpos = (ULONG)sLen2;

            //Do LEFT and RIGHT trim....
            while ((*sTempItem + startpos == ' ') && (startpos < endpos))
              startpos++;

            while ((*sTempItem + endpos == ' ') && (endpos > startpos))
              endpos--;


            ZeroMemory(ItemValue, sLen2 + 1);
            strncpy(ItemValue, sTempItem + startpos, sLen2 - startpos - (sLen2 - endpos));

          }
          else
          {
            foundSection = FALSE;
            foundItem = FALSE;
          }
          retSize = (LONG)std::min((LONG)strlen(sTempItem), sLen);

          break;
        }
      }

    retVal = foundSection & foundItem;
    delete[] iniData;
    CloseHandle(iniFile);
  }

  *ValueLen = retSize;
  return retVal;
}

//Strip quotes from mostly File Paths..  hence FILENAME_MAX
void XKGeneral::StripQuotes(LPSTR sString, LPDWORD strLen)
{
  StripEnclosedChars(sString, strLen, '\"');
}

//This Function simply strips out Characters from a enclosed string..
//Especially if reading from INI file and value is enlcosed with quotes..
void XKGeneral::StripEnclosedChars(LPSTR sString, LPDWORD strLen, CHAR EncloseChar)
{
  UCHAR tmpString[FILENAME_MAX]; //Work off Max Filename.. should be big enough..
  ZeroMemory(tmpString, FILENAME_MAX);

  //get string to work with..
  strcpy((LPSTR)tmpString, sString);
  ZeroMemory(sString, *strLen);

  LPCSTR tmpFirst = strchr((LPCSTR)tmpString, EncloseChar);
  LPCSTR tmpLast = strrchr((LPCSTR)tmpString, EncloseChar);

  //check if Null, if null then just beginning of string..
  tmpFirst = (tmpFirst != NULL) ? tmpFirst + 1 : (LPSTR)tmpString;

  //check if Null, if null then just use end of string..
  tmpLast = (tmpLast != NULL) ? tmpLast : (LPSTR)tmpString + *strLen;

  *strLen = (LONG)(tmpLast - tmpFirst);

  if (tmpFirst != NULL)
    strncpy(sString, tmpFirst, *strLen);
}
