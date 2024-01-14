/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
\file GUIWindow.h
\brief
*/

#include "GUIAction.h"
#include "GUIControlGroup.h"
#include <memory>
#include "threads/CriticalSection.h"

class CFileItem; typedef std::shared_ptr<CFileItem> CFileItemPtr;

#include <limits.h>
#include <map>
#include <vector>

enum RenderOrder {
  RENDER_ORDER_WINDOW = 0,
  RENDER_ORDER_DIALOG = 1,
  RENDER_ORDER_WINDOW_SCREENSAVER = INT_MAX,
  RENDER_ORDER_WINDOW_POINTER = INT_MAX - 1,
  RENDER_ORDER_WINDOW_DEBUG = INT_MAX - 2,
  RENDER_ORDER_DIALOG_TELETEXT = INT_MAX - 3
};

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
  enum LOAD_TYPE { LOAD_EVERY_TIME, LOAD_ON_GUI_INIT, KEEP_IN_MEMORY };

  CGUIWindow(int id, const std::string &xmlFile);
  ~CGUIWindow(void) override;

  bool Initialize();  // loads the window
  bool Load(const std::string& strFileName, bool bContainsPath = false);

  void CenterWindow();

  void DoProcess(unsigned int currentTime, CDirtyRegionList &dirtyregions) override;

  /*! \brief Main render function, called every frame.
   Window classes should override this only if they need to alter how something is rendered.
   General updating on a per-frame basis should be handled in FrameMove instead, as DoRender
   is not necessarily re-entrant.
   \sa FrameMove
   */
  void DoRender() override;

  /*! \brief Do any post render activities.
    Check if window closing animation is finished and finalize window closing.
   */
  void AfterRender();

  /*! \brief Main update function, called every frame prior to rendering
   Any window that requires updating on a frame by frame basis (such as to maintain
   timers and the like) should override this function.
   */
  virtual void FrameMove() {}

  void Close(bool forceClose = false, int nextWindowID = 0, bool enableSound = true, bool bWait = true);

  // OnAction() is called by our window manager.  We should process any messages
  // that should be handled at the window level in the derived classes, and any
  // unhandled messages should be dropped through to here where we send the message
  // on to the currently focused control.  Returns true if the action has been handled
  // and does not need to be passed further down the line (to our global action handlers)
  bool OnAction(const CAction &action) override;

  using CGUIControlGroup::OnBack;
  virtual bool OnBack(int actionID);
  using CGUIControlGroup::OnInfo;
  virtual bool OnInfo(int actionID) { return false; }

  /*! \brief Clear the background (if necessary) prior to rendering the window
   */
  virtual void ClearBackground();

  bool OnMove(int fromControl, int moveAction);
  bool OnMessage(CGUIMessage& message) override;

  bool ControlGroupHasFocus(int groupID, int controlID);
  void SetID(int id) override;
  virtual bool HasID(int controlID) const;
  const std::vector<int>& GetIDRange() const { return m_idRange; }
  int GetPreviousWindow() { return m_previousWindow; }
  CRect GetScaledBounds() const;
  void ClearAll() override;
  using CGUIControlGroup::AllocResources;
  virtual void AllocResources(bool forceLoad = false);
  void FreeResources(bool forceUnLoad = false) override;
  void DynamicResourceAlloc(bool bOnOff) override;
  virtual bool IsDialog() const { return false; }
  virtual bool IsDialogRunning() const { return false; }
  virtual bool IsModalDialog() const { return false; }
  virtual bool IsMediaWindow() const { return false; }
  virtual bool HasListItems() const { return false; }
  virtual bool IsSoundEnabled() const { return true; }
  virtual CFileItemPtr GetCurrentListItem(int offset = 0) { return CFileItemPtr(); }
  virtual int GetViewContainerID() const { return 0; }
  virtual int GetViewCount() const { return 0; }
  virtual bool CanBeActivated() const { return true; }
  virtual bool IsActive() const;
  void SetCoordsRes(const RESOLUTION_INFO& res) { m_coordsRes = res; }
  const RESOLUTION_INFO& GetCoordsRes() const { return m_coordsRes; }
  void SetLoadType(LOAD_TYPE loadType) { m_loadType = loadType; }
  LOAD_TYPE GetLoadType() { return m_loadType; }
  int GetRenderOrder() { return m_renderOrder; }
  void SetInitialVisibility() override;
  bool IsVisible() const override { return true; }; // windows are always considered visible as they implement their own
                                                   // versions of UpdateVisibility, and are deemed visible if they're in
                                                   // the window manager's active list.
  virtual bool HasVisibleControls() { return true; }; //Assume that window always has visible controls

  bool IsAnimating(ANIMATION_TYPE animType) override;

  /*!
   \brief Return if the window is a custom window
   \return true if the window is an custom window otherwise false
   */
  bool IsCustom() const { return m_custom; }

  /*!
   \brief Mark this window as custom window
   \param custom true if this window is a custom window, false if not
   */
  void SetCustom(bool custom) { m_custom = custom; }

  void DisableAnimations();

  virtual void ResetControlStates();
  void UpdateControlStats() override {}; // Do not count window itself

  void       SetRunActionsManually();
  void       RunLoadActions() const;
  void       RunUnloadActions() const;

  /*! \brief Set a property
   Sets the value of a property referenced by a key.
   \param key name of the property to set
   \param value value to set, may be a string, integer, boolean or double.
   \sa GetProperty
   */
  void SetProperty(const std::string &key, const CVariant &value);

  /*! \brief Retrieve a property
   \param key name of the property to retrieve
   \return value of the property, empty if it doesn't exist
   */
  CVariant GetProperty(const std::string &key) const;

  /*! \brief Clear a all the window's properties
   \sa SetProperty, HasProperty, GetProperty
   */
  void ClearProperties();

