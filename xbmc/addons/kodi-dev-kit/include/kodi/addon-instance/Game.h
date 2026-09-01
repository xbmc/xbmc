/*
 *  Copyright (C) 2014-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/addon-instance/game.h"

#include <algorithm>
#include <functional>

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @addtogroup cpp_kodi_addon_game
///
/// For use with Libretro cores and with stand-alone games or emulators that do
/// not use the Libretro API.
///
/// Possible examples include Nvidia GameStream via Limelight or capturing WINE
/// games through the Game API.
///

//==============================================================================
/// @defgroup cpp_kodi_addon_game_Defs Definitions, structures and enumerators
/// @ingroup cpp_kodi_addon_game
/// @brief **Game add-on instance definition values**
///

//==============================================================================
/// @defgroup cpp_kodi_addon_game_Defs_InputTypes_GameControllerLayout class GameControllerLayout
/// @ingroup cpp_kodi_addon_game_Defs_InputTypes
/// @brief Data of layouts for known controllers.
///
/// Used on @ref kodi::addon::CInstanceGame::SetControllerLayouts().
///@{
class GameControllerLayout
{
public:
  /*! @cond PRIVATE */
  explicit GameControllerLayout() = default;
  GameControllerLayout(const game_controller_layout& layout) : controller_id(layout.controller_id)
  {
    provides_input = layout.provides_input;
    for (unsigned int i = 0; i < layout.digital_button_count; ++i)
      digital_buttons.emplace_back(layout.digital_buttons[i]);
    for (unsigned int i = 0; i < layout.analog_button_count; ++i)
      analog_buttons.emplace_back(layout.analog_buttons[i]);
    for (unsigned int i = 0; i < layout.analog_stick_count; ++i)
      analog_sticks.emplace_back(layout.analog_sticks[i]);
    for (unsigned int i = 0; i < layout.accelerometer_count; ++i)
      accelerometers.emplace_back(layout.accelerometers[i]);
    for (unsigned int i = 0; i < layout.key_count; ++i)
      keys.emplace_back(layout.keys[i]);
    for (unsigned int i = 0; i < layout.rel_pointer_count; ++i)
      rel_pointers.emplace_back(layout.rel_pointers[i]);
    for (unsigned int i = 0; i < layout.abs_pointer_count; ++i)
      abs_pointers.emplace_back(layout.abs_pointers[i]);
    for (unsigned int i = 0; i < layout.motor_count; ++i)
      motors.emplace_back(layout.motors[i]);
  }
  /*! @endcond */

  /// @brief Controller identifier
  std::string controller_id;

  /// @brief Provides input
  ///
  /// False for multitaps
  bool provides_input{false};

  /// @brief Digital buttons
  std::vector<std::string> digital_buttons;

  /// @brief Analog buttons
  std::vector<std::string> analog_buttons;

  /// @brief Analog sticks
  std::vector<std::string> analog_sticks;

  /// @brief Accelerometers
  std::vector<std::string> accelerometers;

  /// @brief Keys
  std::vector<std::string> keys;

  /// @brief Relative pointers
  std::vector<std::string> rel_pointers;

  /// @brief Absolute pointers
  std::vector<std::string> abs_pointers;

  /// @brief Motors
  std::vector<std::string> motors;
};
///@}
//------------------------------------------------------------------------------

//==============================================================================
/// @addtogroup cpp_kodi_addon_game
/// @brief @cpp_class{ kodi::addon::CInstanceGame }
/// **Game add-on instance**\n
/// This class provides the basic game processing system for use as an add-on in
/// Kodi.
///
/// Kodi creates this class in the add-on.
///
class ATTR_DLL_LOCAL CInstanceGame : public IAddonInstance
{
public:
  //============================================================================
  /// @defgroup cpp_kodi_addon_game_Base 1. Basic functions
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Functions to manage the addon and get basic information about it**
  ///
  ///@{

