/*!
\file GUIControl.h
\brief 
*/

#ifndef GUILIB_GUICONTROL_H
#define GUILIB_GUICONTROL_H
#pragma once

#include "key.h"
#include "GraphicContext.h"
#include "GUICallback.h"
#include "GUIFont.h"
#include "VisibleEffect.h"

class CLabelInfo
{
public:
  CLabelInfo()
  {
    textColor = 0;
    shadowColor = 0;
    disabledColor = 0;
    selectedColor = 0;
    font = NULL;
    align = XBFONT_LEFT;
    offsetX = offsetY = 0;
  };
  DWORD textColor;
  DWORD shadowColor;
  DWORD selectedColor;
  DWORD disabledColor;
  DWORD align;
  int offsetX;
  int offsetY;
  CAngle angle;
  CGUIFont *font;
};

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
  virtual void Render();

  // OnAction() is called by our window when we are the focused control.
  // We should process any control-specific actions in the derived classes,
  // and return true if we have taken care of the action.  Returning false
  // indicates that the message may be handed down to the window or application
  // levels.  This base class implementation handles basic movement, and should
  // be called from the derived classes when the action has not been handled.
  // Return true to indicate that the action has been dealt with.
  virtual bool OnAction(const CAction &action);

  // Common actions to make the code easier to read (no ugly switch statements in derived controls)
  virtual void OnUp();
  virtual void OnDown();
  virtual void OnLeft();
  virtual void OnRight();

  /// \brief Called when the mouse is over the control.  Default implementation selects the control.
  virtual void OnMouseOver();
  /// \brief Called when the mouse is dragging over the control.  Default implementation does nothing.
  virtual void OnMouseDrag() {};
  /// \brief Called when the left mouse button is pressed on the control.  Default implementation does nothing.
  virtual void OnMouseClick(DWORD dwButton) {};
  /// \brief Called when the left mouse button is pressed on the control.  Default implementation does nothing.
  virtual void OnMouseDoubleClick(DWORD dwButton) {};
  /// \brief Called when the mouse wheel has moved whilst over the control.  Default implementation does nothing
  virtual void OnMouseWheel() {};
  virtual bool OnMessage(CGUIMessage& message);
  DWORD GetID(void) const;
  void SetID(DWORD dwID) { m_dwControlID = dwID; };
  DWORD GetParentID(void) const;
  bool HasFocus(void) const;
  virtual void PreAllocResources() {}
  virtual void AllocResources();
  virtual void FreeResources();
          bool IsAllocated();
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return false; };
  virtual bool CanFocus() const;
  virtual bool IsVisible() const;
  virtual bool IsDisabled() const;
  virtual bool IsSelected() const;
  bool HasRendered() const { return m_hasRendered; };
  virtual void SetPosition(int iPosX, int iPosY);
  virtual void SetAlpha(DWORD dwAlpha);
  virtual void SetColourDiffuse(D3DCOLOR colour);
  virtual DWORD GetColourDiffuse() const { return m_colDiffuse;};
  virtual int GetXPosition() const;
  virtual int GetYPosition() const;
  virtual DWORD GetWidth() const;
  virtual DWORD GetHeight() const;
  /// \brief Used to test whether the pointer location (fPosX, fPosY) is inside the control.  For mouse events.
  virtual bool HitTest(int iPosX, int iPosY) const;
  virtual void SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight);
  DWORD GetControlIdUp() const { return m_dwControlUp;};
  DWORD GetControlIdDown() const { return m_dwControlDown;};
  DWORD GetControlIdLeft() const { return m_dwControlLeft;};
  DWORD GetControlIdRight() const { return m_dwControlRight;};
  void SetFocus(bool bOnOff);
  virtual void SetWidth(int iWidth);
  virtual void SetHeight(int iHeight);
  virtual void SetVisible(bool bVisible);
  void SetVisibleCondition(int visible);
//#ifdef PRE_SKIN_VERSION_2_0_COMPATIBILITY
  void SetVisibleCondition(int visible, bool allowHiddenFocus, bool startHidden);
//  void SetVisibleCondition(int visible, bool allowHiddenFocus);
  int GetVisibleCondition() const { return m_visibleCondition; };
  void UpdateVisibility();
  void SetInitialVisibility();
  bool UpdateEffectState();
  void SetSelected(bool bSelected);
  virtual void SetEnabled(bool bEnable);
  bool CalibrationEnabled() const;
  void SetGroup(int iGroup);
  int GetGroup(void) const;
  virtual void Update() { m_bInvalidated = true; };
  virtual void SetPulseOnSelect(bool pulse) { m_pulseOnSelect = pulse; };
  bool GetPulseOnSelect() const { return m_pulseOnSelect; };
  virtual CStdString GetDescription() const { return ""; };

  enum GUICONTROLTYPES {
    GUICONTROL_UNKNOWN,
    GUICONTROL_BUTTON,
    GUICONTROL_CONDITIONAL_BUTTON,
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
    GUICONTROL_SETTINGS_SLIDER,
    GUICONTROL_SPINBUTTON,
    GUICONTROL_SPIN,
    GUICONTROL_SPINEX,
    GUICONTROL_TEXTBOX,
    GUICONTROL_THUMBNAIL,
    GUICONTROL_TOGGLEBUTTON,
    GUICONTROL_VIDEO,
    GUICONTROL_MOVER,
    GUICONTROL_RESIZE,
    GUICONTROL_BUTTONBAR,
    GUICONTROL_CONSOLE,
    GUICONTROL_EDIT,
    GUICONTROL_VISUALISATION,
    GUICONTROL_MULTI_IMAGE
  };
  GUICONTROLTYPES GetControlType() const { return ControlType; }

  void SetAnimations(const vector<CAnimation> &animations);
  void QueueAnimation(ANIMATION_TYPE anim);
  bool IsAnimating(ANIMATION_TYPE anim);
  CAnimation *GetAnimation(ANIMATION_TYPE type);
protected:
  void Animate();
  void UpdateStates(ANIMATION_TYPE type, ANIMATION_PROCESS currentProcess, ANIMATION_STATE currentState);

  DWORD m_dwControlLeft;
  DWORD m_dwControlRight;
  DWORD m_dwControlUp;
  DWORD m_dwControlDown;
  int m_iPosX;
  int m_iPosY;
  DWORD m_dwHeight;
  DWORD m_dwWidth;
  D3DCOLOR m_colDiffuse;
  DWORD m_dwAlpha;
  DWORD m_dwControlID;
  DWORD m_dwParentID;
  bool m_bHasFocus;
  bool m_bVisible;
  bool m_bDisabled;
  bool m_bSelected;
  int m_iGroup;
  bool m_bCalibration;
  bool m_bInvalidated;
  bool m_bAllocated;
  bool m_pulseOnSelect;
  GUICONTROLTYPES ControlType;

  // visibility condition/state
  int m_visibleCondition;
  bool m_allowHiddenFocus;
  bool m_lastVisible;
  bool m_startHidden; // PRE_SKIN_VERSION_2_0_COMPATIBILITY
  bool m_hasRendered;

  // animation effects
  vector<CAnimation> m_animations;
  CAnimation m_tempAnimation;   // We can remove this once we have got rid of the effects
};

#endif
