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

#include "input/joysticks/interfaces/IButtonSequence.h"

#include <map>
#include <string>
#include <vector>

namespace KODI
{
namespace JOYSTICK
{
  /*!
   * \brief Hush!!!
   */
  class CJoystickEasterEgg : public IButtonSequence
  {
  public:
    explicit CJoystickEasterEgg(const std::string& controllerId);
    virtual ~CJoystickEasterEgg() = default;

    // implementation of IButtonSequence
    virtual bool OnButtonPress(const FeatureName& feature) override;

    static void OnFinish(void);

  private:
    // Construction parameters
    const std::string m_controllerId;

    static const std::map<std::string, std::vector<FeatureName>> m_sequence;

    unsigned int m_state;
  };
}
}
