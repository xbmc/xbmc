/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
class CGameClientCheevos;
class CGameClientInput;
class CGameClientProperties;
class CGameClientStreams;

struct GameClientSubsystems
{
  std::unique_ptr<CGameClientCheevos> Cheevos;
  std::unique_ptr<CGameClientInput> Input;
  std::unique_ptr<CGameClientProperties> AddonProperties;
  std::unique_ptr<CGameClientStreams> Streams;
};

/*!
 * \ingroup games
 *
 * \brief Base class for game client subsystems
 */
class CGameClientSubsystem
{
protected:
  CGameClientSubsystem(CGameClient& gameClient,
                       AddonInstance_Game& addonStruct,
                       CCriticalSection& clientAccess);

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
  static GameClientSubsystems CreateSubsystems(CGameClient& gameClient,
                                               AddonInstance_Game& gameStruct,
                                               CCriticalSection& clientAccess);

  /*!
   * \brief Deallocate subsystems
   *
   * \param subsystems The subsystems created by CreateSubsystems()
   */
  static void DestroySubsystems(GameClientSubsystems& subsystems);

protected:
  // Subsystems
  CGameClientCheevos& Cheevos() const;
  CGameClientInput& Input() const;
  CGameClientProperties& AddonProperties() const;
  CGameClientStreams& Streams() const;

  // Construction parameters
  CGameClient& m_gameClient;
  AddonInstance_Game& m_struct;
  CCriticalSection& m_clientAccess;
};

} // namespace GAME
} // namespace KODI
