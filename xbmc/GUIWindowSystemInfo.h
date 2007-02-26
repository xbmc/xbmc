#pragma once
#include "guiwindow.h"
#include "xbox/xkeeprom.h"

#define CONTROL_BT_HDD			92
#define CONTROL_BT_DVD			93
#define CONTROL_BT_STORAGE		94
#define CONTROL_BT_DEFAULT		95
#define CONTROL_BT_NETWORK		96
#define CONTROL_BT_VIDEO		97
#define CONTROL_BT_HARDWARE		98
#define AddStr(a,b) (pstrOut += wsprintf( pstrOut, a, b ))

#define SYSINFO_TMP_SIZE 256

class CGUIWindowSystemInfo :
      public CGUIWindow
{
public:
	CGUIWindowSystemInfo(void);
	virtual ~CGUIWindowSystemInfo(void);
	virtual bool    OnMessage(CGUIMessage& message);
	virtual bool	OnAction(const CAction &action);
	virtual void	Render();

	void	SetLabelDummy();
	bool	GetBuildTime(int i_lblp1, int i_lblp2, int i_lblp3);
	static bool GetResolution(CStdString& strResol);
	static void GetFreeMemory(CStdString& strFreeMem);
#ifdef HAS_SYSINFO
	void	UpdateButtons();
	void	CreateEEPROMBackup(LPCSTR BackupFilePrefix);
	void	WriteTXTInfoFile(LPCSTR strFilename);
	
	bool	GetUnits(int i_lblp1, int i_lblp2, int i_lblp3);
	bool	GetRefurbInfo(int label1, int label2);
	bool	GetNetwork(int i_lblp1, int i_lblp2, int i_lblp3, int i_lblp4, int i_lblp5, int i_lblp6, int i_lblp7);
	bool	GetStorage(int i_lblp1, int i_lblp2, int i_lblp3, int i_lblp4, int i_lblp5, int i_lblp6, int i_lblp7, int i_lblp8, int i_lblp9, int i_lblp10);
	
	void GetMACAddress(CStdString& strMacAddress);
	static bool	GetBIOSInfo(CStdString& strBiosName);
	static bool GetXBVerInfo(CStdString& strXBoxVer);
	void GetXBOXSerial(CStdString& strXBOXSerial);
	void GetXBProduceInfo(CStdString& strXBProDate);
	static bool GetModChipInfo(CStdString& strModChip);
	void GetVideoXBERegion(CStdString& strVideoXBERegion);
	void GetDVDZone(CStdString& strdvdzone);
	static bool GetINetState(CStdString& strInetCon);
	void GetXBLiveKey(CStdString& strXBLiveKey);
	void GetHDDKey(CStdString& strhddlockey);

#endif
private:
	bool b_IsHome;
	DWORD	m_dwFPSTime;
	DWORD	m_dwFrames;
	float	cputemp;
	float	mbtemp;
  CStdString mplayerVersion;

#ifdef HAS_SYSINFO
	bool GetDiskSpace(const CStdString &drive, ULARGE_INTEGER &total, ULARGE_INTEGER& totalFree, CStdString &string);
#ifdef HAS_XBOX_HARDWARE
	XKEEPROM* m_pXKEEPROM;
	XBOX_VERSION  m_XBOX_Version;
#endif
#endif
};