  //============================================================================
  /// @brief Game class constructor
  ///
  /// Used by an add-on that supports only one game instance.
  ///
  /// Kodi creates this class in the add-on.
  ///
  /// --------------------------------------------------------------------------
  ///
  /// **Here's an example of how to use this:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/addon-instance/Game.h>
  /// ...
  ///
  /// class ATTR_DLL_LOCAL CGameExample
  ///   : public kodi::addon::CAddonBase,
  ///     public kodi::addon::CInstanceGame
  /// {
  /// public:
  ///   CGameExample()
  ///   {
  ///   }
  ///
  ///   virtual ~CGameExample();
  ///   {
  ///   }
  ///
  ///   ...
  /// };
  ///
  /// ADDONCREATOR(CGameExample)
  /// ~~~~~~~~~~~~~
  ///
  CInstanceGame() : IAddonInstance(IInstanceInfo(CPrivateBase::m_interface->firstKodiInstance))
  {
    if (CPrivateBase::m_interface->globalSingleInstance != nullptr)
    {
      throw std::logic_error(
          "kodi::addon::CInstanceGame: Cannot create more than one game instance!");
    }
    SetAddonStruct(CPrivateBase::m_interface->firstKodiInstance);
    CPrivateBase::m_interface->globalSingleInstance = this;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Destructor
  ///
  ~CInstanceGame() override = default;
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// The path of the game client being loaded.
  ///
  /// @return The game client DLL path
  ///
  /// @remarks Only called from the add-on itself
  ///
  std::string GameClientDllPath() const { return m_instanceData->props->game_client_dll_path; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Paths to proxy DLLs used to load the game client.
  ///
  /// @param[out] paths Vector list to store available DLL paths
  ///
  /// @return True if DLL paths were found, false otherwise
  ///
  /// @remarks Only called from the add-on itself
  ///
  bool ProxyDllPaths(std::vector<std::string>& paths)
  {
    for (unsigned int i = 0; i < m_instanceData->props->proxy_dll_count; ++i)
    {
      if (m_instanceData->props->proxy_dll_paths[i] != nullptr)
        paths.emplace_back(m_instanceData->props->proxy_dll_paths[i]);
    }
    return !paths.empty();
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// The "system" directories of the frontend.
  ///
  /// These directories can be used to store system-specific ROMs such as
  /// BIOSes, configuration data, etc.
  ///
  /// @param[out] dirs Vector list to store available resource directories
  ///
  /// @return True if resource directories were found, false otherwise
  ///
  /// @remarks Only called from the add-on itself
  ///
  bool ResourceDirectories(std::vector<std::string>& dirs)
  {
    for (unsigned int i = 0; i < m_instanceData->props->resource_directory_count; ++i)
    {
      if (m_instanceData->props->resource_directories[i] != nullptr)
        dirs.emplace_back(m_instanceData->props->resource_directories[i]);
    }
    return !dirs.empty();
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// The writable directory of the frontend.
  ///
  /// This directory can be used to store SRAM, memory cards, high scores,
  /// etc, if the game client cannot use the regular memory interface,
  /// GetMemoryData().
  ///
  /// @return The profile directory
  ///
  /// @remarks Only called from the add-on itself
  ///
  std::string ProfileDirectory() const { return m_instanceData->props->profile_directory; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// The value of the <supports_vfs> property from addon.xml.
  ///
  /// @return True if VFS is supported, false otherwise
  ///
  /// @remarks Only called from the add-on itself
  ///
  bool SupportsVFS() const { return m_instanceData->props->supports_vfs; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// The extensions in the <extensions> property from addon.xml.
  ///
  /// @param[out] extensions Vector list to store available extensions
  ///
  /// @return True if successful and extensions were found, false otherwise
  ///
  /// @remarks Only called from the add-on itself
  ///
  bool Extensions(std::vector<std::string>& extensions)
  {
    for (unsigned int i = 0; i < m_instanceData->props->extension_count; ++i)
    {
      if (m_instanceData->props->extensions[i] != nullptr)
        extensions.emplace_back(m_instanceData->props->extensions[i]);
    }
    return !extensions.empty();
  }
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  ///
  /// @defgroup cpp_kodi_addon_game_Operation 2. Game operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Game operations**
  ///
  /// These are mandatory functions for using this add-on for gameplay
  /// functionality.
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Game operation parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_game_Operation_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_game_Operation_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Load a game
  ///
  /// @param[in] url The URL to load
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the game was loaded
  ///
  virtual GAME_ERROR LoadGame(const std::string& url) { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Load a game that requires multiple files
  ///
  /// @param[in] type The game type
  /// @param[in] urls An array of urls
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the game was loaded
  ///
  virtual GAME_ERROR LoadGameSpecial(SPECIAL_GAME_TYPE type, const std::vector<std::string>& urls)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Begin playing without a game file
  ///
  /// If the add-on supports standalone mode, it must add the <supports_standalone>
  /// tag to the extension point in addon.xml:
  ///
  ///     <supports_no_game>false</supports_no_game>
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the game add-on was loaded
  ///
  virtual GAME_ERROR LoadStandalone() { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Unload the current game
  ///
  /// Unloads a currently loaded game
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the game was unloaded
  ///
  virtual GAME_ERROR UnloadGame() { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get timing information about the loaded game
  ///
  /// @param[out] timing_info The info structure to fill
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if info was filled
  ///
  virtual GAME_ERROR GetGameTiming(game_system_timing& timing_info)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get region of the loaded game
  ///
  /// @return The region, or @ref GAME_REGION_UNKNOWN if unknown or no game is loaded
  ///
  virtual GAME_REGION GetRegion() { return GAME_REGION_UNKNOWN; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Return true if the client requires the frontend to provide a game loop
  ///
  /// The game loop is a thread that calls RunFrame() in a loop at a rate
  /// determined by the playback speed and the client's FPS.
  ///
  /// @return True if the frontend should provide a game loop, false otherwise
  ///
  virtual bool RequiresGameLoop() { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Run a single frame for add-ons that use a game loop
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if there was no error
  ///
  virtual GAME_ERROR RunFrame() { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Reset the current game
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the game was reset
  ///
  virtual GAME_ERROR Reset() { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Requests the frontend to stop the current game
  ///
  /// @remarks Only called from the add-on itself
  ///
  void CloseGame(void) { m_instanceData->toKodi->CloseGame(m_instanceData->toKodi->kodiInstance); }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Get the speed the game is being played back at
  ///
  /// @return The speed as a multiple of normal speed
  ///
  /// The value is the same one Kodi shows the user, so it follows the player's
  /// fast-forward and rewind semantics:
  ///
  ///   - `1.0` is normal speed
  ///   - `0.0` is paused
  ///   - greater than `1.0` is fast-forward
  ///   - between `0.0` and `1.0` is slow motion
  ///   - less than `0.0` is rewind
  ///
  /// Rewind is driven by Kodi replaying saved states, so a client is run
  /// forwards even while the speed is negative. A client that changes its
  /// behaviour with the speed should read the sign as "the user is going
  /// backwards", not as a direction to run in.
  ///
  /// @remarks Only called from the add-on itself
  ///
  double GetPlaybackSpeed()
  {
    return m_instanceData->toKodi->GetPlaybackSpeed(m_instanceData->toKodi->kodiInstance);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Update the video and audio timing of the running game
  ///
  /// @param[in] timingInfo The new video frame rate and audio sample rate
  ///
  /// This updates timing reported by the add-on after gameplay has started.
  /// Add-ons should call this when the game changes its timing dynamically.
  ///
  /// @remarks Only called from the add-on itself
  ///
  void SetGameTiming(const game_system_timing& timingInfo)
  {
    m_instanceData->toKodi->SetGameTiming(m_instanceData->toKodi->kodiInstance, &timingInfo);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_addon_game_Operation_CStream Class: CStream
  /// @ingroup cpp_kodi_addon_game_Operation
  /// @brief @cpp_class{ kodi::addon::CInstanceGame::CStream }
  /// **Game stream handler**
  ///
  /// This class will be integrated into the addon, which can then open it if
  /// necessary for the processing of an audio or video stream.
  ///
  /// @note Callback to Kodi class
  ///@{
  class CStream
  {
  public:
    CStream() = default;

    CStream(const game_stream_properties& properties) { Open(properties); }

    ~CStream() { Close(); }

    //==========================================================================
    /// @ingroup cpp_kodi_addon_game_Operation_CStream
    /// @brief Create a stream for gameplay data
    ///
    /// @param[in] properties The stream properties
    ///
    /// @return A stream handle, or `nullptr` on failure
    ///
    /// @remarks Only called from the add-on itself
    ///
    bool Open(const game_stream_properties& properties)
    {
      if (!CPrivateBase::m_interface->globalSingleInstance)
        return false;

      if (m_handle)
      {
        kodi::Log(ADDON_LOG_INFO, "kodi::addon::CInstanceGame::CStream already reopened");
        Close();
      }

      AddonToKodiFuncTable_Game& cb =
          *static_cast<CInstanceGame*>(CPrivateBase::m_interface->globalSingleInstance)
               ->m_instanceData->toKodi;
      m_handle = cb.OpenStream(cb.kodiInstance, &properties);
      return m_handle != nullptr;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    /// @ingroup cpp_kodi_addon_game_Operation_CStream
    /// @brief Free the specified stream
    ///
    /// @remarks Only called from the add-on itself
    ///
    void Close()
    {
      if (!m_handle || !CPrivateBase::m_interface->globalSingleInstance)
        return;

      AddonToKodiFuncTable_Game& cb =
          *static_cast<CInstanceGame*>(CPrivateBase::m_interface->globalSingleInstance)
               ->m_instanceData->toKodi;
      cb.CloseStream(cb.kodiInstance, m_handle);
      m_handle = nullptr;
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    /// @ingroup cpp_kodi_addon_game_Operation_CStream
    /// @brief Get a buffer for zero-copy stream data
    ///
    /// @param[in] width The framebuffer width, or 0 for no width specified
    /// @param[in] height The framebuffer height, or 0 for no height specified
    /// @param[out] buffer The buffer, or unmodified if false is returned
    ///
    /// @return True if buffer was set, false otherwise
    ///
    /// @note If this returns true, buffer must be freed using @ref ReleaseBuffer()
    ///
    /// @remarks Only called from the add-on itself
    ///
    bool GetBuffer(unsigned int width, unsigned int height, game_stream_buffer& buffer)
    {
      if (!m_handle || !CPrivateBase::m_interface->globalSingleInstance)
        return false;

      AddonToKodiFuncTable_Game& cb =
          *static_cast<CInstanceGame*>(CPrivateBase::m_interface->globalSingleInstance)
               ->m_instanceData->toKodi;
      return cb.GetStreamBuffer(cb.kodiInstance, m_handle, width, height, &buffer);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    /// @ingroup cpp_kodi_addon_game_Operation_CStream
    /// @brief Add a data packet to a stream
    ///
    /// @param[in] packet The data packet
    ///
    /// @remarks Only called from the add-on itself
    ///
    void AddData(const game_stream_packet& packet)
    {
      if (!m_handle || !CPrivateBase::m_interface->globalSingleInstance)
        return;

      AddonToKodiFuncTable_Game& cb =
          *static_cast<CInstanceGame*>(CPrivateBase::m_interface->globalSingleInstance)
               ->m_instanceData->toKodi;
      cb.AddStreamData(cb.kodiInstance, m_handle, &packet);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    /// @ingroup cpp_kodi_addon_game_Operation_CStream
    /// @brief Free an allocated buffer
    ///
    /// @param[in] buffer The buffer returned from GetStreamBuffer()
    ///
    /// @remarks Only called from the add-on itself
    ///
    void ReleaseBuffer(game_stream_buffer& buffer)
    {
      if (!m_handle || !CPrivateBase::m_interface->globalSingleInstance)
        return;

      AddonToKodiFuncTable_Game& cb =
          *static_cast<CInstanceGame*>(CPrivateBase::m_interface->globalSingleInstance)
               ->m_instanceData->toKodi;
      cb.ReleaseStreamBuffer(cb.kodiInstance, m_handle, &buffer);
    }
    //--------------------------------------------------------------------------

    //==========================================================================
    /// @ingroup cpp_kodi_addon_game_Operation_CStream
    /// @brief Check if the stream opened correctly, e.g. after calling the constructor
    ///
    /// @return True if stream was successfully opened, false otherwise
    ///
    /// @remarks Only called from the add-on itself
    ///
    bool IsOpen() const { return m_handle != nullptr; }
    //--------------------------------------------------------------------------

  private:
    KODI_GAME_STREAM_HANDLE m_handle = nullptr;
  };
  ///@}

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  ///
  /// @defgroup cpp_kodi_addon_game_HardwareRendering 3. Hardware rendering operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Hardware rendering operations**
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Hardware rendering operation parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_game_HardwareRendering_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_game_HardwareRendering_source_addon_auto_check
  ///
  ///@{

  //==========================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Enable hardware rendering functionality
  ///
  /// @return True if hardware rendering was enabled, false otherwise
  ///
  /// @remarks Only called from the add-on itself
  ///
  bool EnableHardwareRendering(const game_hw_rendering_properties& properties)
  {
    return m_instanceData->toKodi->EnableHardwareRendering(m_instanceData->toKodi->kodiInstance,
                                                           &properties);
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Invalidates the current HW context and reinitializes GPU resources
  ///
  /// Any GL state is lost, and must not be deinitialized explicitly.
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the HW context was reset
  ///
  virtual GAME_ERROR HwContextReset() { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Called before the context is destroyed
  ///
  /// Resources can be deinitialized at this step.
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the HW context was destroyed
  ///
  virtual GAME_ERROR HwContextDestroy() { return GAME_ERROR_NOT_IMPLEMENTED; }

  //============================================================================
  /// @brief **Callback to Kodi Function**<br>Get a symbol from the hardware context
  ///
  /// @param[in] sym The symbol's name
  ///
  /// @return A function pointer for the specified symbol
  ///
  /// @remarks Only called from the add-on itself
  ///
  game_proc_address_t HwGetProcAddress(const char* sym)
  {
    return m_instanceData->toKodi->HwGetProcAddress(m_instanceData->toKodi->kodiInstance, sym);
  }
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  ///
  /// @defgroup cpp_kodi_addon_game_Audio 4. Audio operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Audio operations**
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Audio operation parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_game_Audio_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_game_Audio_source_addon_auto_check
  ///
  ///@{

  //==========================================================================
  /// @brief Tell the client the frontend is ready for audio
  ///
  /// Only implemented by clients that asked for the asynchronous audio
  /// interface. Such a client produces no audio of its own accord: it waits to
  /// be asked, then writes what it has through AddStreamData() on the thread
  /// this was called from.
  ///
  /// @return The error, or GAME_ERROR_NO_ERROR if audio was handled
  ///
  virtual GAME_ERROR AudioAvailable() { return GAME_ERROR_NOT_IMPLEMENTED; }
  //--------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  /// @defgroup cpp_kodi_addon_game_InputOperations 5. Input operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Input operations**
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Hardware rendering operation parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_game_InputOperations_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_game_InputOperations_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Check if input is accepted for a feature on the controller
  ///
  /// If only a subset of the controller profile is used, this can return false
  /// for unsupported features to not absorb their input.
  ///
  /// If the entire controller profile is used, this should always return true.
  ///
  /// @param[in] controller_id The ID of the controller profile
  /// @param[in] feature_name The name of a feature in that profile
  /// @return True if input is accepted for the feature, false otherwise
  ///
  virtual bool HasFeature(const std::string& controller_id, const std::string& feature_name)
  {
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get the input topology that specifies which controllers can be connected
  ///
  /// @return The input topology, or null to use the default
  ///
  /// If this returns non-null, topology must be freed using FreeTopology().
  ///
  /// If this returns null, the topology will default to a single port that can
  /// accept all controllers imported by addon.xml. The port ID is set to
  /// the @ref DEFAULT_PORT_ID constant.
  ///
  virtual game_input_topology* GetTopology() { return nullptr; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Free the topology's resources
  ///
  /// @param[in] topology The topology returned by GetTopology()
  ///
  virtual void FreeTopology(game_input_topology* topology) {}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Set the layouts for known controllers
  ///
  /// @param[in] controllers The controller layouts
  ///
  /// After loading the input topology, the frontend will call this with
  /// controller layouts for all controllers discovered in the topology.
  ///
  virtual void SetControllerLayouts(
      const std::vector<kodi::addon::GameControllerLayout>& controllers)
  {
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Enable/disable keyboard input using the specified controller
  ///
  /// @param[in] enable True to enable input, false otherwise
  /// @param[in] controller_id The controller ID if enabling, or unused if disabling
  ///
  /// @return True if keyboard input was enabled, false otherwise
  ///
  virtual bool EnableKeyboard(bool enable, const std::string& controller_id) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Enable/disable mouse input using the specified controller
  ///
  /// @param[in] enable True to enable input, false otherwise
  /// @param[in] controller_id The controller ID if enabling, or unused if disabling
  ///
  /// @return True if mouse input was enabled, false otherwise
  ///
  virtual bool EnableMouse(bool enable, const std::string& controller_id) { return false; }
  //--------------------------------------------------------------------------

  //==========================================================================
  /// @brief Connect/disconnect a controller to a port on the virtual game console
  ///
  /// @param[in] connect True to connect a controller, false to disconnect
  /// @param[in] port_address The address of the port
  /// @param[in] controller_id The controller ID if connecting, or unused if disconnecting
  ///
  /// @return True if the \p controller_id was (dis-)connected to the port, false otherwise
  ///
  /// The address is a string that allows traversal of the controller topology.
  /// It is formed by alternating port IDs and controller IDs separated by "/".
  ///
  /// For example, assume that the topology represented in XML for Snes9x is:
  ///
  /// ~~~~~~~~~~~~~{.xml}
  ///     <logicaltopology>
  ///       <port type="controller" id="1">
  ///         <accepts controller="game.controller.snes"/>
  ///         <accepts controller="game.controller.snes.multitap">
  ///           <port type="controller" id="1">
  ///             <accepts controller="game.controller.snes"/>
  ///           </port>
  ///           <port type="controller" id="2">
  ///             <accepts controller="game.controller.snes"/>
  ///           </port>
  ///           ...
  ///         </accepts>
  ///       </port>
  ///     </logicaltopology>
  /// ~~~~~~~~~~~~~
  ///
  /// To connect a multitap to the console's first port, the multitap's controller
  /// info is set using the port address:
  ///
  ///     /1
  ///
  /// To connect a SNES controller to the second port of the multitap, the
  /// controller info is next set using the address:
  ///
  ///     /1/game.controller.multitap/2
  ///
  /// Any attempts to connect a controller to a port on a disconnected multitap
  /// will return false.
  ///
  virtual bool ConnectController(bool connect,
                                 const std::string& port_address,
                                 const std::string& controller_id)
  {
    return false;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Notify the add-on of an input event
  ///
  /// @param[in] event The input event
  ///
  /// @return True if the event was handled, false otherwise
  ///
  virtual bool InputEvent(const game_input_event& event) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Notify the port of an input event
  ///
  /// @param[in] event The input event
  /// @return True if the event was handled, false otherwise
  ///
  /// @note Input events can arrive for the following sources:
  ///   - @ref GAME_INPUT_EVENT_MOTOR
  ///
  /// @remarks Only called from the add-on itself
  ///
  bool KodiInputEvent(const game_input_event& event)
  {
    return m_instanceData->toKodi->InputEvent(m_instanceData->toKodi->kodiInstance, &event);
  }
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  /// @defgroup cpp_kodi_addon_game_SerializationOperations 6. Serialization operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Serialization operations**
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Serialization operation parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_game_SerializationOperations_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_game_SerializationOperations_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Get the number of bytes required to serialize the game
  ///
  /// @return The number of bytes, or 0 if serialization is not supported
  ///
  virtual size_t SerializeSize() { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Serialize the state of the game
  ///
  /// @param[in] data The buffer receiving the serialized game data
  /// @param[in] size The size of the buffer
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the game was serialized into the buffer
  ///
  virtual GAME_ERROR Serialize(uint8_t* data, size_t size) { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Deserialize the game from the given state
  ///
  /// @param[in] data A buffer containing the game's new state
  /// @param[in] size The size of the buffer
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the game deserialized
  ///
  virtual GAME_ERROR Deserialize(const uint8_t* data, size_t size)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief How many bytes the achievement runtime's state needs right now
  ///
  /// Asked each time rather than reserved once, so it grows with the runtime.
  ///
  /// @return The size, or 0 when there is nothing to save
  ///
  /// @note Added in Game API 8.0.0
  ///
  virtual size_t AchievementStateSize() { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Write the achievement runtime's state
  ///
  /// Kept beside the emulator's state, not inside it: a savestate whose
  /// emulator memory does not match what the core reports is refused before the
  /// core sees it.
  ///
  /// @note Added in Game API 8.0.0
  ///
  virtual GAME_ERROR SerializeAchievements(uint8_t* data, size_t size)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Restore achievement state written by SerializeAchievements()
  ///
  /// Called for every savestate load, including ones that carry no achievement
  /// state: a savestate written by a build without it, or while signed out.
  /// In that case @p data is nullptr and @p size is 0, and the runtime must be
  /// reset rather than left as it is. Emulator memory has jumped, so every
  /// hit count and prior value the runtime holds describes a moment that no
  /// longer follows from it, and leaving them could award an achievement the
  /// player did not earn.
  ///
  /// @param[in] data The state, or nullptr if the savestate carries none
  /// @param[in] size The size of @p data, or 0 if the savestate carries none
  ///
  /// @note Added in Game API 8.0.0
  ///
  virtual GAME_ERROR DeserializeAchievements(const uint8_t* data, size_t size)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  /// @defgroup cpp_kodi_addon_game_CheatOperations 7. Cheat operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Cheat operations**
  ///
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Cheat operation parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_game_CheatOperations_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_game_CheatOperations_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Reset the cheat system
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the cheat system was reset
  ///
  virtual GAME_ERROR CheatReset() { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get a region of memory
  ///
  /// @param[in] type The type of memory to retrieve
  /// @param[in] data Set to the region of memory; must remain valid until UnloadGame() is called
  /// @param[in] size Set to the size of the region of memory
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if data was set to a valid buffer
  ///
  virtual GAME_ERROR GetMemory(GAME_MEMORY type, uint8_t*& data, size_t& size)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Set a cheat code
  ///
  /// @param[in] index
  /// @param[in] enabled
  /// @param[in] code
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the cheat was set
  ///
  virtual GAME_ERROR SetCheat(unsigned int index, bool enabled, const std::string& code)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //============================================================================
  /// @brief Set the credentials of the RetroAchievements user
  ///
  /// @param[in] username The RetroAchievements username of the user
  /// @param[in] token The login token to RetroAchievements of the user
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the call was successful
  ///
  virtual GAME_ERROR SetRetroAchievementsCredentials(const std::string& username,
                                                     const std::string& token)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //============================================================================
  /// @brief Activate an achievement
  ///
  /// @param[in] cheevoId The achievement ID
  /// @param[in] memAddrExpression Achievement memory expression from patch data
  ///                              as a string
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the call was successful
  ///
  virtual GAME_ERROR ActivateAchievement(unsigned int cheevoId,
                                         const std::string& memAddrExpression)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }

  //============================================================================
  /// @brief Get triggered achievement URL and ID pairs
  ///
  /// @param[in] callback Callback invoked once per triggered achievement during
  ///                     this call. It may be called zero or more times before
  ///                     the function returns. Implementations must not
  ///                     retain/copy the callback for later use. The URL string
  ///                     reference is valid only for the callback invocation and
  ///                     must be copied if needed afterwards.
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the call was successful
  ///
  virtual GAME_ERROR GetCheevoUrlId(
      const std::function<void(const std::string& achievementUrl, unsigned int cheevoId)>& callback)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Notify Kodi that a game has been identified by the achievement runtime
  ///
  /// @param[in] data The achievement set of the game
  ///
  /// @remarks Only called from the add-on itself. The pointers inside @p data
  ///          need only stay valid for the duration of the call.
  ///
  /// @note Added in Game API 8.0.0
  ///
  void RCOnGameLoaded(const game_rc_game_loaded& data)
  {
    m_instanceData->toKodi->RCOnGameLoaded(m_instanceData->toKodi->kodiInstance, &data);
  }

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Notify Kodi that the player has earned an achievement
  ///
  /// @param[in] data The achievement that was earned
  ///
  /// @remarks Only called from the add-on itself
  ///
  /// @note Added in Game API 8.0.0
  ///
  void RCOnAchievementTriggered(const game_rc_achievement_triggered& data)
  {
    m_instanceData->toKodi->RCOnAchievementTriggered(m_instanceData->toKodi->kodiInstance, &data);
  }

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Notify Kodi that every achievement of the game has been earned
  ///
  /// @param[in] title The title of the completed game
  /// @param[in] hardcore True if the game was completed in hardcore mode
  ///
  /// @remarks Only called from the add-on itself
  ///
  /// @note Added in Game API 8.0.0
  ///
  void RCOnGameCompleted(const std::string& title, bool hardcore)
  {
    m_instanceData->toKodi->RCOnGameCompleted(m_instanceData->toKodi->kodiInstance, title.c_str(),
                                              hardcore);
  }

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Publish the current rich presence evaluation to Kodi
  ///
  /// @param[in] evaluation What the player is currently doing in the game
  ///
  /// @remarks Only called from the add-on itself
  ///
  /// @note Added in Game API 8.0.0
  ///
  void RCOnRichPresenceUpdated(const std::string& evaluation)
  {
    m_instanceData->toKodi->RCOnRichPresenceUpdated(m_instanceData->toKodi->kodiInstance,
                                                    evaluation.c_str());
  }

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Notify Kodi of the outcome of a login attempt
  ///
  /// @param[in] data The login result
  ///
  /// @remarks Only called from the add-on itself
  ///
  /// @note Added in Game API 8.0.0
  ///
  void RCOnLoginResult(const game_rc_login_result& data)
  {
    m_instanceData->toKodi->RCOnLoginResult(m_instanceData->toKodi->kodiInstance, &data);
  }

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Publish progress towards the game's measured achievements
  ///
  /// Sent whenever the reported progress changes. Achievements without a
  /// measured trigger condition are omitted.
  ///
  /// @param[in] progress Progress of each measured achievement
  ///
  /// @remarks Only called from the add-on itself
  ///
  /// @note Added in Game API 8.0.0
  ///
  void RCOnAchievementProgress(const std::vector<game_rc_achievement_progress>& progress)
  {
    m_instanceData->toKodi->RCOnAchievementProgress(m_instanceData->toKodi->kodiInstance,
                                                    progress.empty() ? nullptr : progress.data(),
                                                    static_cast<unsigned int>(progress.size()));
  }

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Show the achievement the player is working towards, or hide it
  ///
  /// @param[in] indicator The achievement to show, or `nullptr` to hide it
  ///
  /// @remarks Only called from the add-on itself
  ///
  /// @note Added in Game API 8.1.0
  ///
  void RCOnProgressIndicator(const game_rc_progress_indicator* indicator)
  {
    m_instanceData->toKodi->RCOnProgressIndicator(m_instanceData->toKodi->kodiInstance, indicator);
  }

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Show or hide the achievement the player is inside an attempt at
  ///
  /// @param[in] indicator The achievement being attempted, or `nullptr` to hide
  ///
  /// @remarks Only called from the add-on itself
  ///
  /// @note Added in Game API 8.1.0
  ///
  void RCOnChallengeIndicator(const game_rc_challenge_indicator* indicator)
  {
    m_instanceData->toKodi->RCOnChallengeIndicator(m_instanceData->toKodi->kodiInstance, indicator);
  }

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Report an error returned by the RetroAchievements server
  ///
  /// @param[in] message The error as reported by the server
  /// @param[in] api The API call that failed, or an empty string
  ///
  /// @remarks Only called from the add-on itself
  ///
  /// @note Added in Game API 8.0.0
  ///
  void RCOnServerError(const std::string& message, const std::string& api)
  {
    m_instanceData->toKodi->RCOnServerError(m_instanceData->toKodi->kodiInstance, message.c_str(),
                                            api.c_str());
  }

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Report a change in connectivity to the RetroAchievements server
  ///
  /// Unlocks earned while disconnected are held by the add-on and submitted
  /// once the connection returns.
  ///
  /// @param[in] connected True if the server is reachable again
  ///
  /// @remarks Only called from the add-on itself
  ///
  /// @note Added in Game API 8.0.0
  ///
  void RCOnConnectionChanged(bool connected)
  {
    m_instanceData->toKodi->RCOnConnectionChanged(m_instanceData->toKodi->kodiInstance, connected);
  }

  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  /// @defgroup cpp_kodi_addon_game_DiscOperations 8. Disc operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Disc operations**
  ///
  ///---------------------------------------------------------------------------
  ///
  /// **Disc operation parts in interface:**\n
  /// Copy this to your project and extend with your parts or leave functions
  /// complete away where not used or supported.
  ///
  /// @copydetails cpp_kodi_addon_game_DiscOperations_header_addon_auto_check
  /// @copydetails cpp_kodi_addon_game_DiscOperations_source_addon_auto_check
  ///
  ///@{

  //============================================================================
  /// @brief Returns whether the virtual disk tray is currently ejected.
  ///
  /// The initial state should be closed (`false`) unless changed by the game
  /// implementation.
  ///
  /// @return `true` if the tray is ejected (open), otherwise `false`.
  ///
  virtual bool GetEjectState() { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Opens or closes the virtual disk tray.
  ///
  /// The image index should only be changed while the tray is ejected.
  ///
  /// @param[in] ejected `true` to eject/open the tray, `false` to close it.
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the tray state was
  ///         changed successfully.
  ///
  virtual GAME_ERROR SetEjectState(bool ejected) { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Gets the index of the currently inserted disk image.
  ///
  /// @return Current disk image index. A value greater than or equal to
  ///         @ref GetImageCount() indicates that no image is inserted.
  ///
  virtual unsigned int GetImageIndex() { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Inserts the disk image at the given index.
  ///
  /// This should only succeed when the tray is ejected.
  ///
  /// @param[in] imageIndex The image index to insert.
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the image was set.
  ///
  virtual GAME_ERROR SetImageIndex(unsigned int imageIndex) { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Gets the number of available disk images.
  ///
  /// @return The total number of selectable disk images.
  ///
  virtual unsigned int GetImageCount() { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Adds a new disk image slot.
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if a new image index was
  ///         added.
  ///
  virtual GAME_ERROR AddImageIndex() { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Replaces the disk image at the given index.
  ///
  /// The tray must be ejected for this operation.
  ///
  /// @param[in] imageIndex The image index to replace.
  /// @param[in] filePath Path to the new disk image.
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the image was replaced.
  ///
  virtual GAME_ERROR ReplaceImageIndex(unsigned int imageIndex, const std::string& filePath)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Removes the disk image at the given index.
  ///
  /// The tray must be ejected for this operation.
  ///
  /// @param[in] imageIndex The image index to remove.
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the image was removed.
  ///
  virtual GAME_ERROR RemoveImageIndex(unsigned int imageIndex)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Sets which image should be initially inserted on load.
  ///
  /// @param[in] imageIndex The initial image index.
  /// @param[in] filePath Path used to validate the selected image.
  ///
  /// @return The error, or @ref GAME_ERROR_NO_ERROR if the initial image was
  ///         accepted.
  ///
  virtual GAME_ERROR SetInitialImage(unsigned int imageIndex, const std::string& filePath)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Gets the full path of a disk image.
  ///
  /// @param[in] imageIndex The image index to query.
  ///
  /// @return The image path, or an empty string if unavailable.
  ///
  virtual std::string GetImagePath(unsigned int imageIndex) { return ""; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Gets a user-friendly label for a disk image.
  ///
  /// @param[in] imageIndex The image index to query.
  ///
  /// @return The image label, or an empty string if unavailable.
  ///
  virtual std::string GetImageLabel(unsigned int imageIndex) { return ""; }
  //----------------------------------------------------------------------------

  ///@}

private:
  void SetAddonStruct(KODI_ADDON_INSTANCE_STRUCT* instance)
  {
    instance->hdl = this;

    instance->game->toAddon->LoadGame = ADDON_LoadGame;
    instance->game->toAddon->LoadGameSpecial = ADDON_LoadGameSpecial;
    instance->game->toAddon->LoadStandalone = ADDON_LoadStandalone;
    instance->game->toAddon->UnloadGame = ADDON_UnloadGame;
    instance->game->toAddon->GetGameTiming = ADDON_GetGameTiming;
    instance->game->toAddon->GetRegion = ADDON_GetRegion;
    instance->game->toAddon->RequiresGameLoop = ADDON_RequiresGameLoop;
    instance->game->toAddon->RunFrame = ADDON_RunFrame;
    instance->game->toAddon->Reset = ADDON_Reset;

    instance->game->toAddon->HwContextReset = ADDON_HwContextReset;
    instance->game->toAddon->HwContextDestroy = ADDON_HwContextDestroy;

    instance->game->toAddon->AudioAvailable = ADDON_AudioAvailable;

    instance->game->toAddon->HasFeature = ADDON_HasFeature;
    instance->game->toAddon->GetTopology = ADDON_GetTopology;
    instance->game->toAddon->FreeTopology = ADDON_FreeTopology;
    instance->game->toAddon->SetControllerLayouts = ADDON_SetControllerLayouts;
    instance->game->toAddon->EnableKeyboard = ADDON_EnableKeyboard;
    instance->game->toAddon->EnableMouse = ADDON_EnableMouse;
    instance->game->toAddon->ConnectController = ADDON_ConnectController;
    instance->game->toAddon->InputEvent = ADDON_InputEvent;

    instance->game->toAddon->SerializeSize = ADDON_SerializeSize;
    instance->game->toAddon->Serialize = ADDON_Serialize;
    instance->game->toAddon->Deserialize = ADDON_Deserialize;
    instance->game->toAddon->AchievementStateSize = ADDON_AchievementStateSize;
    instance->game->toAddon->SerializeAchievements = ADDON_SerializeAchievements;
    instance->game->toAddon->DeserializeAchievements = ADDON_DeserializeAchievements;

    instance->game->toAddon->CheatReset = ADDON_CheatReset;
    instance->game->toAddon->GetMemory = ADDON_GetMemory;
    instance->game->toAddon->SetCheat = ADDON_SetCheat;

    instance->game->toAddon->SetRetroAchievementsCredentials =
        ADDON_SetRetroAchievementsCredentials;
    instance->game->toAddon->ActivateAchievement = ADDON_ActivateAchievement;
    instance->game->toAddon->GetCheevoUrlId = ADDON_GetCheevoUrlId;

    instance->game->toAddon->GetEjectState = ADDON_GetEjectState;
    instance->game->toAddon->SetEjectState = ADDON_SetEjectState;
    instance->game->toAddon->GetImageIndex = ADDON_GetImageIndex;
    instance->game->toAddon->SetImageIndex = ADDON_SetImageIndex;
    instance->game->toAddon->GetImageCount = ADDON_GetImageCount;
    instance->game->toAddon->AddImageIndex = ADDON_AddImageIndex;
    instance->game->toAddon->ReplaceImageIndex = ADDON_ReplaceImageIndex;
    instance->game->toAddon->RemoveImageIndex = ADDON_RemoveImageIndex;
    instance->game->toAddon->SetInitialImage = ADDON_SetInitialImage;
    instance->game->toAddon->GetImagePath = ADDON_GetImagePath;
    instance->game->toAddon->GetImageLabel = ADDON_GetImageLabel;

    instance->game->toAddon->FreeString = ADDON_FreeString;

    m_instanceData = instance->game;
    m_instanceData->toAddon->addonInstance = this;
  }

  // --- Game operations ---------------------------------------------------------

  inline static GAME_ERROR ADDON_LoadGame(const AddonInstance_Game* instance, const char* url)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->LoadGame(url);
  }

  inline static GAME_ERROR ADDON_LoadGameSpecial(const AddonInstance_Game* instance,
                                                 SPECIAL_GAME_TYPE type,
                                                 const char** urls,
                                                 size_t urlCount)
  {
    std::vector<std::string> urlList;
    for (size_t i = 0; i < urlCount; ++i)
    {
      if (urls[i] != nullptr)
        urlList.emplace_back(urls[i]);
    }

    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->LoadGameSpecial(type, urlList);
  }

  inline static GAME_ERROR ADDON_LoadStandalone(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->LoadStandalone();
  }

  inline static GAME_ERROR ADDON_UnloadGame(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->UnloadGame();
  }

  inline static GAME_ERROR ADDON_GetGameTiming(const AddonInstance_Game* instance,
                                               game_system_timing* timing_info)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->GetGameTiming(*timing_info);
  }

  inline static GAME_REGION ADDON_GetRegion(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->GetRegion();
  }

  inline static bool ADDON_RequiresGameLoop(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->RequiresGameLoop();
  }

  inline static GAME_ERROR ADDON_RunFrame(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->RunFrame();
  }

  inline static GAME_ERROR ADDON_Reset(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->Reset();
  }

  // --- Hardware rendering operations -------------------------------------------

  inline static GAME_ERROR ADDON_HwContextReset(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->HwContextReset();
  }

  inline static GAME_ERROR ADDON_HwContextDestroy(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->HwContextDestroy();
  }

  // --- Audio operations --------------------------------------------------------

  inline static GAME_ERROR ADDON_AudioAvailable(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->AudioAvailable();
  }

  // --- Input operations --------------------------------------------------------

  inline static bool ADDON_HasFeature(const AddonInstance_Game* instance,
                                      const char* controller_id,
                                      const char* feature_name)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->HasFeature(controller_id, feature_name);
  }

  inline static game_input_topology* ADDON_GetTopology(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->GetTopology();
  }

  inline static void ADDON_FreeTopology(const AddonInstance_Game* instance,
                                        game_input_topology* topology)
  {
    static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->FreeTopology(topology);
  }

  inline static void ADDON_SetControllerLayouts(const AddonInstance_Game* instance,
                                                const game_controller_layout* controllers,
                                                unsigned int controller_count)
  {
    if (controllers == nullptr)
      return;

    std::vector<GameControllerLayout> controllerList;
    controllerList.reserve(controller_count);
    for (unsigned int i = 0; i < controller_count; ++i)
      controllerList.emplace_back(controllers[i]);

    static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->SetControllerLayouts(controllerList);
  }

  inline static bool ADDON_EnableKeyboard(const AddonInstance_Game* instance,
                                          bool enable,
                                          const char* controller_id)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->EnableKeyboard(enable, controller_id);
  }

  inline static bool ADDON_EnableMouse(const AddonInstance_Game* instance,
                                       bool enable,
                                       const char* controller_id)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->EnableMouse(enable, controller_id);
  }

  inline static bool ADDON_ConnectController(const AddonInstance_Game* instance,
                                             bool connect,
                                             const char* port_address,
                                             const char* controller_id)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->ConnectController(connect, port_address, controller_id);
  }

  inline static bool ADDON_InputEvent(const AddonInstance_Game* instance,
                                      const game_input_event* event)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->InputEvent(*event);
  }

  // --- Serialization operations ------------------------------------------------

  inline static size_t ADDON_SerializeSize(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->SerializeSize();
  }

  inline static GAME_ERROR ADDON_Serialize(const AddonInstance_Game* instance,
                                           uint8_t* data,
                                           size_t size)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->Serialize(data, size);
  }

  inline static GAME_ERROR ADDON_Deserialize(const AddonInstance_Game* instance,
                                             const uint8_t* data,
                                             size_t size)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->Deserialize(data, size);
  }

  inline static size_t ADDON_AchievementStateSize(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->AchievementStateSize();
  }

  inline static GAME_ERROR ADDON_SerializeAchievements(const AddonInstance_Game* instance,
                                                       uint8_t* data,
                                                       size_t size)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->SerializeAchievements(data, size);
  }

  inline static GAME_ERROR ADDON_DeserializeAchievements(const AddonInstance_Game* instance,
                                                         const uint8_t* data,
                                                         size_t size)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->DeserializeAchievements(data, size);
  }

  // --- Cheat operations --------------------------------------------------------

  inline static GAME_ERROR ADDON_CheatReset(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->CheatReset();
  }

  inline static GAME_ERROR ADDON_GetMemory(const AddonInstance_Game* instance,
                                           GAME_MEMORY type,
                                           uint8_t** data,
                                           size_t* size)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->GetMemory(type, *data, *size);
  }

  inline static GAME_ERROR ADDON_SetCheat(const AddonInstance_Game* instance,
                                          unsigned int index,
                                          bool enabled,
                                          const char* code)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->SetCheat(index, enabled, code);
  }

  inline static GAME_ERROR ADDON_SetRetroAchievementsCredentials(const AddonInstance_Game* instance,
                                                                 const char* username,
                                                                 const char* token)
  {
    if (username == nullptr || token == nullptr)
      return GAME_ERROR_INVALID_PARAMETERS;

    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->SetRetroAchievementsCredentials(username, token);
  }

  inline static GAME_ERROR ADDON_ActivateAchievement(const AddonInstance_Game* instance,
                                                     unsigned int cheevoId,
                                                     const char* memAddrExpression)
  {
    if (memAddrExpression == nullptr)
      return GAME_ERROR_INVALID_PARAMETERS;

    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->ActivateAchievement(cheevoId, memAddrExpression);
  }

  inline static GAME_ERROR ADDON_GetCheevoUrlId(const AddonInstance_Game* instance,
                                                void(__cdecl* callback)(const void* context,
                                                                        const char* achievementUrl,
                                                                        unsigned int cheevoId),
                                                const void* context)
  {
    if (callback == nullptr)
      return GAME_ERROR_INVALID_PARAMETERS;

    const auto cppCallback =
        [callback, context](const std::string& achievementUrl, unsigned int cheevoId)
    { callback(context, achievementUrl.c_str(), cheevoId); };

    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->GetCheevoUrlId(cppCallback);
  }

  inline static bool ADDON_GetEjectState(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->GetEjectState();
  }

  inline static GAME_ERROR ADDON_SetEjectState(const AddonInstance_Game* instance, bool ejected)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->SetEjectState(ejected);
  }

  inline static unsigned int ADDON_GetImageIndex(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->GetImageIndex();
  }

  inline static GAME_ERROR ADDON_SetImageIndex(const AddonInstance_Game* instance,
                                               unsigned int imageIndex)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->SetImageIndex(imageIndex);
  }

  inline static unsigned int ADDON_GetImageCount(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->GetImageCount();
  }

  inline static GAME_ERROR ADDON_AddImageIndex(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->AddImageIndex();
  }

  inline static GAME_ERROR ADDON_ReplaceImageIndex(const AddonInstance_Game* instance,
                                                   unsigned int imageIndex,
                                                   const char* filePath)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->ReplaceImageIndex(imageIndex, filePath ? filePath : "");
  }

  inline static GAME_ERROR ADDON_RemoveImageIndex(const AddonInstance_Game* instance,
                                                  unsigned int imageIndex)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->RemoveImageIndex(imageIndex);
  }

  inline static GAME_ERROR ADDON_SetInitialImage(const AddonInstance_Game* instance,
                                                 unsigned int imageIndex,
                                                 const char* filePath)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->SetInitialImage(imageIndex, filePath ? filePath : "");
  }

  inline static char* ADDON_GetImagePath(const AddonInstance_Game* instance,
                                         unsigned int imageIndex)
  {
    std::string cppPath =
        static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->GetImagePath(imageIndex);
    if (cppPath.empty())
      return nullptr;

    char* path = new char[cppPath.size() + 1];
    std::copy(cppPath.begin(), cppPath.end(), path);
    path[cppPath.size()] = '\0';
    return path;
  }

  inline static char* ADDON_GetImageLabel(const AddonInstance_Game* instance,
                                          unsigned int imageIndex)
  {
    std::string cppLabel =
        static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->GetImageLabel(imageIndex);
    if (cppLabel.empty())
      return nullptr;

    char* label = new char[cppLabel.size() + 1];
    std::copy(cppLabel.begin(), cppLabel.end(), label);
    label[cppLabel.size()] = '\0';
    return label;
  }

  inline static void ADDON_FreeString(const AddonInstance_Game* instance, char* str)
  {
    delete[] str;
  }

  AddonInstance_Game* m_instanceData;
};

} /* namespace addon */
} /* namespace kodi */

#endif /* __cplusplus */
