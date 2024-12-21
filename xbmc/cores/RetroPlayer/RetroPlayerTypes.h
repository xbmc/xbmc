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

enum class DataAccess
{
  READ_ONLY,
  WRITE_ONLY,
  READ_WRITE
};

enum class DataAlignment
{
  DATA_UNALIGNED,
  DATA_ALIGNED,
};

/*!
 * \brief A function pointer representing a hardware procedure
 *
 * This type alias is used to dynamically load and invoke hardware-specific
 * procedures, such as OpenGL or OpenGL ES functions, at runtime. The function
 * pointer can be retrieved using the `GetHwProcedureAddress` method in
 * \ref CRPProcessInfo.
 *
 * \note The function must be cast to the appropriate signature before use
 */
using HwProcedureAddress = void (*)();
} // namespace RETRO
} // namespace KODI
