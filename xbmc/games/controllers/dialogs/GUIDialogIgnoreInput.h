/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GUIDialogButtonCapture.h"
#include "input/joysticks/DriverPrimitive.h"

#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
class CGUIDialogIgnoreInput : public CGUIDialogButtonCapture
{
public:
  CGUIDialogIgnoreInput() = default;

  ~CGUIDialogIgnoreInput() override = default;

  // specialization of IButtonMapper via CGUIDialogButtonCapture
  bool AcceptsPrimitive(JOYSTICK::PRIMITIVE_TYPE type) const override;

protected:
  // implementation of CGUIDialogButtonCapture
  std::string GetDialogText() override;
  std::string GetDialogHeader() override;
  bool MapPrimitiveInternal(JOYSTICK::IButtonMap* buttonMap,
                            KEYMAP::IKeymap* keymap,
                            const JOYSTICK::CDriverPrimitive& primitive) override;
  void OnClose(bool bAccepted) override;

private:
  bool AddPrimitive(const JOYSTICK::CDriverPrimitive& primitive);

  std::string m_location;
  std::vector<JOYSTICK::CDriverPrimitive> m_capturedPrimitives;
};
} // namespace GAME
} // namespace KODI
