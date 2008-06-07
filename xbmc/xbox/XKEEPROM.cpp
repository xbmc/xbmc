/*********************************
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
**       XKEEPROM.CPP - XBOX EEPROM Class' Implementation
********************************************************************************************************
**
**  This Class encapsulates the XBOX EEPROM strucure and many helper functions to parse,
**  Calculate CRC's and Decrypt various values in the XBOX EEPROM..
**
********************************************************************************************************
********************************************************************************************************
**  CREDITS:
********************************************************************************************************
**  XBOX-LINUX TEAM:
**  ---------------
**    Wow, you guys are awsome !!  I bow down to your greatness !!  the "Friday 13th" Middle
**    Message Hack really saved our butts !!
**    REFERENCE URL:  http://xbox-linux.sourceforge.net
**
********************************************************************************************************
UPDATE LOG:
--------------------------------------------------------------------------------------------------------
Date: 11/27/2004
By: Yoshihiro
Reason: Update for xbox 1.6
--------------------------------------------------------------------------------------------------------
Date: 02/18/2003
By: UNDEAD [team-assembly]
Reason: Prepared 0.2 for Public Release
--------------------------------------------------------------------------------------------------------
Date: 01/25/2003
By: UNDEAD [team-assembly]
Reason: Added XBOX Specific code to read EEPROM Data from Hardware
--------------------------------------------------------------------------------------------------------
Date: 01/06/2003
By: UNDEAD [team-assembly]
Reason: Prepared for Public Release
--------------------------------------------------------------------------------------------------------*/

#include "stdafx.h"
#include "xkeeprom.h"
#include <stdio.h>


/* Default Constructor using a Blank eeprom image... */
XKEEPROM::XKEEPROM()
{
  m_XBOX_Version = V1_0;
  ZeroMemory(&m_EEPROMData, sizeof(EEPROMDATA));
  m_EncryptedState = FALSE;
}

/* Constructor to specify a eeprom image to use ... */
XKEEPROM::XKEEPROM(LPEEPROMDATA pEEPROMData, BOOL Encrypted)
{
  m_XBOX_Version = V_NONE;
  memcpy(&m_EEPROMData, (LPBYTE)pEEPROMData, sizeof(EEPROMDATA));
  m_EncryptedState = Encrypted;
}
/* Default Destructor */
XKEEPROM::~XKEEPROM(void)
{}
/* Read a EEPROM image from a .BIN file.. */
/* could be a decrypted or Enrytped.. make sure you specify correct value */
BOOL XKEEPROM::ReadFromBINFile(LPCSTR FileName, BOOL IsEncrypted)
{
  //First Make sure the File exists...
  WIN32_FIND_DATA wfd;
  HANDLE hf = FindFirstFile(FileName, &wfd);
  if (hf == INVALID_HANDLE_VALUE)
    return FALSE;

  //Now Read the EEPROM Data from File..
  BYTE Data[EEPROM_SIZE];
  DWORD dwBytesRead = 0;
  ZeroMemory(Data, EEPROM_SIZE);

  hf = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
  BOOL retVal = ReadFile(hf, Data, EEPROM_SIZE, &dwBytesRead, NULL);
  if (retVal && (dwBytesRead >= EEPROM_SIZE))
  {
    memcpy(&m_EEPROMData, Data, EEPROM_SIZE);
    m_EncryptedState = IsEncrypted;
  }
  else
    retVal = FALSE;

  CloseHandle(hf);

  return retVal;
}

/* Write the Current EEPROM image to a .BIN file.. */
BOOL XKEEPROM::WriteToBINFile(LPCSTR FileName)
{
  BOOL retVal = FALSE;
  DWORD dwBytesWrote = 0;

  // if not encrypted, encrypt before we write the image.
  if (!m_EncryptedState)
    if (!EncryptAndCalculateCRC())
      return FALSE;

  HANDLE hf = CreateFile(FileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hf !=  INVALID_HANDLE_VALUE)
  {
    //Write EEPROM File
    retVal = WriteFile(hf , &m_EEPROMData, EEPROM_SIZE, &dwBytesWrote, NULL);
    CloseHandle(hf);
  }
  else
    retVal = FALSE;
  return retVal;
}

