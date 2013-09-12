/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "addons/Addon.h"
#include "FileItem.h"
#include "GameClientDLL.h"
#include "GameFileLoader.h"
#include "games/libretro/LibretroCallbacks.h"
#include "games/tags/GameInfoTagLoader.h"
#include "SerialState.h"
#include "threads/CriticalSection.h"

#include <boost/shared_ptr.hpp>
#include <set>
#include <string>

#define GAMECLIENT_MAX_PLAYERS  8

namespace GAMES
{
  class CGameClient;
  typedef boost::shared_ptr<CGameClient> GameClientPtr;

  class CGameClient : public ADDON::CAddon, public ILibretroCallbacksDLL
  {
  public:
    CGameClient(const ADDON::AddonProps &props);
    CGameClient(const cp_extension_t *props);
    virtual ~CGameClient() { DeInit(); }

    virtual ADDON::AddonPtr Clone() const;

    /**
     * Load the DLL and query basic parameters. After Init() is called, the
     * Get*() functions may be called.
     */
    bool Init();

    // Cleanly shut down and unload the DLL.
    void DeInit();

    /**
     * Perform the gamut of checks on the file: "gameclient" property, platform,
     * extension, and a positive match on at least one of the CGameFileLoader
     * strategies.
     */
    bool CanOpen(const CFileItem &file) const;

    bool OpenFile(const CFileItem &file, ILibretroCallbacksAV *callbacks);
    void CloseFile();

    /**
     * Settings and strings are handled slightly differently with game client
     * because they all share the possibility of having a system directory.
     * Trivial saves are skipped to avoid unnecessary file creations, and
     * strings simply use g_localizeStrings.
     * \sa LoadSettings
     */
    virtual void SaveSettings();
    virtual CStdString GetString(uint32_t id);

    const std::set<std::string>        &GetExtensions() const { return m_extensions; }
    const GAME_INFO::GamePlatformArray &GetPlatforms() const { return m_platforms; }
    bool                               AllowVFS() const { return m_bAllowVFS; }
    bool                               RequireZip() const { return m_bRequireZip; }

    const std::string                  &GetFilePath() const { return m_gameFile.Path(); }
    // Returns true after Init() is called and until DeInit() is called.
    bool                               IsInitialized() const { return m_dll.IsLoaded(); }
    // Precondition: Init() must be called first and return true.
    const std::string                  &GetClientName() const { return m_clientName; }
    // Precondition: Init() must be called first and return true.
    const std::string                  &GetClientVersion() const { return m_clientVersion; }
    /**
     * Find the region of a currently running game. The return value will be
     * RETRO_REGION_NTSC, RETRO_REGION_PAL or -1 for invalid.
     */
    int                                GetRegion() const { return m_region; }

    /**
     * Each port (or player, if you will) must be associated with a device. The
     * default device is RETRO_DEVICE_JOYPAD. For a list of valid devices, see
     * libretro.h.
     *
     * Do not exceed the number of devices that the game client supports. A
     * quick analysis of SNES9x Next v2 showed that a third port will overflow
     * a buffer. Currently, there is no way to determine the number of ports a
     * client will support, so stick with 1.
     *
     * Precondition: OpenFile() must return true.
     */
    void SetDevice(unsigned int port, unsigned int device);

    /**
     * Allow the game to run and produce a video frame.
     * Precondition: OpenFile() returned true.
     * Returns false if an exception is thrown in retro_run().
     */
    bool RunFrame();

    /**
     * Rewind gameplay 'frames' frames.
     * As there is a fixed size buffer backing
     * save state deltas, it might not be possible to rewind as many
     * frames as desired. The function returns number of frames actually rewound.
     */
    unsigned int RewindFrames(unsigned int frames);

    // Returns how many frames it is possible to rewind with a call to RewindFrames().
    size_t GetAvailableFrames() const { return m_bRewindEnabled ? m_serialState.GetFramesAvailable() : 0; }

    // Returns the maximum amount of frames that can ever be rewound.
    size_t GetMaxFrames() const { return m_bRewindEnabled ? m_serialState.GetMaxFrames() : 0; }

    // Reset the game, if running.
    void Reset();

    // Video framerate is used to calculate savestate wall time
    double GetFrameRate() const { return m_frameRate * m_frameRateCorrection; }
    void SetFrameRateCorrection(double correctionFactor);
    double GetSampleRate() const { return m_sampleRate; }

