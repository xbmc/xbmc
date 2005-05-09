#pragma once
#include "guiwindow.h"
#include "GUIDialogProgress.h"

class CGUIWindowSystemInfo :
      public CGUIWindow
{
public:
	CGUIWindowSystemInfo(void);
	virtual ~CGUIWindowSystemInfo(void);
	virtual bool    OnMessage(CGUIMessage& message);
	virtual bool	OnAction(const CAction &action);
	virtual void	Render();

	wchar_t	m_wszMPlayerVersion[50];
	unsigned short  fanSpeed;
	DWORD	m_dwlastTime ;
	DWORD	m_dwFPSTime;
	DWORD	m_dwFrames;
	float	cputemp;
	float	mbtemp;
	
	void	UpdateButtons();
	void	SetLabelDummy();
	void	CreateEEPROMBackup(LPCSTR BackupFilePrefix);
	void	WriteTXTInfoFile(LPCSTR strFilename);
	
	bool	GetUnits(int i_lblp1, int i_lblp2);
	bool	GetATAPIValues(int i_lblp1, int i_lblp2);
	bool	GetNetwork(int i_lblp1, int i_lblp2, int i_lblp3, int i_lblp4, int i_lblp5, int i_lblp6);
	bool	GetBuildTime(int i_lblp1, int i_lblp2, int i_lblp3);
	bool	GetATAValues(int i_lblp1, int i_lblp2, int i_lblp3, int i_lblp4, int i_lblp5);
	bool	GetStorage(int i_lblp1, int i_lblp2, int i_lblp3, int i_lblp4, int i_lblp5, int i_lblp6, int i_lblp7, int i_lblp8, int i_lblp9, int i_lblp10);
	
	static bool	GetKernelVersion(CStdString& strKernel);
	static bool	GetCPUFreqInfo(CStdString& strItemCPU);
	static bool	GetMACAdress(CStdString& strMacAdress);
	static bool	GetBIOSInfo(CStdString& strBiosName);
	static bool GetVideoEncInfo(CStdString& strItemVideoENC);
	static bool GetResolution(CStdString& strResol);
	static bool GetXBVerInfo(CStdString& strXBoxVer);
	static bool GetXBOXSerial(CStdString& strXBOXSerial);
	static bool GetXBProduceInfo(CStdString& strXBProDate);
	static bool GetModChipInfo(CStdString& strModChip);
	static void GetAVPackInfo(CStdString& stravpack);
	static bool GetVideoXBERegion(CStdString& strVideoXBERegion);
	static bool GetDVDZone(CStdString& strdvdzone);
	static bool GetINetState(CStdString& strInetCon);
	static bool GetXBLiveKey(CStdString& strXBLiveKey);
	static bool GetHDDKey(CStdString& strhddlockey);
	static bool GetHDDTemp(CStdString& strItemhdd);
	static bool GetCurTime(CStdString& strCurTime);
	static void GetFreeMemory(CStdString& strFreeMem);
	static bool GetCPUTemp(CStdString& strCPUTemp);
	static bool GetGPUTemp(CStdString& strGPUTemp);
	static bool GetFanSpeed(CStdString& strFanSpeed);
	
private:
	bool b_IsHome;
	
	#define CONTROL_BT_HDD			92
	#define CONTROL_BT_DVD			93
	#define CONTROL_BT_STORAGE		94
	#define CONTROL_BT_DEFAULT		95
	#define CONTROL_BT_NETWORK		96
	#define CONTROL_BT_VIDEO		97
	#define CONTROL_BT_HARDWARE		98
	#define AddStr(a,b) (pstrOut += wsprintf( pstrOut, a, b ))

	static void	BytesToHexStr(LPBYTE SrcBytes, DWORD byteCount, LPSTR DstString);
	static void	BytesToHexStr(LPBYTE SrcBytes, DWORD byteCount, LPSTR DstString, UCHAR Seperator);
};