/* Read from a .CFG File  ans set Current EEPROM Data */
/* Encrypt as V1.0 by default  */
BOOL XKEEPROM::ReadFromCFGFile(LPCSTR FileName)
{
  //First Make sure the File exists...
  BOOL retVal = FALSE;
  WIN32_FIND_DATA wfd;
  HANDLE hf = FindFirstFile(FileName, &wfd);
  if (hf == INVALID_HANDLE_VALUE)
    return FALSE;

  //Now Clear the EEPROM Data and read from File..
  BYTE tmpData[EEPROM_SIZE];
  DWORD tmpLen = EEPROM_SIZE;

  if (hf != INVALID_HANDLE_VALUE)
  {
    //Get Confounder
    tmpLen = EEPROM_SIZE;
    ZeroMemory(tmpData, tmpLen);
    ZeroMemory(&m_EEPROMData, EEPROM_SIZE);
    m_EncryptedState = FALSE;

    if (XKGeneral::ReadINIFileItem(FileName, "EEPROMDATA", "Confounder",(LPSTR) &tmpData, &tmpLen))
    {
      XKGeneral::StripQuotes((LPSTR)tmpData, &tmpLen);
      XKGeneral::HexStrToBytes(tmpData, (LPDWORD)&tmpLen, TRUE);

      memcpy(m_EEPROMData.Confounder, tmpData, 8);
    }

    //Get HDDKey
    tmpLen = EEPROM_SIZE;
    ZeroMemory(tmpData, tmpLen);
    if (XKGeneral::ReadINIFileItem(FileName, "EEPROMDATA", "HDDKey",(LPSTR) &tmpData, &tmpLen))
    {
      XKGeneral::StripQuotes((LPSTR)tmpData, &tmpLen);
      XKGeneral::HexStrToBytes(tmpData, (LPDWORD)&tmpLen, TRUE);

      memcpy(m_EEPROMData.HDDKey, tmpData, 16);
    }

    //Get XBERegion
    tmpLen = EEPROM_SIZE;
    ZeroMemory(tmpData, tmpLen);
    if (XKGeneral::ReadINIFileItem(FileName, "EEPROMDATA", "XBERegion",(LPSTR) &tmpData, &tmpLen))
    {
      XKGeneral::StripQuotes((LPSTR)tmpData, &tmpLen);
      XKGeneral::HexStrToBytes(tmpData, (LPDWORD)&tmpLen, TRUE);

      memcpy(m_EEPROMData.XBERegion, tmpData, 1);
    }


    //Get Online Key
    tmpLen = EEPROM_SIZE;
    ZeroMemory(tmpData, tmpLen);
    if (XKGeneral::ReadINIFileItem(FileName, "EEPROMDATA", "OnlineKey",(LPSTR) &tmpData, &tmpLen))
    {
      XKGeneral::StripQuotes((LPSTR)tmpData, &tmpLen);
      XKGeneral::HexStrToBytes(tmpData, (LPDWORD)&tmpLen, TRUE);

      memcpy(m_EEPROMData.OnlineKey, tmpData, 16);
    }


    //Get SerialNumber
    tmpLen = EEPROM_SIZE;
    ZeroMemory(tmpData, tmpLen);
    if (XKGeneral::ReadINIFileItem(FileName, "EEPROMDATA", "XBOXSerial",(LPSTR) &tmpData, &tmpLen))
    {
      XKGeneral::StripQuotes((LPSTR)tmpData, &tmpLen);
      tmpLen=12;
      XKGeneral::MixedStrToDecStr((LPSTR)tmpData, (LPDWORD)&tmpLen, 10, FALSE);
      memcpy(m_EEPROMData.SerialNumber, tmpData, 12);
    }

    //Get MAC Address
    tmpLen = EEPROM_SIZE;
    ZeroMemory(tmpData, tmpLen);
    if (XKGeneral::ReadINIFileItem(FileName, "EEPROMDATA", "XBOXMAC",(LPSTR) &tmpData, &tmpLen))
    {
      XKGeneral::StripQuotes((LPSTR)tmpData, &tmpLen);
      XKGeneral::HexStrToBytes(tmpData, (LPDWORD)&tmpLen, TRUE);

      memcpy(m_EEPROMData.MACAddress, tmpData, 6);
    }


    //Get Video Mode
    tmpLen = EEPROM_SIZE;
    ZeroMemory(tmpData, tmpLen);
    if (XKGeneral::ReadINIFileItem(FileName, "EEPROMDATA", "VideoMode",(LPSTR) &tmpData, &tmpLen))
    {
      XKGeneral::StripQuotes((LPSTR)tmpData, &tmpLen);
      UINT* tmpVM = (UINT*) &m_EEPROMData.VideoStandard;

      if (strcmp((LPCSTR)tmpData, "NTSC") == 0)
        *tmpVM = NTSC_M;
      else if (strcmp((LPCSTR)tmpData, "PAL") == 0)
        *tmpVM = PAL_I;
      else
        *tmpVM = NTSC_M;
    }

    if (m_XBOX_Version == V_NONE)
      EncryptAndCalculateCRC(V1_0); //Use default V1.0
    else
      EncryptAndCalculateCRC();

    retVal = TRUE;
  }

  return retVal;
}

