/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "cores/GameSettings.h"
#include "threads/CriticalSection.h"

#include <map>
#include <memory>

namespace KODI
{
namespace GAME
{
class CDialogGameAdvancedSettings;
class CDialogGameVideoSelect;
} // namespace GAME

namespace RETRO
{
class CGameWindowFullScreen;
class CGUIGameControl;
class CGUIGameSettingsHandle;
class CGUIGameVideoHandle;
class CGUIRenderTargetFactory;
class CGUIRenderHandle;
class CGUIRenderTarget;
class IGameCallback;
class IRenderCallback;
class IPlayback;

/*!
 * \brief Class to safely route commands between the GUI and RetroPlayer
 *
 * This class is brought up before the GUI and player core factory. It
 * provides the GUI with safe access to a registered player.
 *
 * Access to the player is done through handles. When a handle is no
 * longer needed, it should be destroyed.
 *
 * Three kinds of handles are provided:
 *
 *   - CGUIRenderHandle
 *         Allows the holder to invoke render events
 *
 *   - CGUIGameVideoHandle
 *         Allows the holder to query video properties, such as the filter
 *         or view mode.
 *
 *   - CGUIGameSettingsHandle
 *         Allows the holder to query game properties, such as the ID of the
 *         game client or the game's filename.
 *
 * Each manager fulfills the following design requirements:
 *
 *   1. No assumption of player lifetimes
 *
 *   2. No assumption of GUI element lifetimes, as long as handles are
 *      destroyed before this class is destructed
 *
 *   3. No limit on the number of handles
 */
class CGUIGameRenderManager
{
  // Classes that call into the protected interface
  friend class CGUIGameSettingsHandle;
  friend class CGUIGameVideoHandle;
  friend class CGUIRenderHandle;

public:
  CGUIGameRenderManager() = default;
  ~CGUIGameRenderManager();

  /*!
   * \brief Register a RetroPlayer instance
   *
   * \param factory The interface for creating render targets exposed to the GUI
   * \param callback The interface for querying video properties
   * \param gameCallback The interface for querying game properties
   */
  void RegisterPlayer(CGUIRenderTargetFactory* factory,
                      IRenderCallback* callback,
                      IGameCallback* gameCallback);

  /*!
   * \brief Unregister a RetroPlayer instance
   */
  void UnregisterPlayer();

  /*!
   * \brief Register a GUI game control ("gamewindow" skin control)
   *
   * \param control The game control
   *
   * \return A handle to invoke render events
   */
  std::shared_ptr<CGUIRenderHandle> RegisterControl(CGUIGameControl& control);

  /*!
   * \brief Register a fullscreen game window ("FullscreenGame" window)
   *
   * \param window The game window
   *
   * \return A handle to invoke render events
   */
  std::shared_ptr<CGUIRenderHandle> RegisterWindow(CGameWindowFullScreen& window);

  /*!
   * \brief Register a video select dialog (for selecting video filters,
   *        view modes, etc.)
   *
   * \param dialog The video select dialog
   *
   * \return A handle to query game and video properties
   */
  std::shared_ptr<CGUIGameVideoHandle> RegisterDialog(GAME::CDialogGameVideoSelect& dialog);

  /*!
   * \brief Register a game settings dialog
   *
   * \return A handle to query game properties
   */
  std::shared_ptr<CGUIGameSettingsHandle> RegisterGameSettingsDialog();

protected:
  // Functions exposed to friend class CGUIRenderHandle
  void UnregisterHandle(CGUIRenderHandle* handle);
  void Render(CGUIRenderHandle* handle);
  void RenderEx(CGUIRenderHandle* handle);
  void ClearBackground(CGUIRenderHandle* handle);
  bool IsDirty(CGUIRenderHandle* handle);

  // Functions exposed to friend class CGUIGameVideoHandle
  void UnregisterHandle(CGUIGameVideoHandle* handle) {}
  bool IsPlayingGame();
  bool SupportsRenderFeature(RENDERFEATURE feature);
  bool SupportsScalingMethod(SCALINGMETHOD method);

  // Functions exposed to CGUIGameSettingsHandle
  void UnregisterHandle(CGUIGameSettingsHandle* handle) {}
  std::string GameClientID();
  std::string GetPlayingGame();
  std::string CreateSavestate(bool autosave);
  bool UpdateSavestate(const std::string& savestatePath);
  bool LoadSavestate(const std::string& savestatePath);
  void FreeSavestateResources(const std::string& savestatePath);
  void CloseOSD();

private:
  /*!
   * \brief Helper function to create or destroy render targets when a
   *        factory is registered/unregistered
   */
  void UpdateRenderTargets();

  /*!
   * \brief Helper function to create a render target
   *
   * \param handle The handle given to the registered GUI element
   *
   * \return A target to receive rendering commands
   */
  CGUIRenderTarget* CreateRenderTarget(CGUIRenderHandle* handle);

  // Render events
  CGUIRenderTargetFactory* m_factory = nullptr;
  std::map<CGUIRenderHandle*, std::shared_ptr<CGUIRenderTarget>> m_renderTargets;
  CCriticalSection m_targetMutex;

  // Video properties
  IRenderCallback* m_callback = nullptr;
  CCriticalSection m_callbackMutex;

  // Game properties
  IGameCallback* m_gameCallback = nullptr;
  CCriticalSection m_gameCallbackMutex;
};
} // namespace RETRO
} // namespace KODI