    /**
     * If the game client was a bad boy and provided no extensions, this will
     * optimistically return true.
     */
    bool IsExtensionValid(const std::string &ext) const;

    // Inherited from ILibretroCallbacksDLL
    virtual void SetPixelFormat(LIBRETRO::retro_pixel_format format);
    virtual void SetKeyboardCallback(LIBRETRO::retro_keyboard_event_t callback);
    virtual void SetDiskControlCallback(const LIBRETRO::retro_disk_control_callback *callback_struct) { } // TODO
    virtual void SetRenderCallback(const LIBRETRO::retro_hw_render_callback *callback_struct) { } // TODO

    // Static wrappers for ILibretroCallbacksAV
    static void VideoFrame(const void *data, unsigned width, unsigned height, size_t pitch);
    static void AudioSample(int16_t left, int16_t right);
    static size_t AudioSampleBatch(const int16_t *data, size_t frames);
    static int16_t GetInputState(unsigned port, unsigned device, unsigned index, unsigned id);

  protected:
    CGameClient(const CGameClient &other);
    virtual bool LoadSettings(bool bForce = false);

  private:
    void Initialize();

    /**
     * Perform the actual loading of the game by the DLL. The resulting CGameFile
     * is placed in m_gameFile.
     */
    bool OpenInternal(const CFileItem& file);

    /**
     * Calls retro_get_system_av_info() and prints the game/environment info on
     * the screen. The framerate and samplerate are stored in m_frameRate and
     * m_sampleRate.
     */
    void LoadGameInfo();

    /**
     * Initialize the game client serialization subsystem. If successful,
     * m_bRewindEnabled and m_serialSize are set appropriately.
     */
    void InitSerialization();

    /**
     * Given the strategies above, order them in the way that respects
     * CSettings::Get().GetBool("gamesdebug.prefervfs").
     */
    static void GetStrategy(CGameFileLoaderUseHD &hd, CGameFileLoaderUseParentZip &outerzip,
        CGameFileLoaderUseVFS &vfs, CGameFileLoaderEnterZip &innerzip, CGameFileLoader *strategies[4]);

    /**
     * Parse a pipe-separated list, returned from the game client, into an
     * array. The extensions list can contain both upper and lower case
     * extensions; only lower-case extensions are stored in m_validExtensions.
     */
    void SetExtensions(const std::string &strExtensionList);
    void SetPlatforms(const std::string &strPlatformList);

    /**
     * m_extensions, m_platforms, m_bAllowVFS, and m_bRequireZip are the core
     * configuration parameters of game clients are listed here first. These
     * are the fields listed in addon.xml, and are the fields that are consulted
     * when deciding if this game client supports a particular game file.
     * CGameFileLoader doesn't touch any other part of the game client (with
     * the exception of ID).
     */
    std::set<std::string>        m_extensions;
    GAME_INFO::GamePlatformArray m_platforms;
    bool                         m_bAllowVFS;
    /**
      * If false, and ROM is in a zip, ROM file must be loaded from within the
      * zip instead of extracted to a temporary cache. In XBMC's case, loading
      * from the VFS is like extraction because the relative paths to the
      * ROM's other files are not available to the emulator.
      */
    bool                         m_bRequireZip;

    GameClientDLL                m_dll;
    static ILibretroCallbacksAV  *m_callbacks;
    bool                         m_bIsInited; // Keep track of whether m_dll.retro_init() has been called
    bool                         m_bIsPlaying; // This is true between retro_load_game() and retro_unload_game()
    CGameFile                    m_gameFile; // the current playing file

    // Returned by m_dll
    std::string                  m_clientName;
    std::string                  m_clientVersion;
    double                       m_frameRate; // Video framerate
    double                       m_frameRateCorrection; // Framerate correction factor (to sync to audio)
    double                       m_sampleRate; // Audio frequency
    int                          m_region; // Region of the loaded game

    CCriticalSection             m_critSection;
    unsigned int                 m_serialSize;
    bool                         m_bRewindEnabled;
    CSerialState                 m_serialState;

    // If rewinding is disabled, use a buffer to avoid re-allocation when saving games
    std::vector<uint8_t>         m_savestateBuffer;

    /**
     * This callback exists to give XBMC a chance to poll for input. XBMC already
     * takes care of this, so the callback isn't needed.
     */
    static void NoopPoop() { }
  };
}
