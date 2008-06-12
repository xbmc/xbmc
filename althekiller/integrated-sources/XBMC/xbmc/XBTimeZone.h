#pragma once

#define XC_DISABLE_DST_FLAG  0x02
#define XC_DST_SETTING       0x11
#define XC_USER_SETTINGS     0xFF

// slightly different mini timezone structure from the structure used in xboxdash.xbe:
//  differences:
//    - structured like TIME_ZONE_INFORMATION for consistency and sanity
//    - no weird (apparently unused?) extra flag bytes
//    - standard char arrays are used

typedef struct _DST_DATE {
    BYTE Month;     // 1-12
    BYTE Week;      // 1-5
    BYTE DayOfWeek; // 0-6
    BYTE Hour;      // 0-24?
} DST_DATE;

typedef struct _MINI_TZI {
  const char * Name;
  SHORT        Bias;
  const char * StandardName;
  DST_DATE     StandardDate;
  SHORT        StandardBias;
  const char * DaylightName;
  DST_DATE     DaylightDate;
  SHORT        DaylightBias;
} MINI_TZI;

// Snagged from XKEEPROM.H, a few modifications :)
typedef struct _EEPROM_USER_SETTINGS {
     BYTE   Checksum3[4];           // 0x60 - 0x63  other Checksum of next

     LONG   TimeZoneBias;        // 0x64 - 0x67 Zone Bias?
     DWORD  TimeZoneStdName;     // 0x68 - 0x6B Standard timezone
     DWORD  TimeZoneDltName;     // 0x5C - 0x6F Daylight timezone
     BYTE   UNKNOWN4[8];            // 0x70 - 0x77 Unknown Padding ?
     DST_DATE   TimeZoneStdDate;     // 0x78 - 0x7B 10-05-00-02 (Month-Day-DayOfWeek-Hour)
     DST_DATE   TimeZoneDltDate;     // 0x7C - 0x7F 04-01-00-02 (Month-Day-DayOfWeek-Hour)
     BYTE   UNKNOWN5[8];            // 0x80 - 0x87 Unknown Padding ?
     LONG   TimeZoneStdBias;     // 0x88 - 0x8B Standard Bias
     LONG   TimeZoneDltBias;     // 0x8C - 0x8F Daylight Bias

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
} EEPROM_USER_SETTINGS;


extern const MINI_TZI g_TimeZoneInfo[];

class XBTimeZone
{
public:
  static const char * GetTimeZoneString(int index);
  static const char * GetTimeZoneName(int index);

  static int GetNumberOfTimeZones();
  static int GetTimeZoneIndex();
  static void SetTimeZoneIndex(int index);
  static bool SetTimeZoneInfo(const MINI_TZI * tzi);
  
  static bool GetDST();
  static void SetDST(BOOL bEnable);
};

extern XBTimeZone g_timezone;
