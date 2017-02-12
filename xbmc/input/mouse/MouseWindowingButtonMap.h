/*
 *      Copyright (C) 2016 Team Kodi
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

#include "input/mouse/IMouseButtonMap.h"

#include <string>
#include <utility>
#include <vector>

namespace KODI
{
namespace MOUSE
{
  /*!
   * \ingroup mouse
   * \brief Maps mouse windowing events to higher-level features understood by IMouseInputHandler implementations.
   */
  class CMouseWindowingButtonMap : public IMouseButtonMap
  {
  public:
    virtual ~CMouseWindowingButtonMap(void) = default;

    // implementation of IMouseButtonMap
    virtual std::string ControllerID(void) const override;
    virtual bool GetButton(unsigned int buttonIndex, std::string& feature) override;
    virtual bool GetRelativePointer(std::string& feature) override;
    virtual bool GetButtonIndex(const std::string& feature, unsigned int& buttonIndex) override;

  private:
    static std::vector<std::pair<unsigned int, std::string>> m_buttonMap;
    static std::string m_pointerName;
  };
}
}
