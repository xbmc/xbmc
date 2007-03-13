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
private:
	bool b_IsHome;
  unsigned int iControl;
  void	SetLabelDummy();
};

