/*
 *  Copyright (C) 2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "threads/Thread.h"

#include <functional>

namespace KODI
{
namespace GAME
{

/*!
 * \ingroup games
 */
class CControllerSelect : public CThread
{
public:
  CControllerSelect();
  ~CControllerSelect() override;

  void Initialize(ControllerVector controllers,
                  ControllerPtr defaultController,
                  bool showDisconnect,
                  const std::function<void(ControllerPtr)>& callback);
  void Deinitialize();

protected:
  // Implementation of CThread
  void Process() override;

private:
  // State parameters
  ControllerVector m_controllers;
  ControllerPtr m_defaultController;
  bool m_showDisconnect = true;
  std::function<void(ControllerPtr)> m_callback;
};
} // namespace GAME
} // namespace KODI
