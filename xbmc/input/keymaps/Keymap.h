/*
 *  Copyright (C) 2017-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "input/keymaps/interfaces/IKeymap.h"

#include <memory>

namespace KODI
{
namespace KEYMAP
{
class IKeymapEnvironment;
class IWindowKeymap;

/*!
 * \ingroup keymap
 */
class CKeymap : public IKeymap
{
public:
  CKeymap(std::shared_ptr<const IWindowKeymap> keymap, const IKeymapEnvironment* environment);

  // implementation of IKeymap
  std::string ControllerID() const override;
  const IKeymapEnvironment* Environment() const override { return m_environment; }
  const KeymapActionGroup& GetActions(const std::string& keyName) const override;

private:
  // Construction parameters
  const std::shared_ptr<const IWindowKeymap> m_keymap;
  const IKeymapEnvironment* const m_environment;
};
} // namespace KEYMAP
} // namespace KODI
