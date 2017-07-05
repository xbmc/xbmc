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

#include "utils/Observer.h"

#include <memory>
#include <string>
#include <vector>

class IKeymap;
class IKeymapEnvironment;

namespace KODI
{
namespace JOYSTICK
{
  class IInputHandler;
  class IInputProvider;
  class IInputReceiver;

  /*!
   * \ingroup joystick
   * \brief
   */
  class CKeymapHandling : public Observer
  {
  public:
    CKeymapHandling(IInputProvider *inputProvider, bool pPromiscuous, const IKeymapEnvironment *environment);

    virtual ~CKeymapHandling();

    /*!
     * \brief
     */
    IInputReceiver *GetInputReceiver(const std::string &controllerId) const;
    
    /*!
     * \brief
     */
    IKeymap *GetKeymap(const std::string &controllerId) const;

    // implementation of Observer
    virtual void Notify(const Observable &obs, const ObservableMessage msg) override;

  private:
    void LoadKeymaps();
    void UnloadKeymaps();

    // Construction parameter
    IInputProvider *const m_inputProvider;
    const bool m_pPromiscuous;
    const IKeymapEnvironment *const m_environment;

    std::vector<std::unique_ptr<IKeymap>> m_keymaps;
    std::vector<std::unique_ptr<IInputHandler>> m_inputHandlers;
  };
}
}
