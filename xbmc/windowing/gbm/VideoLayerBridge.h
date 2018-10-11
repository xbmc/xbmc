/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

class CVideoLayerBridge
{
public:
  virtual ~CVideoLayerBridge() = default;
  virtual void Disable() {};
};

}
}
}