/* Write the Current EEPROM Data to a .CFG File  */
/* If it is encrypted you have to decrypt it first ! */
BOOL XKEEPROM::WriteToCFGFile(LPCSTR FileName)
{
  BOOL retVal = FALSE;
  DWORD dwBytesWrote = 0;
  CHAR tmpData[256];
  ZeroMemory(tmpData, 256);
  DWORD tmpSize = 256;

  //Check if this is currently an encrypted image.. if it is, then decrypt it first..
  BOOL oldEncryptedState = m_EncryptedState;
  if (m_EncryptedState)
    Decrypt();

  HANDLE hf = CreateFile(FileName, GENERIC_WRITE, FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hf !=  INVALID_HANDLE_VALUE)
  {
    //Write CFG File Header..
    LPSTR fHeaderInfo = "#Please note ALL fields and Values are Case Sensitive !!\r\n\r\n[EEPROMDATA]\r\n";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);

    //Write Serial Number
    fHeaderInfo = "XBOXSerial\t= \"";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    WriteFile(hf, m_EEPROMData.SerialNumber, SERIALNUMBER_SIZE, &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);


    //Write MAC Address..
    fHeaderInfo = "XBOXMAC\t\t= \"";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    ZeroMemory(tmpData, tmpSize);
    XKGeneral::BytesToHexStr(m_EEPROMData.MACAddress, MACADDRESS_SIZE, tmpData, ':');
    strupr(tmpData);
    WriteFile(hf, tmpData, (DWORD)strlen(tmpData), &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);


    //Write Online Key ..
    fHeaderInfo = "\r\nOnlineKey\t= \"";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    ZeroMemory(tmpData, tmpSize);
    XKGeneral::BytesToHexStr(m_EEPROMData.OnlineKey, ONLINEKEY_SIZE, tmpData, ':');
    strupr(tmpData);
    WriteFile(hf, tmpData, (DWORD)strlen(tmpData), &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);


    //Write VideoMode ..
    fHeaderInfo = "\r\n#ONLY Use NTSC or PAL for VideoMode\r\n";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    fHeaderInfo = "VideoMode\t= \"";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    VIDEO_STANDARD vdo = GetVideoStandardVal();
    if (vdo == PAL_I)
      fHeaderInfo = "PAL";
    else
      fHeaderInfo = "NTSC";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);

    //Write XBE Region..
    fHeaderInfo = "\r\n#ONLY Use 01, 02 or 04 for XBE Region\r\n";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    fHeaderInfo = "XBERegion\t= \"";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    ZeroMemory(tmpData, tmpSize);
    XKGeneral::BytesToHexStr(m_EEPROMData.XBERegion, XBEREGION_SIZE, tmpData, 0x00);
    strupr(tmpData);
    WriteFile(hf, tmpData, (DWORD)strlen(tmpData), &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);

    //Write HDDKey..
    fHeaderInfo = "\r\nHDDKey\t\t= \"";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    ZeroMemory(tmpData, tmpSize);
    XKGeneral::BytesToHexStr(m_EEPROMData.HDDKey, HDDKEY_SIZE, tmpData, ':');
    strupr(tmpData);
    WriteFile(hf, tmpData, (DWORD)strlen(tmpData), &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);

    //Write Confounder..
    fHeaderInfo = "Confounder\t= \"";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);
    ZeroMemory(tmpData, tmpSize);
    XKGeneral::BytesToHexStr(m_EEPROMData.Confounder, CONFOUNDER_SIZE, tmpData, ':');
    strupr(tmpData);
    WriteFile(hf, tmpData, (DWORD)strlen(tmpData), &dwBytesWrote, NULL);
    fHeaderInfo = "\"\r\n";
    WriteFile(hf, fHeaderInfo, (DWORD)strlen(fHeaderInfo), &dwBytesWrote, NULL);

    retVal = TRUE;
  }

  CloseHandle(hf);

  //Check if this is was an encrypted image.. if it was, then re-encrypt it..
  if (oldEncryptedState)
    EncryptAndCalculateCRC();

  return retVal;

}

//very XBOX specific funtions to read/write EEPROM from hardware
#if defined (_XBOX)
void XKEEPROM::ReadFromXBOX()
{
  XKUtils::ReadEEPROMFromXBOX((LPBYTE)&m_EEPROMData);
  m_EncryptedState = TRUE;
}

void XKEEPROM::WriteToXBOX()
{
  //if we are writing the EEPROM to the XBOX, make sure it is encrypted first!
  if (!m_EncryptedState) {
    //if the EEPROM is not encrypted and we failed to encrypt,
    //bail out before we do any damage
    if (!EncryptAndCalculateCRC())
      return;
  }
  XKUtils::WriteEEPROMToXBOX((LPBYTE)&m_EEPROMData);
}
#endif

