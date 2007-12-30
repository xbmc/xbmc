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
**       XKEEPROM.H - XBOX EEPROM Class' Header
********************************************************************************************************
**
**  This is the Class Header, see the .CPP file for more comments and implementation details.
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
--------------------------------------------------------------------------------------------------------
*/

#pragma once
#if defined (_WINDOWS)
	#pragma message ("Compiling for WINDOWS: " __FILE__)
//	#include <afxwin.h>         // MFC core and standard components
#elif defined (_XBOX)
	#pragma message ("Compiling for XBOX: " __FILE__)
	#include <xtl.h>
	#include "XKExports.h"
	#include "XKUtils.h"
#else
	#error ERR: Have to Define _WINDOWS or _XBOX !!
#endif

#include "XKGeneral.h"
#include "XKRC4.h"
#include "XKSHA1.h"
#include "XKCRC.h"
class XKEEPROM
{
public:
  //Defines for Data structure sizes..
  #define EEPROM_SIZE         0x100
  #define CONFOUNDER_SIZE     0x008
  #define HDDKEY_SIZE         0x010
  #define XBEREGION_SIZE      0x001
  #define SERIALNUMBER_SIZE   0x00C
  #define MACADDRESS_SIZE     0x006
  #define ONLINEKEY_SIZE      0x010
  #define DVDREGION_SIZE      0x001
  #define VIDEOSTANDARD_SIZE  0x004

  //EEPROM Data structe value enums
  enum XBOX_VERSION
  {
    V_NONE = 0x00,
    V_DBG = 0x09,
    V1_0  = 0x0A,
    V1_1  = 0x0B,
    V1_6  = 0x0C
  };

  enum DVD_ZONE
  {
    ZONE_NONE = 0x00,
    ZONE1 = 0x01,
    ZONE2 = 0x02,
    ZONE3 = 0x03,
    ZONE4 = 0x04,
    ZONE5 = 0x05,
    ZONE6 = 0x06
  };

  enum VIDEO_FLAGS
  {
    VIDEO_480I      =0x00,
    VIDEO_720P      =0x02,
    VIDEO_1080I     =0x04,
    VIDEO_480P      =0x08
  };

  enum VIDEO_SIZE
  {
    LETTERBOX =0x10,
    WIDESCREEN=0x01
  };

  enum VIDEO_STANDARD
  {
    VID_INVALID = 0x00000000,
    NTSC_M      = 0x00400100,
    NTSC_J      = 0x00400200,
    PAL_I       = 0x00800300,
    PAL_M       = 0x00400400
  };

  enum XBE_REGION
  {
    XBE_INVALID    = 0x00,
    NORTH_AMERICA  = 0x01,
    JAPAN          = 0x02,
    EURO_AUSTRALIA = 0x04
  };


  //Structure that holds contents of 256 byte EEPROM image..
  struct EEPROMDATA
  {
     BYTE   HMAC_SHA1_Hash[20]; // 0x00 - 0x13 HMAC_SHA1 Hash
     BYTE   Confounder[8];      // 0x14 - 0x1B RC4 Encrypted Confounder ??
     BYTE   HDDKey[16];        // 0x1C - 0x2B RC4 Encrypted HDD key
     BYTE   XBERegion[4];       // 0x2C - 0x2F RC4 Encrypted Region code (0x01 North America, 0x02 Japan, 0x04 Europe)

     BYTE   Checksum2[4];       // 0x30 - 0x33 Checksum of next 44 bytes
     UCHAR  SerialNumber[12];     // 0x34 - 0x3F Xbox serial number
     BYTE   MACAddress[6];        // 0x40 - 0x45 Ethernet MAC address
     BYTE   UNKNOWN2[2];          // 0x46 - 0x47  Unknown Padding ?

     BYTE   OnlineKey[16];        // 0x48 - 0x57 Online Key ?

     BYTE   VideoStandard[4];     // 0x58 - 0x5B  ** 0x00014000 = NTSC, 0x00038000 = PAL

     BYTE   UNKNOWN3[4];          // 0x5C - 0x5F  Unknown Padding ?


     //Comes configured up to here from factory..  everything after this can be zero'd out...
     //To reset XBOX to Factory settings, Make checksum3 0xFFFFFFFF and zero all data below (0x64-0xFF)
     //Doing this will Reset XBOX and upon startup will get Language & Setup screen...
     BYTE   Checksum3[4];           // 0x60 - 0x63  other Checksum of next

     BYTE   TimeZoneBias[4];        // 0x64 - 0x67 Zone Bias?
     UCHAR  TimeZoneStdName[4];     // 0x68 - 0x6B Standard timezone
     UCHAR  TimeZoneDltName[4];     // 0x5C - 0x6F Daylight timezone
     BYTE   UNKNOWN4[8];            // 0x70 - 0x77 Unknown Padding ?
     BYTE   TimeZoneStdDate[4];     // 0x78 - 0x7B 10-05-00-02 (Month-Day-DayOfWeek-Hour)
     BYTE   TimeZoneDltDate[4];     // 0x7C - 0x7F 04-01-00-02 (Month-Day-DayOfWeek-Hour)
     BYTE   UNKNOWN5[8];            // 0x80 - 0x87 Unknown Padding ?
     BYTE   TimeZoneStdBias[4];     // 0x88 - 0x8B Standard Bias?
     BYTE   TimeZoneDltBias[4];     // 0x8C - 0x8F Daylight Bias?

