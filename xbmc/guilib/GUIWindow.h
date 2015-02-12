/*!
\file GUIWindow.h
\brief
*/

#ifndef GUILIB_GUIWINDOW_H
#define GUILIB_GUIWINDOW_H

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

#include "GUIControlGroup.h"
#include <memory>
#include "threads/CriticalSection.h"

class CFileItem; typedef std::shared_ptr<CFileItem> CFileItemPtr;

#include "GUICallback.h"  // for GUIEvent

#include <map>
#include <vector>

#define ON_CLICK_MESSAGE(i,c,m) \
{ \
 GUIEventHandler<c, CGUIMessage&> clickHandler(this, &m); \
 m_mapClickEvents[i] = clickHandler; \
} \

#define ON_SELECTED_MESSAGE(i,c,m) \
{ \
 GUIEventHandler<c, CGUIMessage&> selectedHandler(this, &m); \
 m_mapSelectedEvents[i] = selectedHandler; \
} \

// forward
class TiXmlNode;
class TiXmlElement;
class CXBMCTinyXML;
class CVariant;

class COrigin
{
public:
  COrigin()
  {
    x = y = 0;
  };
  float x;
  float y;
  INFO::InfoPtr condition;
};

/*!
 \ingroup winmsg
 \brief
 */
class CGUIWindow : public CGUIControlGroup, protected CCriticalSection
{
public:

  enum WINDOW_TYPE { WINDOW = 0, MODAL_DIALOG, MODELESS_DIALOG, BUTTON_MENU, SUB_MENU };
  enum LOAD_TYPE { LOAD_EVERY_TIME, LOAD_ON_GUI_INIT, KEEP_IN_MEMORY };

  CGUIWindow(int id, const std::string &xmlFile);
  virtual ~CGUIWindow(void);

  bool Initialize();  // loads the window
  bool Load(const std::string& strFileName, bool bContainsPath = false);

  void CenterWindow();

