/*!
	\file GUIWindow.h
	\brief 
	*/

#ifndef GUILIB_GUIWINDOW_H
#define GUILIB_GUIWINDOW_H

#pragma once

#include <map>
#include <string>
#include <vector>
using namespace std;
#include "guiControl.h" 
#include "guiCallback.h"

#define ON_CLICK_MESSAGE(i,c,m) \
{ \
	EventHandler<c, CGUIMessage&> clickHandler(this, m); \
	m_mapClickEvents[i] = clickHandler; \
} \

// forward
class TiXmlNode;

class CPosition
{
public:
  CGUIControl* pControl;
  int x;
  int y;
};
/*!
	\ingroup winmsg
	\brief 
	*/
class CGUIWindow
{
public:
	CGUIWindow(DWORD dwID);
	virtual ~CGUIWindow(void);

	virtual bool      Load(const CStdString& strFileName, bool bContainsPath = false);
	virtual void			SetPosition(int iPosX, int iPosY);
	void					CenterWindow();
	virtual void    		Render();
	virtual void    		OnAction(const CAction &action);
	void					OnMouseAction();
	virtual void			OnMouse();
	bool					HandleMouse(CGUIControl *pControl);
	virtual bool    		OnMessage(CGUIMessage& message);
	void            		Add(CGUIControl* pControl);
	void					Remove(DWORD dwId);
	int             		GetFocusControl();
	void            		SelectNextControl();
	void	                SelectPreviousControl();
	void					SetID(DWORD dwID);
	DWORD           		GetID(void) const;
	DWORD					GetPreviousWindowID(void) const;
	DWORD					GetWidth() { return m_dwWidth; };
	DWORD					GetHeight() { return m_dwHeight; };
	int						GetPosX() { return m_iPosX; };
	int						GetPosY() { return m_iPosY; };
	const CGUIControl*		GetControl(int iControl) const;
	void					ClearAll();
	int						GetFocusedControl() const;
	virtual void			AllocResources();
	virtual void			FreeResources();
	virtual void			ResetAllControls();
	static void		        FlushReferenceCache();
	virtual bool	IsDialog() {return false;};

protected:
	virtual void        OnWindowLoaded() {};
  virtual void			OnInitWindow();
	struct stReferenceControl
	{
		char				 m_szType[128];
		CGUIControl* m_pControl;
	};

	typedef Event<CGUIMessage&> CLICK_EVENT; 
	typedef map<int, CLICK_EVENT> MAPCONTROLCLICKEVENTS;
	MAPCONTROLCLICKEVENTS m_mapClickEvents;

	typedef vector<struct stReferenceControl> VECREFERENCECONTOLS;
	typedef vector<struct stReferenceControl>::iterator IVECREFERENCECONTOLS;
	bool LoadReference(VECREFERENCECONTOLS& controls);
  void LoadControl(const TiXmlNode* pControl, int iGroup, VECREFERENCECONTOLS& referencecontrols,RESOLUTION& resToUse);
	static CStdString CacheFilename;
	static VECREFERENCECONTOLS ControlsCache;

	vector<CGUIControl*> m_vecControls;
  typedef vector<CGUIControl*>::iterator ivecControls;
  DWORD  m_dwWindowId;
  DWORD  m_dwPreviousWindowId;
  DWORD  m_dwDefaultFocusControlID;
  bool m_bRelativeCoords;
	bool m_bNeedsScaling;
  DWORD m_iPosX;
  DWORD m_iPosY;
  DWORD	m_dwWidth;
  DWORD m_dwHeight;
  vector<int> m_vecGroups;
};

#endif