#ifdef _DEBUG
  void DumpTextureUse() override;
#endif
  bool HasSaveLastControl() const { return !m_defaultAlways; }

  virtual void OnDeinitWindow(int nextWindowID);
protected:
  EVENT_RESULT OnMouseEvent(const CPoint& point, const KODI::MOUSE::CMouseEvent& event) override;

  /*!
   \brief Load the window XML from the given path
   \param strPath the path to the window XML
   \param strLowerPath a lowered path to the window XML
   */
  virtual bool LoadXML(const std::string& strPath, const std::string &strLowerPath);

  /*!
   \brief Loads the window from the given XML element
   \param pRootElement the XML element
   \return true if the window is loaded from the given XML otherwise false.
   */
  virtual bool Load(TiXmlElement *pRootElement);

  /*!
   \brief Prepare the XML for load
   \param rootElement the original XML element
   \return the prepared XML (resolved includes, constants and expression)
   */
  virtual std::unique_ptr<TiXmlElement> Prepare(const std::unique_ptr<TiXmlElement>& rootElement);

  /*!
   \brief Check if window needs a (re)load. The window need to be (re)loaded when window is not loaded or include conditions values were changed
   */
  bool NeedLoad() const;

  virtual void SetDefaults();
  virtual void OnWindowUnload() {}
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();
  void Close_Internal(bool forceClose = false, int nextWindowID = 0, bool enableSound = true);
  EVENT_RESULT OnMouseAction(const CAction &action);
  bool Animate(unsigned int currentTime) override;
  bool CheckAnimation(ANIMATION_TYPE animType) override;

  // control state saving on window close
  virtual void SaveControlStates();
  virtual void RestoreControlStates();

  // methods for updating controls and sending messages
  void OnEditChanged(int id, std::string &text);
  bool SendMessage(int message, int id, int param1 = 0, int param2 = 0);

  void LoadControl(TiXmlElement* pControl, CGUIControlGroup *pGroup, const CRect &rect);

  std::vector<int> m_idRange;
  RESOLUTION_INFO m_coordsRes; // resolution that the window coordinates are in.
  bool m_needsScaling;
  bool m_windowLoaded;  // true if the window's xml file has been loaded
  LOAD_TYPE m_loadType;
  bool m_dynamicResourceAlloc;
  bool m_closing;
  bool m_active;        // true if window is active or dialog is running
  KODI::GUILIB::GUIINFO::CGUIInfoColor m_clearBackground; // colour to clear the window

  int m_renderOrder;      // for render order of dialogs

  /*! \brief Grabs the window's top,left position in skin coordinates
   The window origin may change based on `<origin>` tag conditions in the skin.

   \return the window's origin in skin coordinates
   */
  CPoint GetPosition() const override;
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

  /*! \brief window root xml definition after resolving any skin includes.
    Stored to avoid parsing the XML every time the window is loaded.
   */
  std::unique_ptr<TiXmlElement> m_windowXMLRootElement;

  bool m_manualRunActions;

  int m_exclusiveMouseControl; ///< \brief id of child control that wishes to receive all mouse events \sa GUI_MSG_EXCLUSIVE_MOUSE

  int m_menuControlID;
  int m_menuLastFocusedControlID;
  bool m_custom;

private:
  std::map<std::string, CVariant, icompare> m_mapProperties;
  std::map<INFO::InfoPtr, bool> m_xmlIncludeConditions; ///< \brief used to store conditions used to resolve includes for this window
};

