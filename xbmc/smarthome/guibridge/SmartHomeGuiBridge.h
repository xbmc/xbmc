/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include <memory>
#include <mutex>

namespace KODI
{
namespace SMART_HOME
{
class CGUICameraControl;
class CGUIRenderHandle;
class CGUIRenderTarget;
class CGUIRenderTargetFactory;

/*!
 * \brief Class to safely route commands between the GUI and the renderer
 *
 * This class is brought up before the GUI and the renderer factory. It provides
 * the GUI with safe access to a registered renderer.
 *
 * Access to the renderer is done through handles. When a handle is no longer
 * needed, it should be destroyed.
 *
 * One kind of handle is provided:
 *
 *   - CGUIRenderHandle
 *         Allows the holder to invoke render events
 *
 * Each GUI bridge fulfills the following design requirements:
 *
 *   1. No assumption of renderer lifetimes
 *
 *   2. No assumption of GUI element lifetimes, as long as handles are
 *      destroyed before this class is destructed
 *
 *   3. No limit on the number of handles
 */
class CSmartHomeGuiBridge
{
  friend class CGUIRenderHandle;

public:
  CSmartHomeGuiBridge() = default;
  ~CSmartHomeGuiBridge();

  /*!
   * \brief Register a renderer instance
   *
   * \param factory The interface for creating render targets exposed to the GUI
   */
  void RegisterRenderer(CGUIRenderTargetFactory& factory);

  /*!
   * \brief Unregister a renderer instance
   */
  void UnregisterRenderer();

  /*!
   * \brief Register a GUI camera view control ("cameraview" skin control)
   *
   * \param control The camera view control
   *
   * \return A handle to invoke render events
   */
  std::shared_ptr<CGUIRenderHandle> RegisterControl(CGUICameraControl& control);

protected:
  // Functions exposed to friend class CGUIRenderHandle
  void UnregisterHandle(CGUIRenderHandle* handle);
  void Render(CGUIRenderHandle* handle);
  void ClearBackground(CGUIRenderHandle* handle);
  bool IsDirty(CGUIRenderHandle* handle);

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
  std::mutex m_targetMutex;
};

} // namespace SMART_HOME
} // namespace KODI