/* Return EEPROM data for this object, Check if it is Encrypted with IsEncrypted() */
void XKEEPROM::GetEEPROMData(LPEEPROMDATA pEEPROMData)
{
  memcpy(pEEPROMData, &m_EEPROMData, EEPROM_SIZE);
}

/* Set a Decrypted EEPROM image as EEPROM data for this object */
void XKEEPROM::SetDecryptedEEPROMData(XBOX_VERSION Version, LPEEPROMDATA pEEPROMData)
{
  memcpy(&m_EEPROMData, pEEPROMData, EEPROM_SIZE);
  m_EncryptedState = FALSE;
  m_XBOX_Version = Version;
}

/* Set a encrypted EEPROM image as EEPROM data for this object */
void XKEEPROM::SetEncryptedEEPROMData(LPEEPROMDATA pEEPROMData)
{
  memcpy(&m_EEPROMData, pEEPROMData, EEPROM_SIZE);
  m_EncryptedState = TRUE;
}

/* Get current detected XBOX Version for this EEPROM */
XBOX_VERSION XKEEPROM::GetXBOXVersion()
{
  return m_XBOX_Version;
}

/* Get confounder in the form of BYTES in  a  Hex String representation */
void XKEEPROM::GetConfounderString(LPSTR Confounder)
{
  //Check if this is currently an encrypted image.. if it is, then decrypt it first..
  BOOL oldEncryptedState = m_EncryptedState;
  if (m_EncryptedState)
    Decrypt();

  XKGeneral::BytesToHexStr((LPBYTE)&m_EEPROMData.Confounder, CONFOUNDER_SIZE, Confounder);

  //Check if this is was an encrypted image.. if it was, then re-encrypt it..
  if (oldEncryptedState)
    EncryptAndCalculateCRC();
}

/* Set Confounder in the form of BYTES in  a  Hex String representation */
void XKEEPROM::SetConfounderString(LPCSTR Confounder)
{
  const char* Confounder2 = "4c7033cb5bb597d2";
  DWORD len = CONFOUNDER_SIZE * 2;
  BYTE tmpData[(CONFOUNDER_SIZE * 2) + 1];
  ZeroMemory(tmpData, len + 1);
  memcpy(tmpData, Confounder2, min(strlen(Confounder2), len));

  XKGeneral::HexStrToBytes(tmpData, &len, TRUE);

  //Check if this is currently an encrypted image.. if it is, then decrypt it first..
  BOOL oldEncryptedState = m_EncryptedState;
  if (m_EncryptedState)
    Decrypt();

    memcpy(&m_EEPROMData.Confounder, tmpData, CONFOUNDER_SIZE);

  //Check if this is was an encrypted image.. if it was, then re-encrypt it..
  if (oldEncryptedState)
    EncryptAndCalculateCRC();
}

/* Set HDD Key in the form of BYTES in  a  Hex String representation */
void XKEEPROM::GetHDDKeyString(LPSTR HDDKey)
{
  //Check if this is currently an encrypted image.. if it is, then decrypt it first..
  BOOL oldEncryptedState = m_EncryptedState;
  if (m_EncryptedState)
    Decrypt();

  XKGeneral::BytesToHexStr((LPBYTE)&m_EEPROMData.HDDKey, HDDKEY_SIZE, HDDKey);

  //Check if this is was an encrypted image.. if it was, then re-encrypt it..
  if (oldEncryptedState)
    EncryptAndCalculateCRC();
}

/* Set HDD Key in the form of BYTES in  a  Hex String representation */
void XKEEPROM::SetHDDKeyString(LPCSTR HDDKey)
{
  DWORD len = HDDKEY_SIZE * 2;
  BYTE tmpData[(HDDKEY_SIZE * 2) + 1];
  ZeroMemory(tmpData, len + 1);
  memcpy(tmpData, HDDKey, min(strlen(HDDKey), len));

  XKGeneral::HexStrToBytes(tmpData, &len, TRUE);

  //Check if this is currently an encrypted image.. if it is, then decrypt it first..
  BOOL oldEncryptedState = m_EncryptedState;
  if (m_EncryptedState)
    Decrypt();

    memcpy(&m_EEPROMData.HDDKey, tmpData, HDDKEY_SIZE);

  //Check if this is was an encrypted image.. if it was, then re-encrypt it..
  if (oldEncryptedState)
    EncryptAndCalculateCRC();

}


/* Get XBE Region in the form of BYTES in  a  Hex String representation */
void XKEEPROM::GetXBERegionString(LPSTR XBERegion)
{
  //Check if this is currently an encrypted image.. if it is, then decrypt it first..
  BOOL oldEncryptedState = m_EncryptedState;
  if (m_EncryptedState)
    Decrypt();

  XKGeneral::BytesToHexStr((LPBYTE)&m_EEPROMData.XBERegion, XBEREGION_SIZE, XBERegion);

  //Check if this is was an encrypted image.. if it was, then re-encrypt it..
  if (oldEncryptedState)
    EncryptAndCalculateCRC();
}

