/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../ProcessInfo.h"

namespace VIDEOPLAYER
{

class CProcessInfoX11 : public CProcessInfo
{
public:
  static CProcessInfo* Create();
  static void Register();

  void SetSwDeinterlacingMethods() override;
  std::vector<AVPixelFormat> GetRenderFormats() override;
};

}
