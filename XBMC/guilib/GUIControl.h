#ifndef GUILIB_GUICONTROL_H
#define GUILIB_GUICONTROL_H
#pragma once

#include "gui3d.h"
#include "key.h"
#include "guimessage.h"
#include "graphiccontext.h"


class CGUIControl 
{
public:
  CGUIControl();
  CGUIControl(DWORD dwParentID, DWORD dwControlId, DWORD dwPosX, DWORD dwPosY, DWORD dwWidth, DWORD dwHeight);
  virtual ~CGUIControl(void);
  virtual void  Render();
  virtual void  OnAction(const CAction &action) ;
  virtual bool  OnMessage(CGUIMessage& message);
  DWORD         GetID(void) const; 
  DWORD         GetParentID(void) const; 
  bool          HasFocus(void) const;
  virtual void  AllocResources() {};
  virtual void  FreeResources() {};
  virtual bool  CanFocus() const;
	virtual bool  IsVisible() const;
	virtual bool  IsDisabled() const;
  virtual bool  IsSelected() const;
  virtual void  SetPosition(DWORD dwPosX, DWORD dwPosY);
  virtual void  SetAlpha(DWORD dwAlpha);
	virtual void  SetColourDiffuse(D3DCOLOR colour);
	virtual DWORD GetColourDiffuse() const { return m_colDiffuse;};
  DWORD         GetXPosition() const;
  DWORD         GetYPosition() const;
  virtual DWORD GetWidth() const;
  virtual DWORD GetHeight() const;
  void          SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight);
  void          SetFocus(bool bOnOff);
  void          SetWidth(int iWidth);
  void          SetHeight(int iHeight);
  void          SetVisible(bool bVisible);
	void					EnableCalibration(bool bOnOff);
	bool					CalibrationEnabled() const;
protected:  
  virtual void       Update() {};
  DWORD              m_dwControlLeft;
  DWORD              m_dwControlRight;
  DWORD              m_dwControlUp;
  DWORD              m_dwControlDown;
  DWORD              m_dwPosX;
  DWORD              m_dwPosY;
  DWORD              m_dwHeight;
  DWORD              m_dwWidth;
  D3DCOLOR           m_colDiffuse;
  DWORD							 m_dwControlID;
  DWORD							 m_dwParentID;
  bool							 m_bHasFocus;
	bool							 m_bVisible;
	bool							 m_bDisabled;
  bool							 m_bSelected;
	bool							 m_bCalibration;
};
#endif