/* Set XBE Region in the form of BYTES in  a  Hex String representation */
void XKEEPROM::SetXBERegionString(LPCSTR XBERegion)
{
  DWORD len = XBEREGION_SIZE * 2;
  BYTE tmpData[(XBEREGION_SIZE * 2) + 1];
  ZeroMemory(tmpData, len + 1);
  memcpy(tmpData, XBERegion, min(strlen(XBERegion), len));

  XKGeneral::HexStrToBytes(tmpData, &len, TRUE);

  //Check if this is currently an encrypted image.. if it is, then decrypt it first..
  BOOL oldEncryptedState = m_EncryptedState;
  if (m_EncryptedState)
    Decrypt();

    memcpy(&m_EEPROMData.XBERegion, tmpData, XBEREGION_SIZE);

  //Check if this is was an encrypted image.. if it was, then re-encrypt it..
  if (oldEncryptedState)
    EncryptAndCalculateCRC();
}

/* Set XBE Region using Enum Value */
void XKEEPROM::SetXBERegionVal(XBE_REGION RegionVal)
{
  //Check if this is currently an encrypted image.. if it is, then decrypt it first..
  BOOL oldEncryptedState = m_EncryptedState;
  if (m_EncryptedState)
    Decrypt();

    switch (RegionVal)
    {
      case(NORTH_AMERICA):
      case(JAPAN):
      case(EURO_AUSTRALIA):
      {
        m_EEPROMData.XBERegion[0] = RegionVal; //Only use first byte of Region...
        break;
      }
      default:
        m_EEPROMData.XBERegion[0] = NORTH_AMERICA; //If invalid,use Default of US
    }

  //Check if this is was an encrypted image.. if it was, then re-encrypt it..
  if (oldEncryptedState)
    EncryptAndCalculateCRC();
}

/* Get XBE Region as Enum Value */
XBE_REGION XKEEPROM::GetXBERegionVal()
{
  //Check if this is currently an encrypted image.. if it is, then decrypt it first..
  BOOL oldEncryptedState = m_EncryptedState;
  if (m_EncryptedState)
    Decrypt();

  XBE_REGION retVal = XBE_REGION (m_EEPROMData.XBERegion[0]);

  //Check if this is was an encrypted image.. if it was, then re-encrypt it..
  if (oldEncryptedState)
    EncryptAndCalculateCRC();

  return retVal;
}

/* Get Serial Number in the form of Decimal String representation */
void XKEEPROM::GetSerialNumberString(LPSTR SerialNumber)
{
  strncpy(SerialNumber, (LPSTR)&m_EEPROMData.SerialNumber, SERIALNUMBER_SIZE);
}

/* Set Serial Number in the form of Decimal String representation */
void XKEEPROM::SetSerialNumberString(LPCSTR SerialNumber)
{
  DWORD len = SERIALNUMBER_SIZE;
  strncpy((LPSTR)&m_EEPROMData.SerialNumber, SerialNumber, len);

  CalculateChecksum2();
}

/* Get MAC Address in the form of BYTES in  a  Hex String representation */
void XKEEPROM::GetMACAddressString(LPSTR MACAddress, UCHAR Seperator)
{
  XKGeneral::BytesToHexStr((LPBYTE)&m_EEPROMData.MACAddress, MACADDRESS_SIZE, MACAddress, Seperator);
}

/* Set MAC Address in the form of BYTES in  a  Hex String representation */
void XKEEPROM::SetMACAddressString(LPCSTR MACAddress)
{
  DWORD len = MACADDRESS_SIZE * 3;
  BYTE tmpData[(HDDKEY_SIZE * 3) + 1];
  ZeroMemory(tmpData, len + 1);
  memcpy(tmpData, MACAddress, min(strlen(MACAddress), len));

  XKGeneral::HexStrToBytes(tmpData, &len, TRUE);

  memcpy(&m_EEPROMData.MACAddress, tmpData, MACADDRESS_SIZE);

  CalculateChecksum2();

}

/* Get Online Key in the form of BYTES in  a  Hex String representation */
void XKEEPROM::GetOnlineKeyString(LPSTR OnlineKey)
{
  XKGeneral::BytesToHexStr((LPBYTE)&m_EEPROMData.OnlineKey, ONLINEKEY_SIZE, OnlineKey);
}


