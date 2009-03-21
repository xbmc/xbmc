/*!
\file GUIWindow.h
\brief
*/

#ifndef GUILIB_GUIWINDOW_H
#define GUILIB_GUIWINDOW_H

#pragma once

/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "GUIControlGroup.h"
#include "boost/shared_ptr.hpp"

class CFileItem; typedef boost::shared_ptr<CFileItem> CFileItemPtr;

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
class TiXmlDocument;

class COrigin
{
public:
  COrigin()
  {
    x = y = 0;
    condition = 0;
  };
  float x;
  float y;
  int condition;
};

/*!
 \ingroup winmsg
 \brief
 */
class CGUIWindow : public CGUIControlGroup
{
public:
  enum WINDOW_TYPE { WINDOW = 0, MODAL_DIALOG, MODELESS_DIALOG, BUTTON_MENU, SUB_MENU };

  CGUIWindow(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIWindow(void);

  bool Initialize();  // loads the window
  bool Load(const CStdString& strFileName, bool bContainsPath = false);

  void CenterWindow();
  virtual void Render();

  // Close should never be called on this base class (only on derivatives) - its here so that window-manager can use a general close
  virtual void Close(bool forceClose = false);

  // OnAction() is called by our window manager.  We should process any messages
  // that should be handled at the window level in the derived classes, and any
  // unhandled messages should be dropped through to here where we send the message
  // on to the currently focused control.  Returns true if the action has been handled
  // and does not need to be passed further down the line (to our global action handlers)
  virtual bool OnAction(const CAction &action);

  virtual bool OnMouse(const CPoint &point);
  bool HandleMouse(CGUIControl *pControl, const CPoint &point);
  bool OnMove(int fromControl, int moveAction);
  virtual bool OnMessage(CGUIMessage& message);

  bool ControlGroupHasFocus(int groupID, int controlID);
  virtual bool HasID(DWORD dwID) { return (dwID >= m_dwControlID && dwID < m_dwControlID + m_dwIDRange); };
  void SetIDRange(DWORD dwRange) { m_dwIDRange = dwRange; };
  DWORD GetIDRange() const { return m_dwIDRange; };
  DWORD GetPreviousWindow() { return m_previousWindow; };
  FRECT GetScaledBounds() const;
  virtual void ClearAll();
  virtual void AllocResources(bool forceLoad = false);
  virtual void FreeResources(bool forceUnLoad = false);
  virtual void DynamicResourceAlloc(bool bOnOff);
  virtual bool IsDialog() const { return false; };
  virtual bool IsDialogRunning() const { return false; };
  virtual bool IsModalDialog() const { return false; };
  virtual bool IsMediaWindow() const { return false; };
  virtual bool HasListItems() const { return false; };
  virtual CFileItemPtr GetCurrentListItem(int offset = 0) { return CFileItemPtr(); };
  virtual int GetViewContainerID() const { return 0; };
  virtual bool IsActive() const;
  void SetCoordsRes(RESOLUTION res) { m_coordsRes = res; };
  RESOLUTION GetCoordsRes() const { return m_coordsRes; };
  void SetXMLFile(const CStdString &xmlFile) { m_xmlFile = xmlFile; };
  const CStdString &GetXMLFile() const { return m_xmlFile; };
  void LoadOnDemand(bool loadOnDemand) { m_loadOnDemand = loadOnDemand; };
  bool GetLoadOnDemand() { return m_loadOnDemand; }
  int GetRenderOrder() { return m_renderOrder; };
  virtual void SetInitialVisibility();

  enum OVERLAY_STATE { OVERLAY_STATE_PARENT_WINDOW=0, OVERLAY_STATE_SHOWN, OVERLAY_STATE_HIDDEN };

  OVERLAY_STATE GetOverlayState() const { return m_overlayState; };

  virtual bool IsAnimating(ANIMATION_TYPE animType);
  void DisableAnimations();

  virtual void ResetControlStates();

  void SetProperty(const CStdString &strKey, const char *strValue);
  void SetProperty(const CStdString &strKey, const CStdString &strValue);
  void SetProperty(const CStdString &strKey, int nVal);
  void SetProperty(const CStdString &strKey, bool bVal);
  void SetProperty(const CStdString &strKey, double dVal);

  CStdString GetProperty(const CStdString &strKey) const;
  int        GetPropertyInt(const CStdString &strKey) const;
  bool       GetPropertyBOOL(const CStdString &strKey) const;
  double     GetPropertyDouble(const CStdString &strKey) const;

  void ClearProperties();
  void ClearProperty(const CStdString &strKey);

#ifdef _DEBUG
  void DumpTextureUse();
#endif

  bool HasSaveLastControl() const { return m_saveLastControl; };

protected:
  virtual bool LoadXML(const CStdString& strPath, const CStdString &strLowerPath);  ///< Loads from the given file
  bool Load(TiXmlDocument &xmlDoc);                 ///< Loads from the given XML document
  virtual void LoadAdditionalTags(TiXmlElement *root) {}; ///< Load additional information from the XML document

  virtual void SetDefaults();
  virtual void OnWindowUnload() {}
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual bool OnMouseAction();
  virtual bool RenderAnimation(DWORD time);
  virtual bool CheckAnimation(ANIMATION_TYPE animType);

  CAnimation *GetAnimation(ANIMATION_TYPE animType, bool checkConditions = true);

  // control state saving on window close
  virtual void SaveControlStates();
  virtual void RestoreControlStates();
  void AddControlGroup(int id);

  // methods for updating controls and sending messages
  void OnEditChanged(int id, CStdString &text);
  bool SendMessage(DWORD message, DWORD id, DWORD param1 = 0, DWORD param2 = 0);

  typedef GUIEvent<CGUIMessage&> CLICK_EVENT;
  typedef std::map<int, CLICK_EVENT> MAPCONTROLCLICKEVENTS;
  MAPCONTROLCLICKEVENTS m_mapClickEvents;

  typedef GUIEvent<CGUIMessage&> SELECTED_EVENT;
  typedef std::map<int, SELECTED_EVENT> MAPCONTROLSELECTEDEVENTS;
  MAPCONTROLSELECTEDEVENTS m_mapSelectedEvents;

  void LoadControl(TiXmlElement* pControl, CGUIControlGroup *pGroup);

//#ifdef PRE_SKIN_VERSION_9_10_COMPATIBILITY
  void ChangeButtonToEdit(int id, bool singleLabel = false);
//#endif

  std::vector<CGUIControl*> m_vecControls;
  typedef std::vector<CGUIControl*>::iterator ivecControls;
  typedef std::vector<CGUIControl*>::const_iterator ciControls;
  DWORD m_dwIDRange;
  bool m_bRelativeCoords;
  OVERLAY_STATE m_overlayState;
  RESOLUTION m_coordsRes; // resolution that the window coordinates are in.
  bool m_needsScaling;
  CStdString m_xmlFile;  // xml file to load
  bool m_windowLoaded;  // true if the window's xml file has been loaded
  bool m_loadOnDemand;  // true if the window should be loaded only as needed
  bool m_isDialog;      // true if we have a dialog, false otherwise.
  bool m_dynamicResourceAlloc;

  int m_renderOrder;      // for render order of dialogs

  std::vector<COrigin> m_origins;  // positions of dialogs depending on base window

  // control states
  bool m_saveLastControl;
  int m_lastControlID;
  std::vector<CControlState> m_controlStates;
  DWORD m_previousWindow;

  bool m_animationsEnabled;
  struct icompare
  {
    bool operator()(const CStdString &s1, const CStdString &s2) const
    {
      return s1.CompareNoCase(s2) < 0;
    }
  };

  std::map<CStdString, CStdString, icompare> m_mapProperties;

};

#endif
