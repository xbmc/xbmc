/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "threads/Thread.h"

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CControllerInstaller : public CThread
{
public:
  CControllerInstaller();
  ~CControllerInstaller() override = default;

protected:
  // implementation of CThread
  void Process() override;
};
} // namespace GAME
} // namespace KODI
