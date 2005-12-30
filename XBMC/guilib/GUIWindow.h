/*!
\file GUIWindow.h
\brief 
*/

#ifndef GUILIB_GUIWINDOW_H
#define GUILIB_GUIWINDOW_H

#pragma once

#include "GUIControl.h"

#include <map>
#include <vector>

#define ON_CLICK_MESSAGE(i,c,m) \
{ \
 GUIEventHandler<c, CGUIMessage&> clickHandler(this, m); \
 m_mapClickEvents[i] = clickHandler; \
} \

#define ON_SELECTED_MESSAGE(i,c,m) \
{ \
 GUIEventHandler<c, CGUIMessage&> selectedHandler(this, m); \
 m_mapSelectedEvents[i] = selectedHandler; \
} \

// forward
class TiXmlNode;
class TiXmlElement;

class CPosition
{
public:
  CGUIControl* pControl;
  int x;
  int y;
};
/*!
 \ingroup winmsg
 \brief 
 */
class CGUIWindow
{
public:
  enum WINDOW_TYPE { WINDOW = 0, MODAL_DIALOG, MODELESS_DIALOG, BUTTON_MENU, SUB_MENU };

  CGUIWindow(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIWindow(void);

  void Initialize();  // loads the window
  virtual bool Load(const CStdString& strFileName, bool bContainsPath = false);
  virtual bool Load(const TiXmlElement* pRootElement, RESOLUTION resToUse);
  virtual void SetPosition(int iPosX, int iPosY);
  void CenterWindow();
  virtual void Render();

  // OnAction() is called by our window manager.  We should process any messages
  // that should be handled at the window level in the derived classes, and any
  // unhandled messages should be dropped through to here where we send the message
  // on to the currently focused control.  Returns true if the action has been handled
  // and does not need to be passed further down the line (to our global action handlers)
  virtual bool OnAction(const CAction &action);

  void OnMouseAction();
  virtual bool OnMouse();
  bool HandleMouse(CGUIControl *pControl);
  virtual bool OnMessage(CGUIMessage& message);
  void Add(CGUIControl* pControl);
  void Remove(DWORD dwId);
  int GetFocusControl();
  void SelectNextControl();
  void SelectPreviousControl();
  void SetID(DWORD dwID);
  virtual DWORD GetID(void) const;
  virtual bool HasID(DWORD dwID) { return (dwID >= m_dwWindowId && dwID < m_dwWindowId + m_dwIDRange); };
  void SetIDRange(DWORD dwRange) { m_dwIDRange = dwRange; };
  DWORD GetPreviousWindowID(void) const;
  void SetPreviousWindowID(DWORD id) { m_dwPreviousWindowId = id; };
  DWORD GetWidth() { return m_dwWidth; };
  DWORD GetHeight() { return m_dwHeight; };
  int GetPosX() { return m_iPosX; };
  int GetPosY() { return m_iPosY; };
  const CGUIControl* GetControl(int iControl) const;
  void ClearAll();
  int GetFocusedControl() const;
  virtual void AllocResources(bool forceLoad = false);
  virtual void FreeResources(bool forceUnLoad = false);
  void DynamicResourceAlloc(bool bOnOff);
  virtual void ResetAllControls();
  static void FlushReferenceCache();
  virtual bool IsDialog() const { return false;};
  virtual bool IsMediaWindow() const { return false; };
  virtual bool IsActive() const;
  void SetCoordsRes(RESOLUTION res) { m_coordsRes = res; };
  RESOLUTION GetCoordsRes() const { return m_coordsRes; };
  int GetVisibleCondition() const { return m_visibleCondition; };
  void SetXMLFile(const CStdString &xmlFile) { m_xmlFile = xmlFile; };
  void LoadOnDemand(bool loadOnDemand) { m_loadOnDemand = loadOnDemand; };
  bool GetLoadOnDemand() { return m_loadOnDemand; }
  int GetRenderOrder() { return m_renderOrder; };
  EFFECT_STATE GetEffectState();
  void SetControlVisibility();

  enum OVERLAY_STATE { OVERLAY_STATE_PARENT_WINDOW=0, OVERLAY_STATE_SHOWN, OVERLAY_STATE_HIDDEN };

  OVERLAY_STATE GetOverlayState() const { return m_overlayState; };
  void ChangeControlID(DWORD oldID, DWORD newID, CGUIControl::GUICONTROLTYPES type);

protected:
  virtual void OnWindowUnload() {}
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();
  struct stReferenceControl
  {
    char m_szType[128];
    CGUIControl* m_pControl;
  };

  typedef GUIEvent<CGUIMessage&> CLICK_EVENT;
  typedef std::map<int, CLICK_EVENT> MAPCONTROLCLICKEVENTS;
  MAPCONTROLCLICKEVENTS m_mapClickEvents;

  typedef GUIEvent<CGUIMessage&> SELECTED_EVENT;
  typedef std::map<int, SELECTED_EVENT> MAPCONTROLSELECTEDEVENTS;
  MAPCONTROLSELECTEDEVENTS m_mapSelectedEvents;

  typedef vector<struct stReferenceControl> VECREFERENCECONTOLS;
  typedef vector<struct stReferenceControl>::iterator IVECREFERENCECONTOLS;
  bool LoadReference(VECREFERENCECONTOLS& controls);
  void LoadControl(const TiXmlNode* pControl, int iGroup, VECREFERENCECONTOLS& referencecontrols, RESOLUTION& resToUse);
  static CStdString CacheFilename;
  static VECREFERENCECONTOLS ControlsCache;

  vector<CGUIControl*> m_vecControls;
  typedef std::vector<CGUIControl*>::iterator ivecControls;
  DWORD m_dwWindowId;
  DWORD m_dwIDRange;
  DWORD m_dwPreviousWindowId;
  DWORD m_dwDefaultFocusControlID;
  bool m_bRelativeCoords;
  int m_iPosX;
  int m_iPosY;
  DWORD m_dwWidth;
  DWORD m_dwHeight;
  vector<int> m_vecGroups;
  OVERLAY_STATE m_overlayState;
  bool m_WindowAllocated;
  RESOLUTION m_coordsRes; // resolution that the window coordinates are in.
  bool m_needsScaling;
  CStdString m_xmlFile;  // xml file to load
  bool m_windowLoaded;  // true if the window's xml file has been loaded
  bool m_loadOnDemand;  // true if the window should be loaded only as needed
  bool m_dynamicResourceAlloc;
  int m_visibleCondition;

  EFFECT_STATE m_effectState;
  EFFECT_STATE m_queueState;
  CVisibleEffect m_effect;
  DWORD m_effectStart;
  float m_effectAmount;
  CAttribute m_attribute;

  int m_renderOrder;      // for render order of dialogs
};

#endif
