/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "guilib/iimage.h"

class CWinSystemBase;

namespace KODI
{
namespace RENDERING
{
namespace CAPTURE
{

//! \brief H.273 description of what the display output shows right now.
ImageColorMetadata GetOutputColorMetadata(CWinSystemBase& winSystem);

} // namespace CAPTURE
} // namespace RENDERING
} // namespace KODI
