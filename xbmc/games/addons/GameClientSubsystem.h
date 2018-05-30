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

#include <memory>

struct AddonInstance_Game;
class CCriticalSection;

namespace KODI
{
namespace GAME
{
  class CGameClient;
  class CGameClientInput;
  class CGameClientProperties;
  class CGameClientStreams;

  struct GameClientSubsystems
  {
    std::unique_ptr<CGameClientInput> Input;
    std::unique_ptr<CGameClientProperties> AddonProperties;
    std::unique_ptr<CGameClientStreams> Streams;
  };

  /*!
   * \brief Base class for game client subsystems
   */
  class CGameClientSubsystem
  {
  protected:
    CGameClientSubsystem(CGameClient &gameClient,
                         AddonInstance_Game &addonStruct,
                         CCriticalSection &clientAccess);

    virtual ~CGameClientSubsystem();

  public:
    /*!
     * \brief Create a struct with the allocated subsystems
     *
     * \param gameClient The owner of the subsystems
     * \param gameStruct The game client's add-on function table
     * \param clientAccess Mutex guarding client function access
     *
     * \return A fully-allocated GameClientSubsystems struct
     */
    static GameClientSubsystems CreateSubsystems(CGameClient &gameClient, AddonInstance_Game &gameStruct, CCriticalSection &clientAccess);

    /*!
     * \brief Deallocate subsystems
     *
     * \param subsystems The subsystems created by CreateSubsystems()
     */
    static void DestroySubsystems(GameClientSubsystems &subsystems);

  protected:
    // Subsystems
    CGameClientInput &Input() const;
    CGameClientProperties &AddonProperties() const;
    CGameClientStreams &Streams() const;

    // Construction parameters
    CGameClient &m_gameClient;
    AddonInstance_Game &m_struct;
    CCriticalSection &m_clientAccess;
  };

}
}
