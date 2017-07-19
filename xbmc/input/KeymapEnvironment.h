/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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

private:
  int m_windowId = -1;
};
