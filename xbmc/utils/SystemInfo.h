#pragma once

#define KB	(1024)					// 1 KiloByte (1KB) 	1024 Byte (2^10 Byte)
#define MB	(1024*KB)				// 1 MegaByte (1MB) 	1024 KB (2^10 KB) 
#define GB	(1024*MB)				// 1 GigaByte (1GB) 	1024 MB (2^10 MB)
#define TB	(1024*GB)				// 1 TerraByte (1TB) 	1024 GB (2^10 GB)

#define SMARTXX_LED_OFF			0	// SmartXX ModCHIP LED Controll
#define SMARTXX_LED_BLUE		1
#define SMARTXX_LED_RED			2
#define SMARTXX_LED_BLUE_RED	3
#define SMARTXX_LED_CYCLE		4

#define MAX_KNOWN_ATTRIBUTES	46

struct Bios
{
	char Name[200];
	char Signature[200];
};

class CSysInfo
{
  public:
    CSysInfo();
    virtual ~CSysInfo();
    
    BYTE GetSmartValues(int SmartREQ);
	  double GetCPUFrequence();
	  double RDTSC(void);
    
	  CStdString GetAVPackInfo();
	  CStdString GetModCHIPDetected();
	  CStdString GetVideoEncoder();
	  CStdString SmartXXModCHIP();
    
	  UCHAR IdeRead(USHORT port);
    
	  bool	CheckBios(CStdString& strDetBiosNa);
	  bool GetXBOXVersionDetected(CStdString& strXboxVer);
	  bool GetDVDInfo(CStdString& strDVDModel, CStdString& strDVDFirmware);
	  bool SmartXXLEDControll(int iSmartXXLED);
	  bool GetHDDInfo(CStdString& strHDDModel, CStdString& strHDDSerial,CStdString& strHDDFirmware,CStdString& strHDDpw,CStdString& strHDDLockState);
	  struct Bios * LoadBiosSigns();
    
	  void Init_IDE();
	  void IdeWrite(USHORT port, UCHAR data);

	  int readDriveConfig(int drive);
	  int IDE_getNumBlocks(int driveNum);
	  int IDE_Read(int driveNum, int blockNum, char *buffer);
	  int IDE_Write(int driveNum, int blockNum, char *buffer);
    
	  bool SystemUpTime(int iInputMinutes, int &iMinutes, int &iHours, int &iDays);

  private:
    
    char* ReturnBiosName(char *buffer, char *str);
	  char* ReturnBiosSign(char *buffer, char *str);
	  char* CheckMD5 (struct Bios *Listone, char *Sign);
	  CStdString MD5FileNew(char *filename,long PosizioneInizio,int KBytes);

	  typedef struct _IDEREGS 
	  {	BYTE	bFeaturesReg;			// Used for specifying SMART "commands".
		  BYTE	bSectorCountReg;		// IDE sector count register
		  BYTE	bSectorNumberReg;		// IDE sector number register
		  BYTE	bCylLowReg;				// IDE low order cylinder value
		  BYTE	bCylHighReg;			// IDE high order cylinder value
		  BYTE	bDriveHeadReg;			// IDE drive/head register
		  BYTE	bCommandReg;			// Actual IDE command.
		  BYTE	bReserved;				// reserved for future use.  Must be zero.
	  }IDEREGS, *PIDEREGS, *LPIDEREGS;
	  typedef struct _SENDCMDINPARAMS 
	  {	DWORD	cBufferSize;			// Buffer size in bytes
		  IDEREGS	irDriveRegs;			// Structure with drive register values.
		  BYTE	bDriveNumber;			// Physical drive number to send command to (0,1,2,3)
		  BYTE	bReserved[3];			// Reserved for future expansion.
		  DWORD	dwReserved[4];			// For future use.
		  BYTE 	bBuffer[1];				// Input buffer.
	  }SENDCMDINPARAMS, *PSENDCMDINPARAMS, *LPSENDCMDINPARAMS;
	  typedef struct _GETVERSIONOUTPARAMS 
	  {	BYTE	bVersion;				// Binary driver version.
		  BYTE	bRevision;				// Binary driver revision.
		  BYTE	bReserved;				// Not used.
		  BYTE	bIDEDeviceMap;			// Bit map of IDE devices.
		  DWORD	fCapabilities;			// Bit mask of driver capabilities.
		  DWORD	dwReserved[4];			// For future use.
	  }GETVERSIONOUTPARAMS, *PGETVERSIONOUTPARAMS, *LPGETVERSIONOUTPARAMS;
	  typedef	struct	_DRIVEATTRIBUTE 
	  {	BYTE	bAttrID;				// Identifies which attribute
		  WORD	wStatusFlags;			// see bit definitions below
		  BYTE	bAttrValue;				// Current normalized value
		  BYTE	bWorstValue;			// How bad has it ever been?
		  BYTE	bRawValue[5];			// Un-normalized value
		  BYTE	bReserved;				// ...
	  } DRIVEATTRIBUTE, *PDRIVEATTRIBUTE, *LPDRIVEATTRIBUTE;
	  typedef	struct	_ATTRTHRESHOLD 
	  {	BYTE	bAttrID;				// Identifies which attribute
		  BYTE	bWarrantyThreshold;		// Triggering value
		  BYTE	bReserved[10];			// ...
	  } ATTRTHRESHOLD, *PATTRTHRESHOLD, *LPATTRTHRESHOLD;
  
protected:
    //
 };

extern CSysInfo g_sysinfo;