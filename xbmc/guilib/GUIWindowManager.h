/*!
\file GUIWindowManager.h
\brief
*/

#ifndef GUILIB_CGUIWindowManager_H
#define GUILIB_CGUIWindowManager_H

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

#include <list>
#include <utility>
#include <typeindex>
#include <unordered_map>
#include <vector>

#include "DirtyRegionTracker.h"
#include "guilib/WindowIDs.h"
#include "GUIWindow.h"
#include "IMsgTargetCallback.h"
#include "IWindowManagerCallback.h"
#include "messaging/IMessageTarget.h"
#include "utils/GlobalsHandling.h"

class CGUIDialog;
class CGUIMediaWindow;

enum class DialogModalityType;

namespace KODI
{
  namespace MESSAGING
  {
    class CApplicationMessenger;
  }
}

#define WINDOW_ID_MASK 0xffff

/*!
 \ingroup winman
 \brief
 */
class CGUIWindowManager : public KODI::MESSAGING::IMessageTarget
{
  friend CGUIDialog;
  friend CGUIMediaWindow;
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
  void ActivateWindow(int iWindowID, const std::string &strPath = "");
  void ForceActivateWindow(int iWindowID, const std::string &strPath = "");
  void ChangeActiveWindow(int iNewID, const std::string &strPath = "");
  void ActivateWindow(int iWindowID, const std::vector<std::string>& params, bool swappingWindows = false, bool force = false);
  void PreviousWindow();

  void CloseDialogs(bool forceClose = false) const;
  void CloseInternalModalDialogs(bool forceClose = false) const;

  virtual void OnApplicationMessage(KODI::MESSAGING::ThreadMessage* pMsg) override;
  virtual int GetMessageMask() override;

  // OnAction() runs through our active dialogs and windows and sends the message
  // off to the callbacks (application, python, playlist player) and to the
  // currently focused window(s).  Returns true only if the message is handled.
  bool OnAction(const CAction &action) const;

  /*! \brief Process active controls allowing them to animate before rendering.
   */
  void Process(unsigned int currentTime);

  /*! \brief Mark the screen as dirty, forcing a redraw at the next Render()
   */
  void MarkDirty();

  /*! \brief Mark a region as dirty, forcing a redraw at the next Render()
   */
  void MarkDirty(const CRect& rect);

  /*! \brief Get the current dirty region
   */
  CDirtyRegionList GetDirty() { return m_tracker.GetDirtyRegions(); }

  /*! \brief Rendering of the current window and any dialogs
   Render is called every frame to draw the current window and any dialogs.
   It should only be called from the application thread.
   Returns true only if it has rendered something.
   */
  bool Render();

  void RenderEx() const;

  /*! \brief Do any post render activities.
   */
  void AfterRender();

  /*! \brief Per-frame updating of the current window and any dialogs
   FrameMove is called every frame to update the current window and any dialogs
   on screen. It should only be called from the application thread.
   */
  void FrameMove();

  /*! \brief Return whether the window manager is initialized.
   The window manager is initialized on skin load - if the skin isn't yet loaded,
   no windows should be able to be initialized.
   \return true if the window manager is initialized, false otherwise.
   */
  bool Initialized() const { return m_initialized; };

  /*! \brief Create and initialize all windows and dialogs
   */
  void CreateWindows();

  /*! \brief Destroy and remove all windows and dialogs
  *
  * \return true on success, false if destruction fails for any window
  */
  bool DestroyWindows();

  /*! \brief Return the window for the given type \code{T}.
   *
   * \tparam T the window class type
   * \return the window with for the given type \code{T} or null
   */
  template<typename T, typename std::enable_if<std::is_base_of<CGUIWindow,T>::value>::type* = nullptr>
  T* GetWindow() const { return dynamic_cast<T *>(GetWindow(std::type_index(typeid(T)))); };

  /*! \brief Return the window with the given id or null.
   *
   * \param id the window id
   * \return the window with the given id or null
   */
  CGUIWindow* GetWindow(int id) const;

  /*! \brief Return the dialog window with the given id or null.
   *
   * \param id the dialog window id
   * \return the dialog window with the given id or null
   */
  CGUIDialog* GetDialog(int id) const;

