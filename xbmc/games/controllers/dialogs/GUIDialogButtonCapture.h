/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/joysticks/interfaces/IButtonMapper.h"
#include "threads/Event.h"
#include "threads/Thread.h"
#include "utils/Observer.h"

#include <string>
#include <vector>

namespace KODI
{
namespace KEYMAP
{
class IKeymap;
} // namespace KEYMAP

namespace GAME
{
/*!
 * \ingroup games
 */
class CGUIDialogButtonCapture : public JOYSTICK::IButtonMapper, public Observer, protected CThread
{
public:
  CGUIDialogButtonCapture();

  ~CGUIDialogButtonCapture() override = default;

  // implementation of IButtonMapper
  std::string ControllerID() const override;
  bool NeedsCooldown() const override { return false; }
  bool MapPrimitive(JOYSTICK::IButtonMap* buttonMap,
                    KEYMAP::IKeymap* keymap,
                    const JOYSTICK::CDriverPrimitive& primitive) override;
  void OnEventFrame(const JOYSTICK::IButtonMap* buttonMap, bool bMotion) override {}
  void OnLateAxis(const JOYSTICK::IButtonMap* buttonMap, unsigned int axisIndex) override {}

  // implementation of Observer
  void Notify(const Observable& obs, const ObservableMessage msg) override;

  /*!
   * \brief Show the dialog
   */
  void Show();

protected:
  // implementation of CThread
  void Process() override;

  virtual std::string GetDialogText() = 0;
  virtual std::string GetDialogHeader() = 0;
  virtual bool MapPrimitiveInternal(JOYSTICK::IButtonMap* buttonMap,
                                    KEYMAP::IKeymap* keymap,
                                    const JOYSTICK::CDriverPrimitive& primitive) = 0;
  virtual void OnClose(bool bAccepted) = 0;

  CEvent m_captureEvent;

private:
  void InstallHooks();
  void RemoveHooks();
};
} // namespace GAME
} // namespace KODI
