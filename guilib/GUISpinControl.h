/*!
	\file GUISpinControl.h
	\brief 
	*/

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

/*!
	\ingroup controls
	\brief 
	*/
class CGUISpinControl :  public CGUIControl
{
public:  
  CGUISpinControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight, const CStdString& strUp, const CStdString& strDown, const CStdString& strUpFocus, const CStdString& strDownFocus, const CStdString& strFont, DWORD dwTextColor, int iType,DWORD dwAlign=XBFONT_RIGHT);
  virtual ~CGUISpinControl(void);
  virtual void 	Render();
  virtual void 	OnAction(const CAction &action) ;
  virtual bool 	OnMessage(CGUIMessage& message);
	virtual void PreAllocResources();
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
  virtual void SetDisabledColor(D3DCOLOR color);
  DWORD							GetDisabledColor() const { return m_dwDisabledColor;};
  void					AddLabel(const wstring& strLabel, int  iValue);
  void					AddLabel(CStdString aLabel, int iValue);
  const WCHAR*	GetLabel() const;
  virtual void	SetFocus(bool bOnOff);
	void					SetReverse(bool bOnOff);
  int           GetMaximum() const;
  int           GetMinimum() const;
  	DWORD	GetTextOffsetX() const { return m_dwTextOffsetX;};
	DWORD	GetTextOffsetY() const { return m_dwTextOffsetY;};
  	void	SetTextOffsetX(DWORD dwTextOffsetX) { m_dwTextOffsetX=dwTextOffsetX;};
	void	SetTextOffsetY(DWORD dwTextOffsetY) { m_dwTextOffsetY=dwTextOffsetY;};
	const	CStdString& GetTexutureUpName() const { return m_imgspinUp.GetFileName(); };
	const	CStdString& GetTexutureDownName() const { return m_imgspinDown.GetFileName(); };
	const	CStdString& GetTexutureUpFocusName() const { return m_imgspinUpFocus.GetFileName(); };
	const	CStdString& GetTexutureDownFocusName() const { return m_imgspinDownFocus.GetFileName(); };
	DWORD							GetTextColor() const { return m_dwTextColor;};
	const CStdString& GetFontName() const { return m_strFont; };
	DWORD							GetAlignment() const { return m_dwAlign;};
	int								GetType() const { return m_iType;};
	DWORD							GetSpinWidth() const { return m_imgspinUp.GetWidth(); };
	DWORD							GetSpinHeight() const { return m_imgspinUp.GetHeight(); };
  void              SetFloatInterval(float fInterval);
  float             GetFloatInterval() const;
  bool              GetShowRange() const;
  void              SetShowRange(bool bOnoff) ;
void		SetBuddyControlID(DWORD dwBuddyControlID);
void				SetNonProportional(bool bOnOff);
  void				Clear();
protected:
  void      PageUp();
  void      PageDown();
	bool			CanMoveDown();
	bool			CanMoveUp();
	void			MoveUp();
	void			MoveDown();
  int       m_iStart;
  int       m_iEnd;
  float     m_fStart;
  float     m_fEnd;
  int       m_iValue;
  float     m_fValue;
  int       m_iType;
  int       m_iSelect;
	bool			m_bReverse;
  float     m_fInterval;
  vector<wstring> m_vecLabels;
  vector<int>			m_vecValues;
	CGUIImage m_imgspinUp;
	CGUIImage m_imgspinDown;
	CGUIImage m_imgspinUpFocus;
	CGUIImage m_imgspinDownFocus;
  CStdString    m_strFont;
  CGUIFont* m_pFont;
  DWORD		m_dwTextOffsetX;
  DWORD		m_dwTextOffsetY;
  DWORD     m_dwTextColor;
  DWORD     m_dwDisabledColor;
	DWORD			m_dwAlign;
  bool      m_bShowRange;
  char      m_szTyped[10];
  int       m_iTypedPos;
  DWORD		m_dwBuddyControlID;
  bool		m_bBuddyDisabled;
  float		m_fMaxTextWidth;
};
#endif