  void SetCallback(IWindowManagerCallback& callback);
  void DeInitialize();

  /*! \brief Register a dialog as active dialog
   *
   * \param dialog The dialog to register as active dialog
   */
  void RegisterDialog(CGUIWindow* dialog);
  void RemoveDialog(int id);
  int GetTopMostModalDialogID(bool ignoreClosing = false) const;

  void SendThreadMessage(CGUIMessage& message, int window = 0);
  void DispatchThreadMessages();
  // method to removed queued messages with message id in the requested message id list.
  // pMessageIDList: point to first integer of a 0 ends integer array.
  int RemoveThreadMessageByMessageIds(int *pMessageIDList);
  void AddMsgTarget( IMsgTargetCallback* pMsgTarget );
  int GetActiveWindow() const;
  int GetActiveWindowID();
  int GetFocusedWindow() const;
  bool HasModalDialog(const std::vector<DialogModalityType>& types = std::vector<DialogModalityType>(), bool ignoreClosing = true) const;
  bool HasVisibleModalDialog(const std::vector<DialogModalityType>& types = std::vector<DialogModalityType>()) const;
  bool HasDialogOnScreen() const;
  bool IsWindowActive(int id, bool ignoreClosing = true) const;
  bool IsWindowVisible(int id) const;
  bool IsWindowTopMost(int id) const;
  bool IsWindowActive(const std::string &xmlFile, bool ignoreClosing = true) const;
  bool IsWindowVisible(const std::string &xmlFile) const;
  bool IsWindowTopMost(const std::string &xmlFile) const;
  /*! \brief Checks if the given window is an addon window.
   *
   * \return true if the given window is an addon window, otherwise false.
   */
  bool IsAddonWindow(int id) const { return (id >= WINDOW_ADDON_START && id <= WINDOW_ADDON_END); };
  /*! \brief Checks if the given window is a python window.
   *
   * \return true if the given window is a python window, otherwise false.
   */
  bool IsPythonWindow(int id) const { return (id >= WINDOW_PYTHON_START && id <= WINDOW_PYTHON_END); };
  void GetActiveModelessWindows(std::vector<int> &ids);
#ifdef _DEBUG
  void DumpTextureUse();
#endif
private:
  void RenderPass() const;

  void LoadNotOnDemandWindows();
  void UnloadNotOnDemandWindows();
  void AddToWindowHistory(int newWindowID);
  void ClearWindowHistory();
  void CloseWindowSync(CGUIWindow *window, int nextWindowID = 0);
  CGUIWindow *GetTopMostDialog() const;

  CGUIWindow* GetWindow(std::type_index type) const;

  friend class KODI::MESSAGING::CApplicationMessenger;
  
  /*! \brief Activate the given window.
   *
   * \param windowID The window ID to activate.
   * \param params Parameter
   * \param swappingWindows True if the window should be swapped with the previous window instead of put it in the window history, otherwise false
   * \param force True to ignore checks which refuses opening the window, otherwise false
   */
  void ActivateWindow_Internal(int windowID, const std::vector<std::string> &params, bool swappingWindows, bool force = false);

  void ProcessRenderLoop(bool renderOnly = false);

  std::unordered_map<int, CGUIWindow*> m_mapWindows;
  std::unordered_map<std::type_index, CGUIWindow*> m_mapWindowTypes;
  std::vector<CGUIWindow*> m_vecCustomWindows;
  std::vector<CGUIWindow*> m_activeDialogs;
  std::vector<CGUIWindow*> m_deleteWindows;

  std::stack<int> m_windowHistory;

  IWindowManagerCallback* m_pCallback;
  std::list< std::pair<CGUIMessage*,int> > m_vecThreadMessages;
  CCriticalSection m_critSection;
  std::vector<IMsgTargetCallback*> m_vecMsgTargets;

  int  m_iNested;
  bool m_initialized;

  CDirtyRegionTracker m_tracker;
};

/*!
 \ingroup winman
 \brief
 */
XBMC_GLOBAL_REF(CGUIWindowManager,g_windowManager);
#define g_windowManager XBMC_GLOBAL_USE(CGUIWindowManager)
#endif

