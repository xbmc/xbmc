/*!
	\file GUIControl.h
	\brief 
	*/

#ifndef GUILIB_GUICONTROL_H
#define GUILIB_GUICONTROL_H
#pragma once

#include "gui3d.h"
#include "key.h"
#include "common/mouse.h"
#include "guimessage.h"
#include "graphiccontext.h"


/*!
	\ingroup controls
	\brief Base class for controls
	*/
class CGUIControl 
{
public:
  CGUIControl();
  CGUIControl(DWORD dwParentID, DWORD dwControlId, int iPosX, int iPosY, DWORD dwWidth, DWORD dwHeight);
  virtual ~CGUIControl(void);
  virtual void  Render();
  virtual void  OnAction(const CAction &action) ;
  // Common actions to make the code easier to read (no ugly switch statements in derived controls)
  virtual void  OnUp();
  virtual void  OnDown();
  virtual void  OnLeft();
  virtual void  OnRight();

  /// \brief Called when the mouse is over the control.  Default implementation selects the control.
  virtual void  OnMouseOver();
  /// \brief Called when the mouse is dragging over the control.  Default implementation does nothing.
  virtual void  OnMouseDrag() {};
  /// \brief Called when the left mouse button is pressed on the control.  Default implementation does nothing.
  virtual void  OnMouseClick(DWORD dwButton) {};
  /// \brief Called when the left mouse button is pressed on the control.  Default implementation does nothing.
  virtual void  OnMouseDoubleClick(DWORD dwButton) {};
  /// \brief Called when the mouse wheel has moved whilst over the control.  Default implementation does nothing
  virtual void	OnMouseWheel() {};
  virtual bool  OnMessage(CGUIMessage& message);
  DWORD         GetID(void) const; 
	void					SetID(DWORD dwID) { m_dwControlID = dwID; };
  DWORD         GetParentID(void) const; 
  bool          HasFocus(void) const;
	virtual void  PreAllocResources() {}
  virtual void  AllocResources() {}
  virtual void  FreeResources() {}
  virtual bool  CanFocus() const;
	virtual bool  IsVisible() const;
	virtual bool  IsDisabled() const;
  virtual bool  IsSelected() const;
  virtual void  SetPosition(int iPosX, int iPosY);
  virtual void  SetAlpha(DWORD dwAlpha);
	virtual void  SetColourDiffuse(D3DCOLOR colour);
	virtual DWORD GetColourDiffuse() const { return m_colDiffuse;};
  virtual int GetXPosition() const;
  virtual int GetYPosition() const;
  virtual DWORD GetWidth() const;
  virtual DWORD GetHeight() const;
  /// \brief Used to test whether the pointer location (fPosX, fPosY) is inside the control.  For mouse events.
  virtual bool	HitTest(int iPosX, int iPosY) const;
  void          SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight);
  DWORD         GetControlIdUp() const { return m_dwControlUp;};
  DWORD         GetControlIdDown() const { return m_dwControlDown;};
  DWORD         GetControlIdLeft() const { return m_dwControlLeft;};
  DWORD         GetControlIdRight() const { return m_dwControlRight;};
  void          SetFocus(bool bOnOff);
  void          SetWidth(int iWidth);
  void          SetHeight(int iHeight);
  void          SetVisible(bool bVisible);
  void			SetSelected(bool bSelected);
	void					SetEnabled(bool bEnable);
	virtual void			EnableCalibration(bool bOnOff);
	bool					CalibrationEnabled() const;
  void SetGroup(int iGroup);
  int GetGroup(void) const;

	enum GUICONTROLTYPES { 
		GUICONTROL_UNKNOWN,
		GUICONTROL_BUTTON,
		GUICONTROL_CHECKMARK,
		GUICONTROL_FADELABEL,
		GUICONTROL_IMAGE,
		GUICONTROL_LABEL,
		GUICONTROL_LIST,
		GUICONTROL_LISTEX,
		GUICONTROL_MBUTTON,
		GUICONTROL_PROGRESS,
		GUICONTROL_RADIO,
		GUICONTROL_RAM,
		GUICONTROL_RSS,
		GUICONTROL_SELECTBUTTON,
		GUICONTROL_SLIDER,
		GUICONTROL_SPINBUTTON,
		GUICONTROL_SPIN,
		GUICONTROL_TEXTBOX,
		GUICONTROL_THUMBNAIL,
		GUICONTROL_TOGGLEBUTTON,
		GUICONTROL_VIDEO,
		GUICONTROL_MOVER,
		GUICONTROL_RESIZE,
		GUICONTROL_BUTTONBAR,
		GUICONTROL_CONSOLE
	};
	GUICONTROLTYPES GetControlType() const { return ControlType; }

protected:
  virtual void       Update() {};
  DWORD              m_dwControlLeft;
  DWORD              m_dwControlRight;
  DWORD              m_dwControlUp;
  DWORD              m_dwControlDown;
  int				         m_iPosX;
  int                m_iPosY;
  DWORD              m_dwHeight;
  DWORD              m_dwWidth;
  D3DCOLOR           m_colDiffuse;
  DWORD				 m_dwAlpha;
  DWORD							 m_dwControlID;
  DWORD							 m_dwParentID;
  bool							 m_bHasFocus;
	bool							 m_bVisible;
	bool							 m_bDisabled;
  bool							 m_bSelected;
  int                m_iGroup;
	bool							 m_bCalibration;
	GUICONTROLTYPES ControlType;
};
#endif
