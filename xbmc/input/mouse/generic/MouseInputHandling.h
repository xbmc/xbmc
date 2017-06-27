/*
 *      Copyright (C) 2016-2017 Team Kodi
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

#include "input/mouse/IMouseDriverHandler.h"

namespace KODI
{
namespace MOUSE
{
  class IMouseInputHandler;
  class IMouseButtonMap;
  class CRelativePointer;

  /*!
   * \ingroup mouse
   * \brief Class to translate input from driver info to higher-level features
   */
  class CMouseInputHandling : public IMouseDriverHandler
  {
  public:
    CMouseInputHandling(IMouseInputHandler* handler, IMouseButtonMap* buttonMap);

    ~CMouseInputHandling(void) override = default;

    // implementation of IMouseDriverHandler
    bool OnPosition(int x, int y) override;
    bool OnButtonPress(unsigned int button) override;
    void OnButtonRelease(unsigned int button) override;

  private:
    // Construction parameters
    IMouseInputHandler* const m_handler;
    IMouseButtonMap* const    m_buttonMap;

    // Mouse parameters
    int m_x;
    int m_y;
  };
}
}
