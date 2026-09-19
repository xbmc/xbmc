/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "windowing/OSScreenSaver.h"

namespace KODI::WINDOWING::WASM
{

/*!
 * \brief Keeps the display awake through the browser's Screen Wake Lock API.
 */
class COSScreenSaverWasm : public IOSScreenSaver
{
public:
  void Inhibit() override;
  void Uninhibit() override;
};

} // namespace KODI::WINDOWING::WASM
