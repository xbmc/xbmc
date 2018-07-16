/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "RenderVideoSettings.h"

namespace KODI
{
namespace RETRO
{
  class CRenderSettings
  {
  public:
    CRenderSettings() { Reset(); }

    void Reset();

    bool operator==(const CRenderSettings &rhs) const;
    bool operator<(const CRenderSettings &rhs) const;

    CRenderVideoSettings &VideoSettings() { return m_videoSettings; }
    const CRenderVideoSettings &VideoSettings() const { return m_videoSettings; }

  private:
    CRenderVideoSettings m_videoSettings;
  };
}
}