  virtual void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions);
  
  /*! \brief Main render function, called every frame.
   Window classes should override this only if they need to alter how something is rendered.
   General updating on a per-frame basis should be handled in FrameMove instead, as DoRender
   is not necessarily re-entrant.
   \sa FrameMove
   */
  virtual void DoRender();

  /*! \brief Do any post render activities.
    Check if window closing animation is finished and finalize window closing.
   */
  void AfterRender();
  
  /*! \brief Main update function, called every frame prior to rendering
   Any window that requires updating on a frame by frame basis (such as to maintain
   timers and the like) should override this function.
   */
  virtual void FrameMove() {};

  void Close(bool forceClose = false, int nextWindowID = 0, bool enableSound = true, bool bWait = true);

  // OnAction() is called by our window manager.  We should process any messages
  // that should be handled at the window level in the derived classes, and any
  // unhandled messages should be dropped through to here where we send the message
  // on to the currently focused control.  Returns true if the action has been handled
  // and does not need to be passed further down the line (to our global action handlers)
  virtual bool OnAction(const CAction &action);
  
  virtual bool OnBack(int actionID);

  /*! \brief Clear the background (if necessary) prior to rendering the window
   */
  virtual void ClearBackground();

  bool OnMove(int fromControl, int moveAction);
  virtual bool OnMessage(CGUIMessage& message);

  bool ControlGroupHasFocus(int groupID, int controlID);
  virtual void SetID(int id);
  virtual bool HasID(int controlID) const;
  const std::vector<int>& GetIDRange() const { return m_idRange; };
  int GetPreviousWindow() { return m_previousWindow; };
  CRect GetScaledBounds() const;
  virtual void ClearAll();
  virtual void AllocResources(bool forceLoad = false);
  virtual void FreeResources(bool forceUnLoad = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDialog() const { return false; };
  virtual bool IsDialogRunning() const { return false; };
  virtual bool IsModalDialog() const { return false; };
  virtual bool IsMediaWindow() const { return false; };
  virtual bool HasListItems() const { return false; };
  virtual bool IsSoundEnabled() const { return true; };
  virtual CFileItemPtr GetCurrentListItem(int offset = 0) { return CFileItemPtr(); };
  virtual int GetViewContainerID() const { return 0; };
  virtual bool IsActive() const;
  void SetCoordsRes(const RESOLUTION_INFO &res) { m_coordsRes = res; };
  const RESOLUTION_INFO &GetCoordsRes() const { return m_coordsRes; };
  void SetLoadType(LOAD_TYPE loadType) { m_loadType = loadType; };
  LOAD_TYPE GetLoadType() { return m_loadType; } const
  int GetRenderOrder() { return m_renderOrder; };
  virtual void SetInitialVisibility();
  virtual bool IsVisible() const { return true; }; // windows are always considered visible as they implement their own
                                                   // versions of UpdateVisibility, and are deemed visible if they're in
                                                   // the window manager's active list.

  enum OVERLAY_STATE { OVERLAY_STATE_PARENT_WINDOW=0, OVERLAY_STATE_SHOWN, OVERLAY_STATE_HIDDEN };

  OVERLAY_STATE GetOverlayState() const { return m_overlayState; };

  virtual bool IsAnimating(ANIMATION_TYPE animType);
  void DisableAnimations();

  virtual void ResetControlStates();

  void       SetRunActionsManually();
  void       RunLoadActions();
  void       RunUnloadActions();

  /*! \brief Set a property
   Sets the value of a property referenced by a key.
   \param key name of the property to set
   \param value value to set, may be a string, integer, boolean or double.
   \sa GetProperty
   */
  void SetProperty(const std::string &key, const CVariant &value);

  /*! \brief Retreive a property
   \param key name of the property to retrieve
   \return value of the property, empty if it doesn't exist
   */
  CVariant GetProperty(const std::string &key) const;

  /*! \brief Clear a all the window's properties
   \sa SetProperty, HasProperty, GetProperty
   */
  void ClearProperties();

  void DumpTextureUse();

  bool HasSaveLastControl() const { return !m_defaultAlways; };

  virtual void OnDeinitWindow(int nextWindowID);
protected:
  virtual EVENT_RESULT OnMouseEvent(const CPoint &point, const CMouseEvent &event);
  virtual bool LoadXML(const std::string& strPath, const std::string &strLowerPath);  ///< Loads from the given file
  bool Load(TiXmlElement *pRootElement);                 ///< Loads from the given XML root element
  /*! \brief Check if XML file needs (re)loading
   XML file has to be (re)loaded when window is not loaded or include conditions values were changed
   */
  bool NeedXMLReload();
  virtual void LoadAdditionalTags(TiXmlElement *root) {}; ///< Load additional information from the XML document

  virtual void SetDefaults();
  virtual void OnWindowUnload() {}
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();
  void Close_Internal(bool forceClose = false, int nextWindowID = 0, bool enableSound = true);
  EVENT_RESULT OnMouseAction(const CAction &action);
  virtual bool Animate(unsigned int currentTime);
  virtual bool CheckAnimation(ANIMATION_TYPE animType);

  CAnimation *GetAnimation(ANIMATION_TYPE animType, bool checkConditions = true);

  // control state saving on window close
  virtual void SaveControlStates();
  virtual void RestoreControlStates();

  // methods for updating controls and sending messages
  void OnEditChanged(int id, std::string &text);
  bool SendMessage(int message, int id, int param1 = 0, int param2 = 0);

  typedef GUIEvent<CGUIMessage&> CLICK_EVENT;
  typedef std::map<int, CLICK_EVENT> MAPCONTROLCLICKEVENTS;
  MAPCONTROLCLICKEVENTS m_mapClickEvents;

  typedef GUIEvent<CGUIMessage&> SELECTED_EVENT;
  typedef std::map<int, SELECTED_EVENT> MAPCONTROLSELECTEDEVENTS;
  MAPCONTROLSELECTEDEVENTS m_mapSelectedEvents;

  void LoadControl(TiXmlElement* pControl, CGUIControlGroup *pGroup, const CRect &rect);

  std::vector<int> m_idRange;
  OVERLAY_STATE m_overlayState;
  RESOLUTION_INFO m_coordsRes; // resolution that the window coordinates are in.
  bool m_needsScaling;
  bool m_windowLoaded;  // true if the window's xml file has been loaded
  LOAD_TYPE m_loadType;
  bool m_isDialog;      // true if we have a dialog, false otherwise.
  bool m_dynamicResourceAlloc;
  bool m_closing;
  bool m_active;        // true if window is active or dialog is running
  CGUIInfoColor m_clearBackground; // colour to clear the window

  int m_renderOrder;      // for render order of dialogs

  /*! \brief Grabs the window's top,left position in skin coordinates
   The window origin may change based on <origin> tag conditions in the skin.

   \return the window's origin in skin coordinates
   */
  virtual CPoint GetPosition() const;
  std::vector<COrigin> m_origins;  // positions of dialogs depending on base window

  // control states
  int m_lastControlID;
  std::vector<CControlState> m_controlStates;
  int m_previousWindow;

  bool m_animationsEnabled;
  struct icompare
  {
    bool operator()(const std::string &s1, const std::string &s2) const;
  };

  CGUIAction m_loadActions;
  CGUIAction m_unloadActions;

  TiXmlElement* m_windowXMLRootElement;

  bool m_manualRunActions;

  int m_exclusiveMouseControl; ///< \brief id of child control that wishes to receive all mouse events \sa GUI_MSG_EXCLUSIVE_MOUSE

private:
  std::map<std::string, CVariant, icompare> m_mapProperties;
  std::map<INFO::InfoPtr, bool> m_xmlIncludeConditions; ///< \brief used to store conditions used to resolve includes for this window
};

#endif
