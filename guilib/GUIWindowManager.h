/*!
\file GUIWindowManager.h
\brief
*/

#ifndef GUILIB_CGUIWindowManager_H
#define GUILIB_CGUIWindowManager_H

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

#include "GUIWindow.h"
#include "IWindowManagerCallback.h"
#include "IMsgTargetCallback.h"

class CGUIDialog;

#define WINDOW_ID_MASK 0xffff

/*!
 \ingroup winman
 \brief
 */
class CGUIWindowManager
{
public:
  CGUIWindowManager(void);
  virtual ~CGUIWindowManager(void);
  bool SendMessage(CGUIMessage& message);
  bool SendMessage(int message, int senderID, int destID, int param1 = 0, int param2 = 0);
  bool SendMessage(CGUIMessage& message, int window);
  void Initialize();
  void Add(CGUIWindow* pWindow);
  void AddUniqueInstance(CGUIWindow *window);
  void AddCustomWindow(CGUIWindow* pWindow);
  void Remove(int id);
  void Delete(int id);
  void ActivateWindow(int iWindowID, const CStdString &strPath = "");
  void ChangeActiveWindow(int iNewID, const CStdString &strPath = "");
  void ActivateWindow(int iWindowID, const std::vector<CStdString>& params, bool swappingWindows = false);
  void PreviousWindow();

  void CloseDialogs(bool forceClose = false);

  // OnAction() runs through our active dialogs and windows and sends the message
  // off to the callbacks (application, python, playlist player) and to the
  // currently focused window(s).  Returns true only if the message is handled.
  bool OnAction(const CAction &action);

  /*! \brief Rendering of the current window and any dialogs
   Render is called every frame to draw the current window and any dialogs.
   It should only be called from the application thread.
   */
  void Render();

  /*! \brief Per-frame updating of the current window and any dialogs
   FrameMove is called every frame to update the current window and any dialogs
   on screen. It should only be called from the application thread.
   */
  void FrameMove();

  /*! \brief Method used to mark selected region as dirty so next pass of rendering
   contains said region.
   */
  void MarkDirtyRegion(CRect region);

  /*! \brief Process active controls allowing them to animate before rendering.
   */
  // TODO Old Process method needs to be removed which did both processing and rendering before naming this Process
  void PureProcess(unsigned int currentTime);

  CGUIWindow* GetWindow(int id) const;
  void Process(bool renderOnly = false);
  void SetCallback(IWindowManagerCallback& callback);
  void DeInitialize();

  void RouteToWindow(CGUIWindow* dialog);
  void AddModeless(CGUIWindow* dialog);
  void RemoveDialog(int id);
  int GetTopMostModalDialogID() const;

  void SendThreadMessage(CGUIMessage& message);
  void SendThreadMessage(CGUIMessage& message, int window);
  void DispatchThreadMessages();
  void AddMsgTarget( IMsgTargetCallback* pMsgTarget );
  int GetActiveWindow() const;
  int GetFocusedWindow() const;
  bool HasModalDialog() const;
  bool HasDialogOnScreen() const;
  void UpdateModelessVisibility();
  bool IsWindowActive(int id, bool ignoreClosing = true) const;
  bool IsWindowVisible(int id) const;
  bool IsWindowTopMost(int id) const;
  bool IsWindowActive(const CStdString &xmlFile, bool ignoreClosing = true) const;
  bool IsWindowVisible(const CStdString &xmlFile) const;
  bool IsWindowTopMost(const CStdString &xmlFile) const;
  bool IsOverlayAllowed() const;
  void ShowOverlay(CGUIWindow::OVERLAY_STATE state);
  void GetActiveModelessWindows(std::vector<int> &ids);
#ifdef _DEBUG
  void DumpTextureUse();
#endif
private:
  void LoadNotOnDemandWindows();
  void UnloadNotOnDemandWindows();
  void HideOverlay(CGUIWindow::OVERLAY_STATE state);
  void AddToWindowHistory(int newWindowID);
  void ClearWindowHistory();
  CGUIWindow *GetTopMostDialog() const;

  friend class CApplicationMessenger;
  void ActivateWindow_Internal(int windowID, const std::vector<CStdString> &params, bool swappingWindows);

  typedef std::map<int, CGUIWindow *> WindowMap;
  WindowMap m_mapWindows;
  std::vector <CGUIWindow*> m_vecCustomWindows;
  std::vector <CGUIWindow*> m_activeDialogs;
  std::vector <CGUIWindow*> m_deleteWindows;
  typedef std::vector<CGUIWindow*>::iterator iDialog;
  typedef std::vector<CGUIWindow*>::const_iterator ciDialog;
  typedef std::vector<CGUIWindow*>::reverse_iterator rDialog;
  typedef std::vector<CGUIWindow*>::const_reverse_iterator crDialog;

  std::stack<int> m_windowHistory;

  IWindowManagerCallback* m_pCallback;
  std::vector < std::pair<CGUIMessage*,int> > m_vecThreadMessages;
  CCriticalSection m_critSection;
  std::vector <IMsgTargetCallback*> m_vecMsgTargets;

  bool m_bShowOverlay;

  std::vector<CRect> m_DirtyRegion;
  int  m_iNested;
};

/*!
 \ingroup winman
 \brief
 */
extern CGUIWindowManager g_windowManager;
#endif

