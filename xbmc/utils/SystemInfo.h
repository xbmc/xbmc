#pragma once

#include "md5.h"

#define KB  (1024)          // 1 KiloByte (1KB)   1024 Byte (2^10 Byte)
#define MB  (1024*KB)       // 1 MegaByte (1MB)   1024 KB (2^10 KB)
#define GB  (1024*MB)       // 1 GigaByte (1GB)   1024 MB (2^10 MB)
#define TB  (1024*GB)       // 1 TerraByte (1TB)  1024 GB (2^10 GB)

#define SMARTXX_LED_OFF     0 // SmartXX ModCHIP LED Controll
#define SMARTXX_LED_BLUE    1
#define SMARTXX_LED_RED     2
#define SMARTXX_LED_BLUE_RED  3
#define SMARTXX_LED_CYCLE   4

#define MAX_KNOWN_ATTRIBUTES  46

struct Bios
{
  char Name[50];
  char Signature[33];
};

class CSysInfo
{
  public:
    static double GetCPUFrequency();
    static double RDTSC(void);

    static CStdString GetAVPackInfo();
    static CStdString GetModCHIPDetected();
    static CStdString GetVideoEncoder();
    static CStdString SmartXXModCHIP();

    static bool BackupBios();
    static bool CheckBios(CStdString& strDetBiosNa);
    static bool GetXBOXVersionDetected(CStdString& strXboxVer);
    static bool GetDVDInfo(CStdString& strDVDModel, CStdString& strDVDFirmware);
    static bool GetHDDInfo(CStdString& strHDDModel, CStdString& strHDDSerial,CStdString& strHDDFirmware,CStdString& strHDDpw,CStdString& strHDDLockState);
    static struct Bios * LoadBiosSigns();

    static bool SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays);

  private:

    static char* ReturnBiosName(char *buffer, char *str);
    static char* ReturnBiosSign(char *buffer, char *str);
    static char* CheckMD5 (struct Bios *Listone, char *Sign);
    static char* MDPrint (MD5_CTX *mdContext);
    static char* MD5Buffer(char *filename,long PosizioneInizio,int KBytes);
    static CStdString MD5BufferNew(char *filename,long PosizioneInizio,int KBytes);

    static char MD5_Sign[16];

    // Folder where the Bios Detections Files Are!
    static const char *cTempBIOSFile;
    static const char *cBIOSmd5IDs;
};
