/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "GameClientSubsystem.h"
#include "addons/binary-addons/AddonDll.h"
#include "addons/kodi-addon-dev-kit/include/kodi/kodi_game_types.h"
#include "threads/CriticalSection.h"

#include <atomic>
#include <memory>
#include <set>
#include <stdint.h>
#include <string>

class CFileItem;

namespace KODI
{
namespace RETRO
{
  class IStreamManager;
}

namespace GAME
{

class CGameClientInGameSaves;
class CGameClientInput;
class CGameClientProperties;
class IGameInputCallback;

/*!
 * \ingroup games
 * \brief Interface between Kodi and Game add-ons.
 */
class CGameClient : public ADDON::CAddonDll
{
public:
  static std::unique_ptr<CGameClient> FromExtension(ADDON::CAddonInfo addonInfo, const cp_extension_t* ext);

  explicit CGameClient(ADDON::CAddonInfo addonInfo);

  virtual ~CGameClient(void);

  // Game subsystems (const)
  const CGameClientInput &Input() const { return *m_subsystems.Input; }
  const CGameClientProperties &AddonProperties() const { return *m_subsystems.AddonProperties; }
  const CGameClientStreams &Streams() const { return *m_subsystems.Streams; }

  // Game subsystems (mutable)
  CGameClientInput &Input() { return *m_subsystems.Input; }
  CGameClientProperties &AddonProperties() { return *m_subsystems.AddonProperties; }
  CGameClientStreams &Streams() { return *m_subsystems.Streams; }

  // Implementation of IAddon via CAddonDll
  virtual std::string     LibPath() const override;
  virtual ADDON::AddonPtr GetRunningInstance() const override;

  // Query properties of the game client
  bool SupportsStandalone() const { return m_bSupportsStandalone; }
  bool SupportsPath() const;
  bool SupportsVFS() const { return m_bSupportsVFS; }
  const std::set<std::string>& GetExtensions() const { return m_extensions; }
  bool SupportsAllExtensions() const { return m_bSupportsAllExtensions; }
  bool IsExtensionValid(const std::string& strExtension) const;

  // Start/stop gameplay
  bool Initialize(void);
  void Unload();
  bool OpenFile(const CFileItem& file, RETRO::IStreamManager& streamManager, IGameInputCallback *input);
  bool OpenStandalone(RETRO::IStreamManager& streamManager, IGameInputCallback *input);
  void Reset();
  void CloseFile();
  const std::string& GetGamePath() const { return m_gamePath; }

  // Playback control
  bool RequiresGameLoop() const { return m_bRequiresGameLoop; }
  bool IsPlaying() const { return m_bIsPlaying; }
  size_t GetSerializeSize() const { return m_serializeSize; }
  double GetFrameRate() const { return m_framerate; }
  double GetSampleRate() const { return m_samplerate; }
  void RunFrame();

  // Access memory
  size_t SerializeSize() const { return m_serializeSize; }
  bool Serialize(uint8_t* data, size_t size);
  bool Deserialize(const uint8_t* data, size_t size);

  /*!
    * @brief To get the interface table used between addon and kodi
    * @todo This function becomes removed after old callback library system
    * is removed.
    */
  AddonInstance_Game* GetInstanceInterface() { return &m_struct; }

  // Helper functions
  bool LogError(GAME_ERROR error, const char* strMethod) const;
  void LogException(const char* strFunctionName) const;

private:
  // Private gameplay functions
  bool InitializeGameplay(const std::string& gamePath, RETRO::IStreamManager& streamManager, IGameInputCallback *input);
  bool LoadGameInfo();
  void NotifyError(GAME_ERROR error);
  std::string GetMissingResource();

  // Helper functions
  void LogAddonProperties(void) const;

  /*!
   * @brief Callback functions from addon to kodi
   */
  //@{
  static void cb_close_game(void* kodiInstance);
  static void* cb_open_stream(void* kodiInstance, const game_stream_properties *properties);
  static bool cb_get_stream_buffer(void* kodiInstance, void *stream, unsigned int width, unsigned int height, game_stream_buffer *buffer);
  static void cb_add_stream_data(void* kodiInstance, void *stream, const game_stream_packet *packet);
  static void cb_release_stream_buffer(void* kodiInstance, void *stream, game_stream_buffer *buffer);
  static void cb_close_stream(void* kodiInstance, void *stream);
  static game_proc_address_t cb_hw_get_proc_address(void* kodiInstance, const char* sym);
  static bool cb_input_event(void* kodiInstance, const game_input_event* event);
  //@}

  // Game subsystems
  GameClientSubsystems m_subsystems;

  // Game API xml parameters
  bool                  m_bSupportsVFS;
  bool                  m_bSupportsStandalone;
  std::set<std::string> m_extensions;
  bool                  m_bSupportsAllExtensions;
  //GamePlatforms         m_platforms;

  // Properties of the current playing file
  std::atomic_bool      m_bIsPlaying;          // True between OpenFile() and CloseFile()
  std::string           m_gamePath;
  bool                  m_bRequiresGameLoop = false;
  size_t                m_serializeSize;
  IGameInputCallback*   m_input = nullptr;     // The input callback passed to OpenFile()
  double                m_framerate = 0.0;     // Video frame rate (fps)
  double                m_samplerate = 0.0;    // Audio sample rate (Hz)
  GAME_REGION           m_region;              // Region of the loaded game

  // In-game saves
  std::unique_ptr<CGameClientInGameSaves> m_inGameSaves;

  CCriticalSection m_critSection;

  AddonInstance_Game m_struct;
};

} // namespace GAME
} // namespace KODI