     BYTE   LanguageID[4];            // 0x90 - 0x93 Language ID

     BYTE   VideoFlags[4];            // 0x94 - 0x97 Video Settings
     BYTE   AudioFlags[4];            // 0x98 - 0x9B Audio Settings

     BYTE   ParentalControlGames[4];  // 0x9C - 0x9F 0=MAX rating
     BYTE   ParentalControlPwd[4];    // 0xA0 - 0xA3 7=X, 8=Y, B=LTrigger, C=RTrigger
     BYTE   ParentalControlMovies[4]; // 0xA4 - 0xA7 0=Max rating

     BYTE   XBOXLiveIPAddress[4];  // 0xA8 - 0xAB XBOX Live IP Address..
     BYTE   XBOXLiveDNS[4];        // 0xAC - 0xAF XBOX Live DNS Server..
     BYTE   XBOXLiveGateWay[4];    // 0xB0 - 0xB3 XBOX Live Gateway Address..
     BYTE   XBOXLiveSubNetMask[4]; // 0xB4 - 0xB7 XBOX Live Subnet Mask..
     BYTE   OtherSettings[4];      // 0xA8 - 0xBB Other XBLive settings ?

     BYTE   DVDPlaybackKitZone[4]; // 0xBC - 0xBF DVD Playback Kit Zone

     BYTE   UNKNOWN6[64];          // 0xC0 - 0xFF Unknown Codes / History ?
  };
  typedef EEPROMDATA* LPEEPROMDATA;

  XKEEPROM(void);
  XKEEPROM(LPEEPROMDATA pEEPROMData, BOOL Encrypted);
  ~XKEEPROM(void);

  BOOL      ReadFromBINFile(LPCSTR FileName, BOOL IsEncrypted);
  BOOL      WriteToBINFile(LPCSTR FileName);

  BOOL      ReadFromCFGFile(LPCSTR FileName);
  BOOL      WriteToCFGFile(LPCSTR FileName);

//very XBOX specific funtions to read/write EEPROM from hardware
  void      ReadFromXBOX();
  void      WriteToXBOX();

  void      GetEEPROMData(LPEEPROMDATA pEEPROMData);
  void      SetDecryptedEEPROMData(XBOX_VERSION Version, LPEEPROMDATA pEEPROMData);
  void      SetEncryptedEEPROMData(LPEEPROMDATA pEEPROMData);

  XBOX_VERSION GetXBOXVersion();

  void      GetConfounderString(LPSTR Confounder);
  void      SetConfounderString(LPCSTR Confounder);

  void      GetHDDKeyString(LPSTR HDDKey);
  void      SetHDDKeyString(LPCSTR HDDKey);

  void      GetXBERegionString(LPSTR XBERegion);
  void      SetXBERegionString(LPCSTR XBERegion);
  void      SetXBERegionVal(XBE_REGION RegionVal);
  XBE_REGION GetXBERegionVal();

  void      GetSerialNumberString(LPSTR SerialNumber);
  void      SetSerialNumberString(LPCSTR SerialNumber);

  void      GetMACAddressString(LPSTR MACAddress, UCHAR Seperator);
  void      SetMACAddressString(LPCSTR MACAddress);

  void      GetOnlineKeyString(LPSTR OnlineKey);
  void      SetOnlineKeyString(LPCSTR OnlineKey);

  void      GetDVDRegionString(LPSTR DVDRegion);
  void      SetDVDRegionString(LPCSTR DVDRegion);
  void      SetDVDRegionVal(DVD_ZONE ZoneVal);
  DVD_ZONE  GetDVDRegionVal();

  void      GetVideoStandardString(LPSTR VideoStandard);
  void      SetVideoStandardString(LPCSTR VideoStandard);
  void      SetVideoStandardVal(VIDEO_STANDARD StandardVal);
  void      SetVideoFlag(VIDEO_FLAGS flags);
  void      UnsetVideoFlag(VIDEO_FLAGS flag);
  VIDEO_FLAGS GetVideoFlags();
  void      SetVideoSize(VIDEO_SIZE size);
  VIDEO_SIZE GetVideoSize();

  VIDEO_STANDARD  GetVideoStandardVal();

  BOOL      EncryptAndCalculateCRC();
  BOOL      EncryptAndCalculateCRC(XBOX_VERSION XBOXVersion);

  BOOL      Decrypt();
  BOOL      Decrypt(LPBYTE EEPROM_Key);

  BOOL      IsEncrypted();

  void      CalculateChecksum2();
  void      CalculateChecksum3();

  EEPROMDATA   m_EEPROMData;
  XBOX_VERSION m_XBOX_Version;
  BOOL         m_EncryptedState;
protected:


};

typedef XKEEPROM::XBOX_VERSION XBOX_VERSION;
typedef XKEEPROM::DVD_ZONE DVD_ZONE;
typedef XKEEPROM::VIDEO_STANDARD VIDEO_STANDARD;
typedef XKEEPROM::XBE_REGION XBE_REGION;
typedef XKEEPROM::EEPROMDATA EEPROMDATA;
typedef XKEEPROM::LPEEPROMDATA LPEEPROMDATA;