/* Set Online Key in the form of BYTES in  a  Hex String representation */
void XKEEPROM::SetOnlineKeyString(LPCSTR OnlineKey)
{
  DWORD len = ONLINEKEY_SIZE * 2;
  BYTE tmpData[(HDDKEY_SIZE * 2) + 1];
  ZeroMemory(tmpData, len + 1);
  memcpy(tmpData, OnlineKey, min(strlen(OnlineKey), len));

  XKGeneral::HexStrToBytes(tmpData, &len, TRUE);

  memcpy(&m_EEPROMData.OnlineKey, tmpData, ONLINEKEY_SIZE);

  CalculateChecksum2();
}

/* Get DVD Region in the form of BYTES in  a  Hex String representation */
void XKEEPROM::GetDVDRegionString(LPSTR DVDRegion)
{
  XKGeneral::BytesToHexStr((LPBYTE)&m_EEPROMData.DVDPlaybackKitZone, DVDREGION_SIZE, DVDRegion);
}

/* Set DVD Region in the form of BYTES in  a  Hex String representation */
void XKEEPROM::SetDVDRegionString(LPCSTR DVDRegion)
{
  DWORD len = DVDREGION_SIZE * 2;
  BYTE tmpData[(DVDREGION_SIZE * 2) + 1];
  ZeroMemory(tmpData, len + 1);
  memcpy(tmpData, DVDRegion, min(strlen(DVDRegion), len));

  XKGeneral::HexStrToBytes(tmpData, &len, TRUE);

  memcpy(&m_EEPROMData.DVDPlaybackKitZone, tmpData, DVDREGION_SIZE);

  CalculateChecksum3();

}

/*Set Video Standard with Enum */
void XKEEPROM::SetDVDRegionVal(DVD_ZONE ZoneVal)
{
    switch (ZoneVal)
    {
      case (ZONE1):
      case (ZONE2):
      case (ZONE3):
      case (ZONE4):
      case (ZONE5):
      case (ZONE6):
      {
        m_EEPROMData.DVDPlaybackKitZone[0] = ZoneVal; //Only use first byte of Region...
        break;
      }
      default:
        m_EEPROMData.DVDPlaybackKitZone[0] = ZONE_NONE; //If invalid,use Default of US
    }

    CalculateChecksum3();
}


/*Get DVD Region as Enum */
DVD_ZONE XKEEPROM::GetDVDRegionVal()
{
  DVD_ZONE retVal = DVD_ZONE (m_EEPROMData.DVDPlaybackKitZone[0]);

  return retVal;
}

/* Get Video Standard in the form of BYTES in  a  Hex String representation */
void XKEEPROM::GetVideoStandardString(LPSTR VideoStandard)
{
  XKGeneral::BytesToHexStr((LPBYTE)&m_EEPROMData.VideoStandard, VIDEOSTANDARD_SIZE, VideoStandard);
}

/* Set Video Standard in the form of BYTES in  a  Hex String representation */
void XKEEPROM::SetVideoStandardString(LPCSTR VideoStandard)
{
  DWORD len = VIDEOSTANDARD_SIZE * 2;
  BYTE tmpData[(VIDEOSTANDARD_SIZE * 2) + 1];
  ZeroMemory(tmpData, len + 1);
  memcpy(tmpData, VideoStandard, min(strlen(VideoStandard), len));

  XKGeneral::HexStrToBytes(tmpData, &len, TRUE);

  memcpy(&m_EEPROMData.VideoStandard, tmpData, VIDEOSTANDARD_SIZE);

  CalculateChecksum2();
}


/* Set Video Standard with Enum */
void XKEEPROM::SetVideoStandardVal(VIDEO_STANDARD StandardVal)
{
    VIDEO_STANDARD* VidStandard = (VIDEO_STANDARD*) ((LPDWORD)&m_EEPROMData.VideoStandard);

    char szTmp[128];
    sprintf(szTmp,"%08.8x\n", *VidStandard);
    OutputDebugString(szTmp);

    switch (StandardVal)
    {
      case (NTSC_M):
      case (PAL_I):
      {
        *VidStandard = StandardVal; //Only use first byte of Region...
        break;
      }
      default:
        *VidStandard = VID_INVALID; //If invalid,use Default of US
    }

    CalculateChecksum2();
}

/*Get Video Standard as Enum */
VIDEO_STANDARD XKEEPROM::GetVideoStandardVal()
{
  VIDEO_STANDARD retVal = (VIDEO_STANDARD) *((LPDWORD)&m_EEPROMData.VideoStandard);

  return retVal;
}


/* Encrypt the EEPROM Data for Specific XBOX Version by means of the SHA1 Middle Message hack..*/
BOOL XKEEPROM::EncryptAndCalculateCRC(XBOX_VERSION XBOXVersion)
{
  if (!m_EncryptedState)
  {
    m_XBOX_Version = XBOXVersion;
    return EncryptAndCalculateCRC();
  }
  else return FALSE;
}

