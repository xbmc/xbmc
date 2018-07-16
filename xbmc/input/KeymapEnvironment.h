/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IKeymapEnvironment.h"

class CKeymapEnvironment : public IKeymapEnvironment
{
public:
  virtual ~CKeymapEnvironment() = default;

  // implementation of IKeymapEnvironment
  virtual int GetWindowID() const override { return m_windowId; }
  virtual void SetWindowID(int windowId) override { m_windowId = windowId; }
  virtual int GetFallthrough(int windowId) const override;
  virtual bool UseGlobalFallthrough() const override { return true; }
  virtual bool UseEasterEgg() const override { return true; }

private:
  int m_windowId = -1;
};
