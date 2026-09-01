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
#include "addons/kodi-dev-kit/include/kodi/addon-instance/Game.h"
#include "games/GameTypes.h"
#include "games/addons/streams/GameClientStreamHwFramebuffer.h"
#include "threads/CriticalSection.h"

#include <atomic>
#include <memory>
#include <set>
#include <stdint.h>
#include <string>
#include <utility>

class CFileItem;

namespace KODI
{
namespace RETRO
{
class IStreamManager;
}

namespace GAME
{

class CGameClientCheevos;
class CGameClientInGameSaves;
class CGameClientInput;
class CGameClientProperties;
class IGameInputCallback;

/*!
 * \ingroup games
 *
 * \brief Helper class to have "C" struct created before other parts becomes his pointer.
 */
class CGameClientStruct
{
public:
  CGameClientStruct()
  {
    // Create "C" interface structures, used as own parts to prevent API problems on update
    KODI_ADDON_INSTANCE_INFO* info = new KODI_ADDON_INSTANCE_INFO();
    info->id = "";
    info->version = kodi::addon::GetTypeVersion(ADDON_INSTANCE_GAME);
    info->type = ADDON_INSTANCE_GAME;
    info->kodi = this;
    info->parent = nullptr;
    info->first_instance = true;
    info->functions = new KODI_ADDON_INSTANCE_FUNC_CB();
    m_ifc.info = info;
    m_ifc.functions = new KODI_ADDON_INSTANCE_FUNC();

    m_ifc.game = new AddonInstance_Game;
    m_ifc.game->props = new AddonProps_Game();
    m_ifc.game->toKodi = new AddonToKodiFuncTable_Game();
    m_ifc.game->toAddon = new KodiToAddonFuncTable_Game();
  }

  ~CGameClientStruct()
  {
    delete m_ifc.functions;
    if (m_ifc.info)
      delete m_ifc.info->functions;
    delete m_ifc.info;
    if (m_ifc.game)
    {
      delete m_ifc.game->toAddon;
      delete m_ifc.game->toKodi;
      delete m_ifc.game->props;
      delete m_ifc.game;
    }
  }