/*Encrypt with Current XBOX version by means of the SHA1 Middle Message hack..*/
BOOL XKEEPROM::EncryptAndCalculateCRC()
{
  BOOL retVal = FALSE;
  UCHAR key_hash[20];         //rc4 key initializer

  XKRC4 RC4Obj;
  XKSHA1 SHA1Obj;

  if (((m_XBOX_Version == V_DBG)||(m_XBOX_Version == V1_0)||(m_XBOX_Version == V1_1)||(m_XBOX_Version == V1_6)) && (!m_EncryptedState))
  {
    //clear and re-create data_hash from decrypted data
    ZeroMemory(&m_EEPROMData.HMAC_SHA1_Hash, 20);
    SHA1Obj.XBOX_HMAC_SHA1(m_XBOX_Version, (UCHAR*)&m_EEPROMData.HMAC_SHA1_Hash, &m_EEPROMData.Confounder, 8, &m_EEPROMData.HDDKey, 20, NULL);

    //calculate rc4 key initializer data from eeprom key and data_hash
    SHA1Obj.XBOX_HMAC_SHA1(m_XBOX_Version, key_hash, &m_EEPROMData.HMAC_SHA1_Hash, 20, NULL);

    XKRC4::RC4KEY  RC4_key;

    //initialize RC4 key
    RC4Obj.InitRC4Key(key_hash, 20, &RC4_key);

    //Encrypt data (in eeprom) with generated key
    RC4Obj.RC4EnDecrypt ((UCHAR*)&m_EEPROMData.Confounder, 8, &RC4_key);
    RC4Obj.RC4EnDecrypt ((UCHAR*)&m_EEPROMData.HDDKey, 20, &RC4_key);

    CalculateChecksum2();
    CalculateChecksum3();

    m_EncryptedState = TRUE;
    retVal = TRUE;
  }
  else
  {
    retVal = FALSE;
    m_XBOX_Version = V_NONE;
  }

  return retVal;

}

/*Decrypt EEPROM using auto-detect by means of the SHA1 Middle Message hack..*/
BOOL XKEEPROM::Decrypt()
{
  BOOL retVal = FALSE;
  UCHAR key_hash[20];          //rc4 key initializer
  UCHAR data_hash_confirm[20]; //20 bytes
  UCHAR XBOX_Version = V_DBG;

  XKRC4 RC4Obj;
  XKRC4::RC4KEY  RC4_key;
  XKSHA1 SHA1Obj;
  UCHAR eepData[0x30];
    //int counter;
  //struct rc4_key RC4_key;
  //Keep the Original Data, incase the function fails we can restore it..
  ZeroMemory(eepData, 0x30);
  memcpy(eepData, &m_EEPROMData, 0x30);
    UCHAR Confounder[0x8] = { 0x4c,0x70,0x33,0xcb,0x5b,0xb5,0x97,0xd2 };

    while (((XBOX_Version < 13) && (!retVal)) && m_EncryptedState)
    {
        ZeroMemory(key_hash, 20);
    ZeroMemory(data_hash_confirm, 20);
        memset(&RC4_key,0,sizeof(RC4_key));
    //calculate rc4 key initializer data from eeprom key and data_hash
    SHA1Obj.XBOX_HMAC_SHA1(XBOX_Version, key_hash, &m_EEPROMData.HMAC_SHA1_Hash, 20, NULL);

    //initialize RC4 key
    RC4Obj.InitRC4Key(key_hash, 20, &RC4_key);

    //decrypt data (from eeprom) with generated key
    RC4Obj.RC4EnDecrypt ((UCHAR*)&m_EEPROMData.Confounder, 8, &RC4_key);
    RC4Obj.RC4EnDecrypt ((UCHAR*)&m_EEPROMData.HDDKey, 20, &RC4_key);

    //re-create data_hash from decrypted data
    SHA1Obj.XBOX_HMAC_SHA1(XBOX_Version, data_hash_confirm, &m_EEPROMData.Confounder, 8, &m_EEPROMData.HDDKey, 20, NULL);

    //ensure retrieved data_hash matches regenerated data_hash_confirm
    if (strncmp((const char*)&m_EEPROMData.HMAC_SHA1_Hash,(const char*)&data_hash_confirm[0],0x14))
    {
      //error: hash stored in eeprom[0:19] does not match
      //hash of data which should have been used to generate it.

      //The Key used was wrong.. Restore the Data back to original
      ZeroMemory(&m_EEPROMData, 0x30);
      memcpy(&m_EEPROMData, eepData, 0x30);

      m_XBOX_Version = V_NONE;
      retVal = FALSE;
      XBOX_Version++;
    }
    else
    {
      m_XBOX_Version = (XBOX_VERSION)XBOX_Version;
      m_EncryptedState = FALSE;
      retVal = TRUE;

    }
   }
    return retVal;
}


