/*
 *      Copyright (C) 2016-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "input/keyboard/interfaces/IKeyboardDriverHandler.h"
#include "input/XBMC_vkeys.h"

#include <vector>

namespace KODI
{
namespace KEYBOARD
{
  /*!
   * \brief Hush!!!
   */
  class CKeyboardEasterEgg : public IKeyboardDriverHandler
  {
  public:
    CKeyboardEasterEgg(void);
    ~CKeyboardEasterEgg() override = default;

    // implementation of IKeyboardDriverHandler
    bool OnKeyPress(const CKey& key) override;
    void OnKeyRelease(const CKey& key) override { }

  private:
    static std::vector<XBMCVKey> m_sequence;

    unsigned int m_state;
  };
}
}
