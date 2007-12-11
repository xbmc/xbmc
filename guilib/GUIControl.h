/*!
\file GUIControl.h
\brief 
*/

#ifndef GUILIB_GUICONTROL_H
#define GUILIB_GUICONTROL_H
#pragma once

#include "GraphicContext.h" // needed by any rendering operation (all controls)
#include "Key.h"            // needed by practically all controls (CAction + defines)
#include "GUIMessage.h"     // needed by practically all controls
#include "GUIFont.h"        // needed for the CAngle member (CLabelInfo) among other stuff
#include "VisibleEffect.h"  // needed for the CAnimation members

class CGUIListItem; // forward

enum ORIENTATION { HORIZONTAL = 0, VERTICAL };

class CLabelInfo
{
public:
  CLabelInfo()
  {
    textColor = 0;
    shadowColor = 0;
    disabledColor = 0;
    selectedColor = 0;
    focusedColor = 0;
    font = NULL;
    align = XBFONT_LEFT;
    offsetX = offsetY = 0;
    width = 0;
    angle = 0;
  };
  DWORD textColor;
  DWORD shadowColor;
  DWORD selectedColor;
  DWORD disabledColor;
  DWORD focusedColor;
  DWORD align;
  float offsetX;
  float offsetY;
  float width;
  float angle;
  CGUIFont *font;
};

class CControlState
{
public:
  CControlState(int id, int data)
  {
    m_id = id;
    m_data = data;
  }
  int m_id;
  int m_data;
};

/*!
 \ingroup controls
 \brief Base class for controls
 */
class CGUIControl
{
public:
  CGUIControl();
  CGUIControl(DWORD dwParentID, DWORD dwControlId, float posX, float posY, float width, float height);
  virtual ~CGUIControl(void);

  virtual void DoRender(DWORD currentTime);
  virtual void Render();
  bool HasRendered() const { return m_hasRendered; };

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
  virtual void OnFocus() {};

  /// \brief Called when the mouse is over the control.  Default implementation selects the control.
  virtual bool OnMouseOver(const CPoint &point);
  /// \brief Called when the mouse is dragging over the control.  Default implementation does nothing.
  virtual bool OnMouseDrag(const CPoint &offset, const CPoint &point) { return false; };
  /// \brief Called when the left mouse button is pressed on the control.  Default implementation does nothing.
  virtual bool OnMouseClick(DWORD dwButton, const CPoint &point) { return false; };
  /// \brief Called when the left mouse button is pressed on the control.  Default implementation does nothing.
  virtual bool OnMouseDoubleClick(DWORD dwButton, const CPoint &point) { return false; };
  /// \brief Called when the mouse wheel has moved whilst over the control.  Default implementation does nothing
  virtual bool OnMouseWheel(char wheel, const CPoint &point) { return false; };
  /// \brief Used to test whether the pointer location (fPosX, fPosY) is inside the control.  For mouse events.
  virtual bool HitTest(const CPoint &point) const;
  /// \brief Focus a control from a screen location.  Returns the coordinates of the screen location relative to the control and a pointer to the control.
  virtual bool CanFocusFromPoint(const CPoint &point, CGUIControl **control, CPoint &controlPoint) const;
  /// \brief Unfocus a control if it's not in a screen location.
  virtual void UnfocusFromPoint(const CPoint &point);

  virtual bool OnMessage(CGUIMessage& message);
  DWORD GetID(void) const;
  void SetID(DWORD dwID) { m_dwControlID = dwID; };
  virtual bool HasID(DWORD dwID) const;
  virtual bool HasVisibleID(DWORD dwID) const;
  DWORD GetParentID() const;
  virtual bool HasFocus() const;
  virtual void PreAllocResources() {}
  virtual void AllocResources();
  virtual void FreeResources();
  virtual bool IsAllocated() const;
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return false; };
  virtual bool CanFocus() const;
  virtual bool IsVisible() const;
  virtual bool IsDisabled() const;
  virtual void SetPosition(float posX, float posY);
  virtual void SetHitRect(const CRect &rect);
  virtual void SetCamera(const CPoint &camera);
  virtual void SetColorDiffuse(D3DCOLOR color);
  virtual float GetXPosition() const;
  virtual float GetYPosition() const;
  virtual float GetWidth() const;
  virtual float GetHeight() const;
  virtual void SetNavigation(DWORD dwUp, DWORD dwDown, DWORD dwLeft, DWORD dwRight);
  virtual void SetNavigationActions(const vector<CStdString> &up, const vector<CStdString> &down,
                                    const vector<CStdString> &left, const vector<CStdString> &right);
  void ExecuteActions(const vector<CStdString> &actions);
  DWORD GetControlIdUp() const { return m_dwControlUp;};
  DWORD GetControlIdDown() const { return m_dwControlDown;};
  DWORD GetControlIdLeft() const { return m_dwControlLeft;};
  DWORD GetControlIdRight() const { return m_dwControlRight;};
  virtual DWORD GetNextControl(int direction) const;
