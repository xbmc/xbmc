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
#include "guicontrol.h" 


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
  bool            		Load(const CStdString& strFileName);
  virtual void    		Render();
  virtual void    		OnAction(const CAction &action);
  virtual bool    		OnMessage(CGUIMessage& message);
  void            		Add(CGUIControl* pControl);
	void								Remove(DWORD dwId);
  int             		GetFocusControl();
  void            		SelectNextControl();
	void                SelectPreviousControl();
  DWORD           		GetID(void) const;
	void								SetID(DWORD dwID);
	DWORD								GetPreviousWindowID(void) const;
	const CGUIControl*	GetControl(int iControl) const;
	void								ClearAll();
	int									GetFocusedControl() const;
	virtual void				AllocResources();
  virtual void				FreeResources();
	virtual void				ResetAllControls();
	static void         FlushReferenceCache();
protected:
  virtual void        OnWindowLoaded();
	struct stReferenceControl
	{
		char				 m_szType[128];
		CGUIControl* m_pControl;
	};
	typedef vector<struct stReferenceControl> VECREFERENCECONTOLS;
	typedef vector<struct stReferenceControl>::iterator IVECREFERENCECONTOLS;
	bool LoadReference(const CStdString& strFileName, VECREFERENCECONTOLS& controls);
	static CStdString CacheFilename;
	static VECREFERENCECONTOLS ControlsCache;

	vector<CGUIControl*> m_vecControls;
  typedef vector<CGUIControl*>::iterator ivecControls;
  DWORD  m_dwWindowId;
  DWORD  m_dwPreviousWindowId;
  DWORD  m_dwDefaultFocusControlID;
  vector<CPosition> m_vecPositions;
};

#endif
