/*!
\file GUIWindow.h
\brief 
*/

#ifndef GUILIB_GUIWINDOW_H
#define GUILIB_GUIWINDOW_H

#pragma once

#include "GUIControl.h"

class CGUIControlGroup;
class CFileItem;

#include "GUICallback.h"  // for GUIEvent

#include <map>
#include <vector>

#define ON_CLICK_MESSAGE(i,c,m) \
{ \
 GUIEventHandler<c, CGUIMessage&> clickHandler(this, &c::m); \
 m_mapClickEvents[i] = clickHandler; \
} \

#define ON_SELECTED_MESSAGE(i,c,m) \
{ \
 GUIEventHandler<c, CGUIMessage&> selectedHandler(this, &c::m); \
 m_mapSelectedEvents[i] = selectedHandler; \
} \

// forward
class TiXmlNode;
class TiXmlElement;

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
class CGUIWindow
{
public:
  enum WINDOW_TYPE { WINDOW = 0, MODAL_DIALOG, MODELESS_DIALOG, BUTTON_MENU, SUB_MENU };

  CGUIWindow(DWORD dwID, const CStdString &xmlFile);
  virtual ~CGUIWindow(void);

  bool Initialize();  // loads the window
  virtual bool Load(const CStdString& strFileName, bool bContainsPath = false);
  virtual bool Load(TiXmlElement* pRootElement);
  virtual void SetPosition(float posX, float posY);
  void CenterWindow();
  virtual void Render();

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
  void Add(CGUIControl* pControl);
  void Insert(CGUIControl *control, const CGUIControl *insertPoint);
  bool Remove(DWORD dwId);
  bool ControlGroupHasFocus(int groupID, int controlID);
  void SetID(DWORD dwID);
  virtual DWORD GetID(void) const;
  virtual bool HasID(DWORD dwID) { return (dwID >= m_dwWindowId && dwID < m_dwWindowId + m_dwIDRange); };
  void SetIDRange(DWORD dwRange) { m_dwIDRange = dwRange; };
  DWORD GetIDRange() const { return m_dwIDRange; };
  virtual float GetWidth() { return m_width; };
  virtual float GetHeight() { return m_height; };
  DWORD GetPreviousWindow() { return m_previousWindow; };
  float GetPosX() { return m_posX; };
  float GetPosY() { return m_posY; };
  FRECT GetScaledBounds() const;
  const CGUIControl* GetControl(int iControl) const;
  void ClearAll();
  int GetFocusedControlID() const;
  CGUIControl *GetFocusedControl() const;
  virtual void AllocResources(bool forceLoad = false);
  virtual void FreeResources(bool forceUnLoad = false);
  void DynamicResourceAlloc(bool bOnOff);
//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  static void FlushReferenceCache();
//#endif
  virtual bool IsDialog() const { return false; };
  virtual bool IsDialogRunning() const { return false; };
  virtual bool IsModalDialog() const { return false; };
  virtual bool IsMediaWindow() const { return false; };
  virtual bool HasListItems() const { return false; };
  virtual CFileItem *GetCurrentListItem(int offset = 0) { return NULL; };
  virtual int GetViewContainerID() const { return 0; };
  void GetContainers(vector<CGUIControl *> &containers) const;
  virtual bool IsActive() const;
  bool IsAllocated() const { return m_WindowAllocated; };
  void SetCoordsRes(RESOLUTION res) { m_coordsRes = res; };
  RESOLUTION GetCoordsRes() const { return m_coordsRes; };
  int GetVisibleCondition() const { return m_visibleCondition; };
  void SetXMLFile(const CStdString &xmlFile) { m_xmlFile = xmlFile; };
  const CStdString &GetXMLFile() const { return m_xmlFile; };
  void LoadOnDemand(bool loadOnDemand) { m_loadOnDemand = loadOnDemand; };
  bool GetLoadOnDemand() { return m_loadOnDemand; }
  int GetRenderOrder() { return m_renderOrder; };
  void SetControlVisibility();

  enum OVERLAY_STATE { OVERLAY_STATE_PARENT_WINDOW=0, OVERLAY_STATE_SHOWN, OVERLAY_STATE_HIDDEN };

  OVERLAY_STATE GetOverlayState() const { return m_overlayState; };

  virtual void QueueAnimation(ANIMATION_TYPE animType);
  virtual bool IsAnimating(ANIMATION_TYPE animType);

  virtual void ResetControlStates();

#ifdef _DEBUG
  void DumpTextureUse();
#endif

protected:
  virtual void SetDefaults();
  virtual void OnWindowUnload() {}
  virtual void OnWindowLoaded();
  virtual void OnInitWindow();
  virtual void OnDeinitWindow(int nextWindowID);
  virtual void OnMouseAction();
  virtual bool RenderAnimation(DWORD time);
  virtual void UpdateStates(ANIMATION_TYPE type, ANIMATION_PROCESS currentProcess, ANIMATION_STATE currentState);
  bool HasAnimation(ANIMATION_TYPE animType);

  // control state saving on window close
  virtual void SaveControlStates();
  virtual void RestoreControlStates();
  void AddControlGroup(int id);
  virtual CGUIControl *GetFirstFocusableControl(int id);

  typedef GUIEvent<CGUIMessage&> CLICK_EVENT;
  typedef std::map<int, CLICK_EVENT> MAPCONTROLCLICKEVENTS;
  MAPCONTROLCLICKEVENTS m_mapClickEvents;

  typedef GUIEvent<CGUIMessage&> SELECTED_EVENT;
  typedef std::map<int, SELECTED_EVENT> MAPCONTROLSELECTEDEVENTS;
  MAPCONTROLSELECTEDEVENTS m_mapSelectedEvents;

  void LoadControl(TiXmlElement* pControl, CGUIControlGroup *pGroup);

//#ifdef PRE_SKIN_VERSION_2_1_COMPATIBILITY
  bool LoadReferences();
  static CStdString CacheFilename;
//#endif

  vector<CGUIControl*> m_vecControls;
  typedef std::vector<CGUIControl*>::iterator ivecControls;
  typedef std::vector<CGUIControl*>::const_iterator ciControls;
  DWORD m_dwWindowId;
  DWORD m_dwIDRange;
  DWORD m_dwDefaultFocusControlID;
  bool m_bRelativeCoords;
  float m_posX;
  float m_posY;
  float m_width;
  float m_height;
  OVERLAY_STATE m_overlayState;
  bool m_WindowAllocated;
  RESOLUTION m_coordsRes; // resolution that the window coordinates are in.
  bool m_needsScaling;
  CStdString m_xmlFile;  // xml file to load
  bool m_windowLoaded;  // true if the window's xml file has been loaded
  bool m_loadOnDemand;  // true if the window should be loaded only as needed
  bool m_isDialog;      // true if we have a dialog, false otherwise.
  bool m_dynamicResourceAlloc;
  int m_visibleCondition;

  bool   m_hasCamera;
  CPoint m_camera;      // 3D camera position (x,y coords - z is fixed currently)
  CAnimation m_showAnimation;
  CAnimation m_closeAnimation;

  int m_renderOrder;      // for render order of dialogs
  bool m_hasRendered;

  vector<COrigin> m_origins;  // positions of dialogs depending on base window

  // control states
  bool m_saveLastControl;
  int m_lastControlID;
  int m_focusedControl;
  vector<CControlState> m_controlStates;
  DWORD m_previousWindow;
};

#endif
