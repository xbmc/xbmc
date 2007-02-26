#pragma once
#include "guiwindow.h"

#define CONTROL_BT_HDD			92
#define CONTROL_BT_DVD			93
#define CONTROL_BT_STORAGE		94
#define CONTROL_BT_DEFAULT		95
#define CONTROL_BT_NETWORK		96
#define CONTROL_BT_VIDEO		97
#define CONTROL_BT_HARDWARE		98

#define AddStr(a,b) (pstrOut += wsprintf( pstrOut, a, b ))

class CGUIWindowSystemInfo :
      public CGUIWindow
{
public:
	CGUIWindowSystemInfo(void);
	virtual ~CGUIWindowSystemInfo(void);
	virtual bool OnMessage(CGUIMessage& message);
	virtual bool OnAction(const CAction &action);
	virtual void Render();

	void	SetLabelDummy();
	bool	GetBuildTime(int i_lblp1, int i_lblp2, int i_lblp3);
#ifdef HAS_SYSINFO
	bool	GetNetwork(int i_lblp1, int i_lblp2, int i_lblp3, int i_lblp4, int i_lblp5, int i_lblp6, int i_lblp7);
	bool	GetStorage(int i_lblp1, int i_lblp2, int i_lblp3, int i_lblp4, int i_lblp5, int i_lblp6, int i_lblp7, int i_lblp8, int i_lblp9, int i_lblp10);
	
  static bool	GetBIOSInfo(CStdString& strBiosName);
	static bool GetModChipInfo(CStdString& strModChip);
	static bool GetINetState(CStdString& strInetCon);
	

#endif
private:
	bool b_IsHome;
#ifdef HAS_SYSINFO
	bool GetDiskSpace(const CStdString &drive, ULARGE_INTEGER &total, ULARGE_INTEGER& totalFree, CStdString &string);
#endif
};

