/*
 *  Copyright (C) 2005-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"

namespace kodi
{
namespace gui
{
struct IRenderHelper
{
  virtual ~IRenderHelper() = default;
  virtual bool Init() = 0;
  virtual void Begin() = 0;
  virtual void End() = 0;
}; /* class IRenderHelper */
} /* namespace gui */
} /* namespace kodi */

#if defined(WIN32) && defined(HAS_ANGLE)
#include "gl/GLonDX.h"
#else
/*
 * Default background GUI render helper class
 */
namespace kodi
{
namespace gui
{
struct CRenderHelperStub : public IRenderHelper
{
  bool Init() override { return true; }
  void Begin() override { }
  void End() override { }
}; /* class CRenderHelperStub */

using CRenderHelper = CRenderHelperStub;
} /* namespace gui */
} /* namespace kodi */
#endif

namespace kodi
{
namespace gui
{

/*
 * Create render background handler, e.g. becomes on "Windows" Angle used
 * to emulate GL.
 *
 * This only be used internal and not from addon's direct.
 *
 * Function defines here and not in CAddonBase because of a hen and egg problem.
 */
inline std::shared_ptr<IRenderHelper> GetRenderHelper()
{
  using namespace ::kodi::addon;
  if (CAddonBase::m_interface->addonBase->m_renderHelper)
    return CAddonBase::m_interface->addonBase->m_renderHelper;

  const std::shared_ptr<kodi::gui::IRenderHelper> renderHelper(new CRenderHelper());
  if (!renderHelper->Init())
    return nullptr;

  CAddonBase::m_interface->addonBase->m_renderHelper = renderHelper; // Hold on base for other types
  return renderHelper;
}

} /* namespace gui */
} /* namespace kodi */
