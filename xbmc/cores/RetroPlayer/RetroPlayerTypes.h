/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>
#include <vector>

namespace KODI
{
namespace RETRO
{
  class IRenderBufferPool;
  using RenderBufferPoolPtr = std::shared_ptr<IRenderBufferPool>;
  using RenderBufferPoolVector = std::vector<RenderBufferPoolPtr>;
}
}
