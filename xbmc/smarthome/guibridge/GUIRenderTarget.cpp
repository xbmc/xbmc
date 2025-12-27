/*
 *  Copyright (C) 2021-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIRenderTarget.h"

#include "cores/RetroPlayer/rendering/IRenderManager.h"
#include "smarthome/guibridge/GUIRenderSettings.h"
#include "smarthome/guicontrols/GUICameraControl.h"
#include "utils/Geometry.h" //! @todo For IDE

using namespace KODI;
using namespace SMART_HOME;

namespace KODI
{
namespace RETRO
{
class IGUIRenderSettings;
}
} // namespace KODI

// --- CGUIRenderTarget --------------------------------------------------------

CGUIRenderTarget::CGUIRenderTarget(RETRO::IRenderManager& renderManager)
  : m_renderManager(renderManager)
{
}

// --- CGUIRenderControl -------------------------------------------------------

CGUIRenderControl::CGUIRenderControl(RETRO::IRenderManager& renderManager,
                                     CGUICameraControl& cameraControl)
  : CGUIRenderTarget(renderManager), m_cameraControl(cameraControl)
{
}

void CGUIRenderControl::Render()
{
  const CRect renderRegion = m_cameraControl.GetRenderRegion();
  const RETRO::IGUIRenderSettings& renderSettings = m_cameraControl.GetRenderSettings();

  m_renderManager.RenderControl(true, true, renderRegion, &renderSettings);
}
