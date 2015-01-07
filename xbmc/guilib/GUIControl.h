/*!
\file GUIControl.h
\brief
*/

#ifndef GUILIB_GUICONTROL_H
#define GUILIB_GUICONTROL_H
#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "GraphicContext.h" // needed by any rendering operation (all controls)
#include "GUIMessage.h"     // needed by practically all controls
#include "VisibleEffect.h"  // needed for the CAnimation members
#include "GUIInfoTypes.h"   // needed for CGUIInfoColor to handle infolabel'ed colors
#include "DirtyRegion.h"
#include "GUIAction.h"

class CGUIListItem; // forward
class CAction;
class CMouseEvent;

enum ORIENTATION { HORIZONTAL = 0, VERTICAL };

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
 \brief Results of OnMouseEvent()
 Any value not equal to EVENT_RESULT_UNHANDLED indicates that the event was handled.
 */
enum EVENT_RESULT { EVENT_RESULT_UNHANDLED                      = 0x00,
                    EVENT_RESULT_HANDLED                        = 0x01,
                    EVENT_RESULT_PAN_HORIZONTAL                 = 0x02,
                    EVENT_RESULT_PAN_VERTICAL                   = 0x04,
                    EVENT_RESULT_PAN_VERTICAL_WITHOUT_INERTIA   = 0x08,
                    EVENT_RESULT_PAN_HORIZONTAL_WITHOUT_INERTIA = 0x10,
                    EVENT_RESULT_ROTATE                         = 0x20,
                    EVENT_RESULT_ZOOM                           = 0x40,
                    EVENT_RESULT_SWIPE                          = 0x80
};

/*!
 \ingroup controls
 \brief Base class for controls
 */
class CGUIControl
{
public:
  CGUIControl();
  CGUIControl(int parentID, int controlID, float posX, float posY, float width, float height);
  virtual ~CGUIControl(void);
  virtual CGUIControl *Clone() const=0;

  virtual void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void Process(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  virtual void DoRender();
  virtual void Render() {};

  /*! \brief Returns whether or not we have processed */
  bool HasProcessed() const { return m_hasProcessed; };

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
  virtual bool OnBack();
  virtual void OnNextControl();
  virtual void OnPrevControl();
  virtual void OnFocus() {};
  virtual void OnUnFocus() {};

  /*! \brief React to a mouse event

   Mouse events are sent from the window to all controls, and each control can react based on the event
   and location of the event.

   \param point the location in transformed skin coordinates from the upper left corner of the parent control.
   \param event the mouse event to perform
   \return EVENT_RESULT corresponding to whether the control handles this event
   \sa HitTest, CanFocusFromPoint, CMouseEvent, EVENT_RESULT
   */
  virtual EVENT_RESULT SendMouseEvent(const CPoint &point, const CMouseEvent &event);

  /*! \brief Perform a mouse action

   Mouse actions are sent from the window to all controls, and each control can react based on the event
   and location of the actions.

   \param point the location in transformed skin coordinates from the upper left corner of the parent control.
   \param event the mouse event to perform
   \return EVENT_RESULT corresponding to whether the control handles this event
   \sa SendMouseEvent, HitTest, CanFocusFromPoint, CMouseEvent
   */
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event) { return EVENT_RESULT_UNHANDLED; };

  /*! \brief Unfocus the control if the given point on screen is not within it's boundary
   \param point the location in transformed skin coordinates from the upper left corner of the parent control.
   \sa CanFocusFromPoint
   */
  virtual void UnfocusFromPoint(const CPoint &point);

  /*! \brief Used to test whether the point is inside a control.
   \param point location to test
   \return true if the point is inside the bounds of this control.
   \sa SetHitRect
   */
  virtual bool HitTest(const CPoint &point) const;

