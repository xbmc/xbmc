#ifndef GUILIB_SPINCONTROL_H
#define GUILIB_SPINCONTROL_H

#pragma once
#include "gui3d.h"
#include "guicontrol.h"
#include "guimessage.h"
#include "guifont.h"
#include "guiimage.h"
#include <string>
#include <vector>
using namespace std;

#define SPIN_CONTROL_TYPE_INT    1
#define SPIN_CONTROL_TYPE_FLOAT  2
#define SPIN_CONTROL_TYPE_TEXT   3

class CGUISpinControl :  public CGUIControl
{
public:  
  CGUISpinControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strUp, const CStdString& strDown, const CStdString& strUpFocus, const CStdString& strDownFocus, const CStdString& strFont, DWORD dwTextColor, int iType,DWORD dwAlign=XBFONT_RIGHT);
  virtual ~CGUISpinControl(void);
  virtual void 	Render();
  virtual void 	OnKey(const CKey& key) ;
  virtual bool 	OnMessage(CGUIMessage& message);
  virtual void 	AllocResources();
  virtual void 	FreeResources();
  virtual void 	SetPosition(DWORD dwPosX, DWORD dwPosY);
  virtual DWORD GetWidth() const;
  void 					SetRange(int iStart, int iEnd);
  void 					SetFloatRange(float fStart, float fEnd);
  void 					SetValue(int iValue);
  void 					SetFloatValue(float fValue);
  int  					GetValue() const;
  float					GetFloatValue() const;
  void					AddLabel(const wstring& strLabel, int  iValue);
  const WCHAR*	GetLabel() const;
  virtual void	SetFocus(bool bOnOff);
protected:
	bool			CanMoveDown();
	bool			CanMoveUp();
  int       m_iStart;
  int       m_iEnd;
  float     m_fStart;
  float     m_fEnd;
  int       m_iValue;
  float     m_fValue;
  int       m_iType;
  int       m_iSelect;
  vector<wstring> m_vecLabels;
  vector<int>			m_vecValues;
	CGUIImage m_imgspinUp;
	CGUIImage m_imgspinDown;
	CGUIImage m_imgspinUpFocus;
	CGUIImage m_imgspinDownFocus;
  CStdString    m_strFont;
  CGUIFont* m_pFont;
  DWORD     m_dwTextColor;
	DWORD			m_dwAlign;
};
#endif