  KODI_ADDON_INSTANCE_STRUCT m_ifc;
};

/*!
 * \ingroup games
 * \brief Interface between Kodi and Game add-ons.
 *
 * The game add-on system is extremely large. To make the code more manageable,
 * a subsystem pattern has been put in place. This pattern takes functionality
 * that would normally be placed in this class, and puts it in another class
 * (a "subsystem").
 *
 * The architecture is relatively simple. Subsystems are placed in a flat
 * struct and accessed by calling the subsystem name. For example,
 * historically, OpenJoystick() was a member of this class. Now, the function
 * is called like Input().OpenJoystick().
 *
 * Although this pattern adds a layer of complexity, it enforces modularity and
 * separation of concerns by making it very clear when one subsystem becomes
 * dependent on another. Subsystems are all given access to each other by the
 * calling mechanism. However, calling a subsystem creates a dependency on it,
 * and an engineering decision must be made to justify the dependency.
 *
 * CONTRIBUTING
 *
 * If you wish to contribute, a beneficial task would be to refactor anything
 * in this class into a new subsystem:
 *
 * Using line count as a heuristic, the subsystem pattern has shrunk the .cpp
 * from 1,200 lines to just over 600. Reducing this further is the challenge.
 * You must now choose whether to accept.
 */
class CGameClient : public ADDON::CAddonDll,
                    public IHwFramebufferCallback,
                    private CGameClientStruct
{
public:
  explicit CGameClient(const ADDON::AddonInfoPtr& addonInfo);

  ~CGameClient() override;

  // Game subsystems (const)
  const CGameClientCheevos& Cheevos() const { return *m_subsystems.Cheevos; }
  const CGameClientDiscs& Discs() const { return *m_subsystems.Discs; }
  const CGameClientInput& Input() const { return *m_subsystems.Input; }
  const CGameClientProperties& AddonProperties() const { return *m_subsystems.AddonProperties; }
  const CGameClientStreams& Streams() const { return *m_subsystems.Streams; }

  // Game subsystems (mutable)
  CGameClientCheevos& Cheevos() { return *m_subsystems.Cheevos; }
  CGameClientDiscs& Discs() { return *m_subsystems.Discs; }
  CGameClientInput& Input() { return *m_subsystems.Input; }
  CGameClientProperties& AddonProperties() { return *m_subsystems.AddonProperties; }
  CGameClientStreams& Streams() { return *m_subsystems.Streams; }

  // Implementation of IAddon via CAddonDll
  std::string LibPath() const override;
  ADDON::AddonPtr GetRunningInstance() const override;

  // Query properties of the game client
  bool SupportsStandalone() const { return m_bSupportsStandalone; }
  bool SupportsPath() const;
  bool SupportsVFS() const { return m_bSupportsVFS; }
  const std::set<std::string>& GetExtensions() const { return m_extensions; }
  bool SupportsAllExtensions() const { return m_bSupportsAllExtensions; }
  bool IsExtensionValid(const std::string& strExtension) const;
  const std::string& GetEmulatorName() const { return m_emulatorName; }
  const std::string& GetPlatforms() const { return m_platforms; }
  bool SupportsDiscControl() const { return m_supportsDiscControl; }

  // Start/stop gameplay
  bool Initialize(void);
  void Unload();
  bool OpenFile(const CFileItem& file,
                RETRO::IStreamManager& streamManager,
                IGameInputCallback* input);
  bool OpenStandalone(RETRO::IStreamManager& streamManager, IGameInputCallback* input);
  void Reset();
  void CloseFile();
  const std::string& GetGamePath() const { return m_gamePath; }

  // Playback control
  bool RequiresGameLoop() const { return m_bRequiresGameLoop; }
  bool IsPlaying() const { return m_bIsPlaying; }
  size_t GetSerializeSize() const { return m_serializeSize; }
  double GetFrameRate() const { return m_framerate.load(); }
  double GetSampleRate() const { return m_samplerate.load(); }
  void RunFrame();

  /*!
   * \brief Tell the client what speed the player is running at
   *
   * \param speed The speed as a multiple of normal speed, with the player's own
   *              meaning: 1.0 normal, 0.0 paused, above 1.0 fast-forward,
   *              between 0.0 and 1.0 slow motion, below 0.0 rewind
   *
   * Kept so the client can answer for it when asked. Clients that care use it
   * to drop work the user will not see at speed, and the audio limiter is the
   * usual one.
   */
  void SetPlaybackSpeed(double speed) { m_playbackSpeed = speed; }

  // Access memory
  size_t SerializeSize() const { return m_serializeSize; }
  bool Serialize(uint8_t* data, size_t size);
  bool Deserialize(const uint8_t* data, size_t size);

  /*!
   * \brief Hold the client still for the duration of a savestate snapshot
   *
   * The emulator's memory and the achievement state have to describe the same
   * frame. Both are read from a worker thread while the game loop runs, so
   * locking each read on its own is not enough - RunFrame() would still be
   * free to advance between them, pairing one frame's memory with another
   * frame's achievement progress.
   *
   * The lock is recursive, so the calls made while holding it may take it
   * again.
   */
  std::unique_lock<CCriticalSection> LockForSnapshot() { return std::unique_lock(m_critSection); }

  /*!
   * \brief Take a snapshot of the client's achievement state
   *
   * Separate from the emulator's state, which must keep the exact size the
   * client reports. See savestate.fbs.
   *
   * \param[out] data The state, emptied if there is none
   *
   * \return True if state was captured, false if the client has none
   */
  bool SerializeAchievementState(std::vector<uint8_t>& data);

  /*!
   * \brief Restore the client's achievement state after a savestate load
   *
   * Called for every load, including savestates that carry no achievement
   * data: the client has to know the machine state jumped either way.
   */
  bool DeserializeAchievements(const uint8_t* data, size_t size);

  /*!
   * \brief Give the client the RetroAchievements account to sign in with
   *
   * The account is held by Kodi, which owns the settings it is entered in.
   */
  bool SetRetroAchievementsCredentials(const std::string& username, const std::string& token);

  // Implementation of IHwFramebufferCallback
  void HardwareContextReset() override;

  /*!
   * @brief To get the interface table used between addon and kodi
   * @todo This function becomes removed after old callback library system
   * is removed.
   */
  AddonInstance_Game* GetInstanceInterface() { return m_ifc.game; }

  // Helper functions
  bool LogError(GAME_ERROR error, const char* strMethod) const;
  void LogException(const char* strFunctionName) const;

private:
  // Private gameplay functions
  bool InitializeGameplay(const std::string& gamePath,
                          RETRO::IStreamManager& streamManager,
                          IGameInputCallback* input);
  bool LoadGameInfo();
  void NotifyError(GAME_ERROR error);
  std::string GetMissingResource();

  // Helper functions
  void LogAddonProperties(void) const;

  /*!
   * @brief Callback functions from addon to kodi
   */
  //@{
  static bool cb_enable_hardware_rendering(void* kodiInstance,
                                           const game_hw_rendering_properties* properties);
  static void cb_close_game(KODI_HANDLE kodiInstance);
  static double cb_get_playback_speed(KODI_HANDLE kodiInstance);
  static void cb_set_game_timing(KODI_HANDLE kodiInstance, const game_system_timing* timingInfo);
  static KODI_GAME_STREAM_HANDLE cb_open_stream(KODI_HANDLE kodiInstance,
                                                const game_stream_properties* properties);
  static bool cb_get_stream_buffer(KODI_HANDLE kodiInstance,
                                   KODI_GAME_STREAM_HANDLE stream,
                                   unsigned int width,
                                   unsigned int height,
                                   game_stream_buffer* buffer);
  static void cb_add_stream_data(KODI_HANDLE kodiInstance,
                                 KODI_GAME_STREAM_HANDLE stream,
                                 const game_stream_packet* packet);
  static void cb_release_stream_buffer(KODI_HANDLE kodiInstance,
                                       KODI_GAME_STREAM_HANDLE stream,
                                       game_stream_buffer* buffer);
  static void cb_close_stream(KODI_HANDLE kodiInstance, KODI_GAME_STREAM_HANDLE stream);
  static game_proc_address_t cb_hw_get_proc_address(KODI_HANDLE kodiInstance, const char* sym);
  static bool cb_input_event(KODI_HANDLE kodiInstance, const game_input_event* event);
  static void cb_rc_on_game_loaded(KODI_HANDLE kodiInstance, const game_rc_game_loaded* data);
  static void cb_rc_on_achievement_triggered(KODI_HANDLE kodiInstance,
                                             const game_rc_achievement_triggered* data);
  static void cb_rc_on_game_completed(KODI_HANDLE kodiInstance, const char* title, bool hardcore);
  static void cb_rc_on_rich_presence_updated(KODI_HANDLE kodiInstance, const char* evaluation);
  static void cb_rc_on_login_result(KODI_HANDLE kodiInstance, const game_rc_login_result* data);
  static void cb_rc_on_achievement_progress(KODI_HANDLE kodiInstance,
                                            const game_rc_achievement_progress* progress,
                                            unsigned int count);
  static void cb_rc_on_server_error(KODI_HANDLE kodiInstance, const char* message, const char* api);
  static void cb_rc_on_connection_changed(KODI_HANDLE kodiInstance, bool connected);
  static void cb_rc_on_progress_indicator(KODI_HANDLE kodiInstance,
                                          const game_rc_progress_indicator* indicator);
  static void cb_rc_on_challenge_indicator(KODI_HANDLE kodiInstance,
                                           const game_rc_challenge_indicator* indicator);
  //@}

  /*!
   * \brief Parse the name of a libretro game add-on into its emulator name
   * and platforms
   *
   * \param addonName The name of the add-on, e.g. "Nintendo - SNES / SFC (Snes9x 2002)"
   *
   * \return A tuple of two strings:
   *   - first:  the emulator name, e.g. "Snes9x 2002"
   *   - second: the platforms, e.g. "Nintendo - SNES / SFC"
   *
   * For cores that don't emulate a platform, such as 2048 with the add-on name
   * "2048", then the emulator name will be the add-on name and platforms
   * will be empty, e.g.:
   *   - first:  "2048"
   *   - second: ""
   */
  static std::pair<std::string, std::string> ParseLibretroName(const std::string& addonName);

  // Game subsystems
  GameClientSubsystems m_subsystems;

  // Game API xml parameters
  bool m_bSupportsVFS;
  bool m_bSupportsStandalone;
  std::set<std::string> m_extensions;
  bool m_bSupportsAllExtensions = false;
  std::string m_emulatorName;
  std::string m_platforms;
  bool m_supportsDiscControl{false};

  // Properties of the current playing file
  std::atomic_bool m_bIsPlaying; // True between OpenFile() and CloseFile()
  std::atomic_bool m_hasFrameRun{false};
  // The speed the player is running at, as a multiple of normal speed. Written
  // by the thread that changes the speed and read by the client's own, so it is
  // atomic; a client asks for it from inside a call of its own, which can be on
  // any thread.
  //
  // Starts at normal speed rather than zero because clients ask this while
  // loading, before the player has set a speed for the first time, and the
  // honest answer to "am I being fast-forwarded" at that point is no. Zero
  // would read as paused and have a client behave as though the user had
  // stopped the game before it started.
  std::atomic<double> m_playbackSpeed{1.0};
  std::string m_gamePath;
  bool m_bRequiresGameLoop = false;
  size_t m_serializeSize = 0;
  IGameInputCallback* m_input = nullptr; // The input callback passed to OpenFile()
  std::atomic<double> m_framerate{0.0}; // Video frame rate (fps)
  std::atomic<double> m_samplerate{0.0}; // Audio sample rate (Hz)
  GAME_REGION m_region = GAME_REGION_UNKNOWN; // Region of the loaded game

  // In-game saves
  std::unique_ptr<CGameClientInGameSaves> m_inGameSaves;

  CCriticalSection m_critSection;
};

} // namespace GAME
} // namespace KODI