  virtual bool OnMessage(CGUIMessage& message);
  virtual int GetID(void) const;
  virtual void SetID(int id) { m_controlID = id; };
  virtual bool HasID(int id) const;
  virtual bool HasVisibleID(int id) const;
  int GetParentID() const;
  virtual bool HasFocus() const;
  virtual void AllocResources();
  virtual void FreeResources(bool immediately = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDynamicallyAllocated() { return false; };
  virtual bool CanFocus() const;
  virtual bool IsVisible() const;
  bool IsVisibleFromSkin() const { return m_visibleFromSkinCondition; };
  virtual bool IsDisabled() const;
  virtual void SetPosition(float posX, float posY);
  virtual void SetHitRect(const CRect &rect);
  virtual void SetCamera(const CPoint &camera);
  bool SetColorDiffuse(const CGUIInfoColor &color);
  CPoint GetRenderPosition() const;
  virtual float GetXPosition() const;
  virtual float GetYPosition() const;
  virtual float GetWidth() const;
  virtual float GetHeight() const;

  void MarkDirtyRegion();

  /*! \brief return the render region in screen coordinates of this control
   */
  const CRect &GetRenderRegion() const { return m_renderRegion; };
  /*! \brief calculate the render region in parentcontrol coordinates of this control
   Called during process to update m_renderRegion
   */
  virtual CRect CalcRenderRegion() const;

  /*! \brief Set actions to perform on navigation
   \param actions ActionMap of actions
   \sa SetNavigationAction
   */
  typedef std::map<int, CGUIAction> ActionMap;
  void SetNavigationActions(const ActionMap &actions);

  /*! \brief Set actions to perform on navigation
   Navigations are set if replace is true or if there is no previously set action
   \param actionID id of the nagivation action
   \param actions CGUIAction to set
   \param replace Actions are set only if replace is true or there is no previously set action.  Defaults to true
   \sa SetNavigationActions
   */
  void SetNavigationAction(int actionID, const CGUIAction &action, bool replace = true);

  /*! \brief Get an action the control can be perform.
   \param action the actionID to retrieve.
   */
  CGUIAction GetNavigateAction(int actionID) const;

  /*! \brief  Start navigating in given direction.
   */
  bool Navigate(int direction) const;
  virtual void SetFocus(bool focus);
  virtual void SetWidth(float width);
  virtual void SetHeight(float height);
  virtual void SetVisible(bool bVisible, bool setVisState = false);
  void SetVisibleCondition(const std::string &expression, const std::string &allowHiddenFocus = "");
  bool HasVisibleCondition() const { return m_visibleCondition != NULL; };
  void SetEnableCondition(const std::string &expression);
  virtual void UpdateVisibility(const CGUIListItem *item = NULL);
  virtual void SetInitialVisibility();
  virtual void SetEnabled(bool bEnable);
  virtual void SetInvalid() { m_bInvalidated = true; };
  virtual void SetPulseOnSelect(bool pulse) { m_pulseOnSelect = pulse; };
  virtual std::string GetDescription() const { return ""; };

  void SetAnimations(const std::vector<CAnimation> &animations);
  const std::vector<CAnimation> &GetAnimations() const { return m_animations; };

  virtual void QueueAnimation(ANIMATION_TYPE anim);
  virtual bool IsAnimating(ANIMATION_TYPE anim);
  virtual bool HasAnimation(ANIMATION_TYPE anim);
  CAnimation *GetAnimation(ANIMATION_TYPE type, bool checkConditions = true);
  virtual void ResetAnimation(ANIMATION_TYPE type);
  virtual void ResetAnimations();

  // push information updates
  virtual void UpdateInfo(const CGUIListItem *item = NULL) {};
  virtual void SetPushUpdates(bool pushUpdates) { m_pushedUpdates = pushUpdates; };

  virtual bool IsGroup() const { return false; };
  virtual bool IsContainer() const { return false; };
  virtual bool GetCondition(int condition, int data) const { return false; };

  void SetParentControl(CGUIControl *control) { m_parentControl = control; };
  CGUIControl *GetParentControl(void) const { return m_parentControl; };
  virtual void SaveStates(std::vector<CControlState> &states);

  enum GUICONTROLTYPES {
    GUICONTROL_UNKNOWN,
    GUICONTROL_BUTTON,
    GUICONTROL_CHECKMARK,
    GUICONTROL_FADELABEL,
    GUICONTROL_IMAGE,
    GUICONTROL_BORDEREDIMAGE,
    GUICONTROL_LARGE_IMAGE,
    GUICONTROL_LABEL,
    GUICONTROL_LISTGROUP,
    GUICONTROL_PROGRESS,
    GUICONTROL_RADIO,
    GUICONTROL_RSS,
    GUICONTROL_SELECTBUTTON,
    GUICONTROL_SLIDER,
    GUICONTROL_SETTINGS_SLIDER,
    GUICONTROL_SPIN,
    GUICONTROL_SPINEX,
    GUICONTROL_TEXTBOX,
    GUICONTROL_TOGGLEBUTTON,
    GUICONTROL_VIDEO,
    GUICONTROL_MOVER,
    GUICONTROL_RESIZE,
    GUICONTROL_EDIT,
    GUICONTROL_VISUALISATION,
    GUICONTROL_RENDERADDON,
    GUICONTROL_MULTI_IMAGE,
    GUICONTROL_GROUP,
    GUICONTROL_GROUPLIST,
    GUICONTROL_SCROLLBAR,
    GUICONTROL_LISTLABEL,
    GUICONTROL_MULTISELECT,
    GUICONTAINER_LIST,
    GUICONTAINER_WRAPLIST,
    GUICONTAINER_FIXEDLIST,
    GUICONTAINER_EPGGRID,
    GUICONTAINER_PANEL
  };
  GUICONTROLTYPES GetControlType() const { return ControlType; }

  enum GUIVISIBLE { HIDDEN = 0, DELAYED, VISIBLE };

  enum GUISCROLLVALUE { FOCUS = 0, NEVER, ALWAYS };

#ifdef _DEBUG
  virtual void DumpTextureUse() {};
#endif
protected:
  /*!
   \brief Return the coordinates of the top left of the control, in the control's parent coordinates
   \return The top left coordinates of the control
   */
  virtual CPoint GetPosition() const { return CPoint(GetXPosition(), GetYPosition()); };

  /*! \brief Called when the mouse is over the control.
   Default implementation selects the control.
   \param point location of the mouse in transformed skin coordinates
   \return true if handled, false otherwise.
   */
  virtual bool OnMouseOver(const CPoint &point);

  /*! \brief Test whether we can focus a control from a point on screen
   \param point the location in vanilla skin coordinates from the upper left corner of the parent control.
   \return true if the control can be focused from this location
   \sa UnfocusFromPoint, HitRect
   */
  virtual bool CanFocusFromPoint(const CPoint &point) const;

  virtual bool UpdateColors();
  virtual bool Animate(unsigned int currentTime);
  virtual bool CheckAnimation(ANIMATION_TYPE animType);
  void UpdateStates(ANIMATION_TYPE type, ANIMATION_PROCESS currentProcess, ANIMATION_STATE currentState);
  bool SendWindowMessage(CGUIMessage &message) const;

  // navigation and actions
  ActionMap m_actions;

  float m_posX;
  float m_posY;
  float m_height;
  float m_width;
  CRect m_hitRect;
  CGUIInfoColor m_diffuseColor;
  int m_controlID;
  int m_parentID;
  bool m_bHasFocus;
  bool m_bInvalidated;
  bool m_bAllocated;
  bool m_pulseOnSelect;
  GUICONTROLTYPES ControlType;

  CGUIControl *m_parentControl;   // our parent control if we're part of a group

  // visibility condition/state
  INFO::InfoPtr m_visibleCondition;
  GUIVISIBLE m_visible;
  bool m_visibleFromSkinCondition;
  bool m_forceHidden;       // set from the code when a hidden operation is given - overrides m_visible
  CGUIInfoBool m_allowHiddenFocus;
  bool m_hasProcessed;
  // enable/disable state
  INFO::InfoPtr m_enableCondition;
  bool m_enabled;

  bool m_pushedUpdates;

  // animation effects
  std::vector<CAnimation> m_animations;
  CPoint m_camera;
  bool m_hasCamera;
  TransformMatrix m_transform;
  TransformMatrix m_cachedTransform; // Contains the absolute transform the control

  bool  m_controlIsDirty;
  CRect m_renderRegion;         // In screen coordinates
};

#endif