//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  const CGUIControl *GetParentControl() const { return m_parentControl; };
//#endif
  virtual void SetFocus(bool focus);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetVisible(bool bVisible);
  void SetVisibleCondition(int visible, bool allowHiddenFocus);
  int GetVisibleCondition() const { return m_visibleCondition; };
  void SetEnableCondition(int condition);
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual void SetInitialVisibility();
  virtual void SetEnabled(bool bEnable);
  virtual void Update() { m_bInvalidated = true; };
  virtual void SetPulseOnSelect(bool pulse) { m_pulseOnSelect = pulse; };
  virtual CStdString GetDescription() const { return ""; };

  void SetAnimations(const vector<CAnimation> &animations);
  const vector<CAnimation> &GetAnimations() const { return m_animations; };

  virtual void QueueAnimation(ANIMATION_TYPE anim);
  virtual bool IsAnimating(ANIMATION_TYPE anim);
  CAnimation *GetAnimation(ANIMATION_TYPE type, bool checkConditions = true);
  virtual void ResetAnimation(ANIMATION_TYPE type);

  virtual bool IsGroup() const { return false; };
  virtual bool IsContainer() const { return false; };

  void SetParentControl(CGUIControl *control) { m_parentControl = control; };
  virtual void SaveStates(vector<CControlState> &states);

  enum GUICONTROLTYPES {
    GUICONTROL_UNKNOWN,
    GUICONTROL_BUTTON,
    GUICONTROL_CONDITIONAL_BUTTON,
    GUICONTROL_CHECKMARK,
    GUICONTROL_FADELABEL,
    GUICONTROL_IMAGE,
    GUICONTROL_LARGE_IMAGE,
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
    GUICONTROL_MULTI_IMAGE,
    GUICONTROL_GROUP,
    GUICONTROL_GROUPLIST,
    GUICONTROL_SCROLLBAR,
    GUICONTAINER_LIST,
    GUICONTAINER_WRAPLIST,
    GUICONTAINER_FIXEDLIST,
    GUICONTAINER_PANEL
  };
  GUICONTROLTYPES GetControlType() const { return ControlType; }

  enum GUIVISIBLE { HIDDEN = 0, DELAYED, VISIBLE };

#ifdef _DEBUG
  virtual void DumpTextureUse() {};
#endif
protected:
  virtual void Animate(DWORD currentTime);
  void UpdateStates(ANIMATION_TYPE type, ANIMATION_PROCESS currentProcess, ANIMATION_STATE currentState);
  bool SendWindowMessage(CGUIMessage &message);

  // navigation
  DWORD m_dwControlLeft;
  DWORD m_dwControlRight;
  DWORD m_dwControlUp;
  DWORD m_dwControlDown;
  vector<CStdString> m_leftActions;
  vector<CStdString> m_rightActions;
  vector<CStdString> m_upActions;
  vector<CStdString> m_downActions;

  float m_posX;
  float m_posY;
  float m_height;
  float m_width;
  CRect m_hitRect;
  D3DCOLOR m_diffuseColor;
  DWORD m_dwControlID;
  DWORD m_dwParentID;
  bool m_bHasFocus;
  bool m_bInvalidated;
  bool m_bAllocated;
  bool m_pulseOnSelect;
  GUICONTROLTYPES ControlType;

  CGUIControl *m_parentControl;   // our parent control if we're part of a group

  // visibility condition/state
  int m_visibleCondition;
  GUIVISIBLE m_visible;
  bool m_visibleFromSkinCondition;
  bool m_forceHidden;       // set from the code when a hidden operation is given - overrides m_visible
  bool m_allowHiddenFocus;
  bool m_hasRendered;
  // enable/disable state
  int m_enableCondition;
  bool m_enabled;

  // animation effects
  vector<CAnimation> m_animations;
  CPoint m_camera;
  bool m_hasCamera;
  TransformMatrix m_transform;
};

#endif
