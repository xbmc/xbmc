#pragma once

#include "md5.h"
#include "../xbox/xkeeprom.h"

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
    CSysInfo();
    virtual ~CSysInfo();
    static void RemoveInstance();
    static CSysInfo* GetInstance();

    bool GetDVDInfo(CStdString& strDVDModel, CStdString& strDVDFirmware);
    bool GetHDDInfo(CStdString& strHDDModel, CStdString& strHDDSerial,CStdString& strHDDFirmware,CStdString& strHDDpw,CStdString& strHDDLockState);
    
    CStdString SmartXXModCHIP();
    CStdString GetAVPackInfo();
    CStdString GetVideoEncoder();
    CStdString GetMPlayerVersion();
    CStdString GetKernelVersion();
    CStdString GetSystemUpTime(bool bTotalUptime);
    CStdString GetCPUFreqInfo();
    CStdString GetXBVerInfo();
    CStdString GetUnits(int iFrontPort);
    CStdString GetMACAddress();
    CStdString GetXBOXSerial(bool bLabel);
    CStdString GetXBProduceInfo();
    CStdString GetVideoXBERegion();
    CStdString GetDVDZone();
    CStdString GetXBLiveKey();
    CStdString GetHDDKey();
    CStdString GetModChipInfo();
    CStdString GetBIOSInfo();
    CStdString GetINetState();

    bool GetRefurbInfo(CStdString& rfi_FirstBootTime, CStdString& rfi_PowerCycleCount);
    char MD5_Sign[32 + 1];

    bool CreateBiosBackup();
    bool CreateEEPROMBackup();
    void WriteTXTInfoFile(LPCSTR strFilename);

  private:
    static CSysInfo* m_pInstance;
    
    XKEEPROM* m_XKEEPROM;
    XBOX_VERSION  m_XBOXVersion;
    
    #define XBOX_BIOS_ID_INI_FILE "Q:\\System\\SystemInfo\\BiosIDs.ini"
    #define XBOX_BIOS_BACKUP_FILE "Q:\\System\\SystemInfo\\BIOSBackup.bin"
    #define XBOX_EEPROM_BIN_BACKUP_FILE "Q:\\System\\SystemInfo\\EEPROMBackup.bin"
    #define XBOX_EEPROM_CFG_BACKUP_FILE "Q:\\System\\SystemInfo\\EEPROMBackup.cfg"
    #define SYSINFO_TMP_SIZE 256
    #define XDEVICE_TYPE_IR_REMOTE  (&XDEVICE_TYPE_IR_REMOTE_TABLE)
    #define DEBUG_KEYBOARD
    #define DEBUG_MOUSE
    
    double GetCPUFrequency();
    double RDTSC(void);
    bool SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays);
    bool GetXBOXVersionDetected(CStdString& strXboxVer);
    CStdString GetModCHIPDetected();

    struct Bios * LoadBiosSigns();
    bool CheckBios(CStdString& strDetBiosNa);
    char* ReturnBiosName(char *buffer, char *str);
    char* ReturnBiosSign(char *buffer, char *str);
    char* CheckMD5 (struct Bios *Listone, char *Sign);
    char* MD5Buffer(char *filename,long PosizioneInizio,int KBytes);
    CStdString MD5BufferNew(char *filename,long PosizioneInizio,int KBytes);
};

extern CSysInfo g_sysinfo;