/*Decrypt With Specific RC4 Key, then detect which xbox Version is this key from*/
BOOL XKEEPROM::Decrypt(LPBYTE EEPROM_Key)
{
  BOOL retVal = FALSE;
  UCHAR key_hash[20];         //rc4 key initializer
  UCHAR data_hash_confirm[20];    //20 bytes
  UCHAR XBOX_Version = V_DBG;

  XKRC4 RC4Obj;
  XKRC4::RC4KEY  RC4_key;
  XKSHA1 SHA1Obj;
  UCHAR eepData[0x30];

  if (m_EncryptedState) //Can only decrypt if currently encrypted..
  {
    //Keep the Original Data, incase the function fails we can restore it..
    ZeroMemory(eepData, 0x30);
    memcpy(eepData, &m_EEPROMData, 0x30);

    ZeroMemory(key_hash, 20);
    ZeroMemory(data_hash_confirm, 20);

    //calculate rc4 key initializer data from eeprom key and data_hash
    SHA1Obj.HMAC_SHA1(key_hash, EEPROM_Key, 16, (UCHAR*)&m_EEPROMData.HMAC_SHA1_Hash, 20, NULL, 0);

    //initialize RC4 key
    RC4Obj.InitRC4Key(key_hash, 20, &RC4_key);

    //decrypt data (from eeprom) with generated key
    RC4Obj.RC4EnDecrypt ((UCHAR*)&m_EEPROMData.Confounder, 8, &RC4_key);
    RC4Obj.RC4EnDecrypt ((UCHAR*)&m_EEPROMData.HDDKey, 20, &RC4_key);

    //re-create data_hash from decrypted data
    SHA1Obj.HMAC_SHA1(data_hash_confirm, EEPROM_Key, 16, (UCHAR*)&m_EEPROMData.Confounder, 8, (UCHAR*)&m_EEPROMData.HDDKey, 20);

    //ensure retrieved data_hash matches regenerated data_hash_confirm
    if (strncmp((const char*)&m_EEPROMData.HMAC_SHA1_Hash,(const char*)&data_hash_confirm[0],0x14))
    {
      //error: hash stored in eeprom[0:19] does not match
      //hash of data which should have been used to generate it.

      //The Key used was wrong.. Restore the Data back to original
      ZeroMemory(&m_EEPROMData, 0x30);
      memcpy(&m_EEPROMData, eepData, 0x30);

      m_XBOX_Version = V_NONE;
      retVal = FALSE;
    }
    else
    {
      //Key supplied is indeed correct for this eeprom..
      //Now detect Version by simply decrypting again with auto detect..
      ZeroMemory(&m_EEPROMData, 0x30);
      memcpy(&m_EEPROMData, eepData, 0x30);
      m_XBOX_Version = V_NONE;
      retVal = Decrypt();
    }
  }
  return retVal;
}

//Encapsulated variable..
BOOL XKEEPROM::IsEncrypted()
{
  return m_EncryptedState;
}

//Calculate Checksum2
void XKEEPROM::CalculateChecksum2()
{
  //Calculate CRC for Serial, Mac, OnlineKey, video region
  XKCRC::QuickCRC(m_EEPROMData.Checksum2, m_EEPROMData.SerialNumber, 0x28);
}

//Calculate Checksum3
void XKEEPROM::CalculateChecksum3()
{
  //calculate CRC's for time zones, time standards, language, dvd region etc.
  XKCRC::QuickCRC(m_EEPROMData.Checksum3, m_EEPROMData.TimeZoneBias, 0x60);
}

void XKEEPROM::SetVideoFlag(VIDEO_FLAGS flag)
{
  m_EEPROMData.VideoFlags[2] |= (flag & 0x0e);
  CalculateChecksum3();
}

void XKEEPROM::UnsetVideoFlag(VIDEO_FLAGS flag)
{
  m_EEPROMData.VideoFlags[2] &= ~flag;
  CalculateChecksum3();
}

XKEEPROM::VIDEO_FLAGS XKEEPROM::GetVideoFlags()
{
  return  (XKEEPROM::VIDEO_FLAGS)(m_EEPROMData.VideoFlags[2] & 0x0e);
}

void XKEEPROM::SetVideoSize(VIDEO_SIZE size)
{
  // only one of these should ever be set
  if (size == LETTERBOX)
    m_EEPROMData.VideoFlags[2] &= ~WIDESCREEN;
  else if (size == WIDESCREEN)
    m_EEPROMData.VideoFlags[2] &= ~LETTERBOX;
  else
    return;

  m_EEPROMData.VideoFlags[2] |= (size);
  CalculateChecksum3();
}

XKEEPROM::VIDEO_SIZE XKEEPROM::GetVideoSize()
{
	return (XKEEPROM::VIDEO_SIZE)(m_EEPROMData.VideoFlags[2] & 0x11);
}
