/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "../AddonBase.h"
#include "../c-api/addon-instance/game.h"

#include <algorithm>

#ifdef __cplusplus

namespace kodi
{
namespace addon
{

//==============================================================================
/// @addtogroup cpp_kodi_addon_game
///
/// To use on Libretro and for stand-alone games or emulators that does not use
/// the Libretro API.
///
/// Possible examples could be, Nvidia GameStream via Limelight or WINE capture
/// could possible through the Game API.
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

  /// @brief Controller identifier.
  std::string controller_id;

  /// @brief Provides input.
  ///
  /// False for multitaps
  bool provides_input{false};

  /// @brief Digital buttons.
  std::vector<std::string> digital_buttons;

  /// @brief Analog buttons.
  std::vector<std::string> analog_buttons;

  /// @brief Analog sticks.
  std::vector<std::string> analog_sticks;

  /// @brief Accelerometers.
  std::vector<std::string> accelerometers;

  /// @brief Keys.
  std::vector<std::string> keys;

  /// @brief Relative pointers.
  std::vector<std::string> rel_pointers;

  /// @brief Absolute pointers.
  std::vector<std::string> abs_pointers;

  /// @brief Motors.
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
/// This class is created at addon by Kodi.
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
  /// Used by an add-on that only supports only Game and only in one instance.
  ///
  /// This class is created at addon by Kodi.
  ///
  ///
  /// --------------------------------------------------------------------------
  ///
  ///
  /// **Here's example about the use of this:**
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
      throw std::logic_error("kodi::addon::CInstanceGame: Creation of more as one in single "
                             "instance way is not allowed!");

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
  /// @return the used game client Dll path
  ///
  /// @remarks Only called from addon itself
  ///
  std::string GameClientDllPath() const { return m_instanceData->props->game_client_dll_path; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Paths to proxy DLLs used to load the game client.
  ///
  /// @param[out] paths vector list to store available dll paths
  /// @return true if success and dll paths present
  ///
  /// @remarks Only called from addon itself
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
  /// @return the used resource directory
  ///
  /// @remarks Only called from addon itself
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
  /// @return the used profile directory
  ///
  /// @remarks Only called from addon itself
  ///
  std::string ProfileDirectory() const { return m_instanceData->props->profile_directory; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// The value of the <supports_vfs> property from addon.xml.
  ///
  /// @return true if VFS is supported
  ///
  /// @remarks Only called from addon itself
  ///
  bool SupportsVFS() const { return m_instanceData->props->supports_vfs; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**\n
  /// The extensions in the <extensions> property from addon.xml.
  ///
  /// @param[out] extensions vector list to store available extension
  /// @return true if success and extensions present
  ///
  /// @remarks Only called from addon itself
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
  /// These are mandatory functions for using this addon to get the available
  /// channels.
  ///
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
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game was loaded
  ///
  virtual GAME_ERROR LoadGame(const std::string& url) { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Load a game that requires multiple files
  ///
  /// @param[in] type The game type
  /// @param[in] urls An array of urls
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game was loaded
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
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game add-on was loaded
  ///
  virtual GAME_ERROR LoadStandalone() { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Unload the current game
  ///
  /// Unloads a currently loaded game
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game was unloaded
  ///
  virtual GAME_ERROR UnloadGame() { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get timing information about the loaded game
  ///
  /// @param[out] timing_info The info structure to fill
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if info was filled
  ///
  virtual GAME_ERROR GetGameTiming(game_system_timing& timing_info)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Get region of the loaded game
  ///
  /// @return the region, or @ref GAME_REGION_UNKNOWN if unknown or no game is loaded
  ///
  virtual GAME_REGION GetRegion() { return GAME_REGION_UNKNOWN; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Return true if the client requires the frontend to provide a game loop
  ///
  /// The game loop is a thread that calls RunFrame() in a loop at a rate
  /// determined by the playback speed and the client's FPS.
  ///
  /// @return true if the frontend should provide a game loop, false otherwise
  ///
  virtual bool RequiresGameLoop() { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Run a single frame for add-ons that use a game loop
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if there was no error
  ///
  virtual GAME_ERROR RunFrame() { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Reset the current game
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game was reset
  ///
  virtual GAME_ERROR Reset() { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //==========================================================================
  /// @brief **Callback to Kodi Function**\n
  /// Requests the frontend to stop the current game
  ///
  /// @remarks Only called from addon itself
  ///
  void CloseGame(void) { m_instanceData->toKodi->CloseGame(m_instanceData->toKodi->kodiInstance); }
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
    /// @return A stream handle, or `nullptr` on failure
    ///
    /// @remarks Only called from addon itself
    ///
    bool Open(const game_stream_properties& properties)
    {
      if (!CPrivateBase::m_interface->globalSingleInstance)
        return false;

      if (m_handle)
      {
        kodi::Log(ADDON_LOG_INFO, "kodi::addon::CInstanceGame::CStream already becomes reopened");
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
    /// @remarks Only called from addon itself
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
    /// @return True if buffer was set, false otherwise
    ///
    /// @note If this returns true, buffer must be freed using @ref ReleaseBuffer().
    ///
    /// @remarks Only called from addon itself
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
    /// @remarks Only called from addon itself
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
    /// @remarks Only called from addon itself
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
    /// @brief To check stream open was OK, e.g. after use of constructor
    ///
    /// @return true if stream was successfully opened
    ///
    /// @remarks Only called from addon itself
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

  //============================================================================
  /// @brief Invalidates the current HW context and reinitializes GPU resources
  ///
  /// Any GL state is lost, and must not be deinitialized explicitly.
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the HW context was reset
  ///
  virtual GAME_ERROR HwContextReset() { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Called before the context is destroyed
  ///
  /// Resources can be deinitialized at this step.
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the HW context was destroyed
  ///
  virtual GAME_ERROR HwContextDestroy() { return GAME_ERROR_NOT_IMPLEMENTED; }

  //============================================================================
  /// @brief **Callback to Kodi Function**<br>Get a symbol from the hardware context
  ///
  /// @param[in] sym The symbol's name
  ///
  /// @return A function pointer for the specified symbol
  ///
  /// @remarks Only called from addon itself
  ///
  game_proc_address_t HwGetProcAddress(const char* sym)
  {
    return m_instanceData->toKodi->HwGetProcAddress(m_instanceData->toKodi->kodiInstance, sym);
  }
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  /// @defgroup cpp_kodi_addon_game_InputOperations 4. Input operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Input operations**
  ///
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
  /// @return true if input is accepted for the feature, false otherwise
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
  /// @return True if the \p controller was (dis-)connected to the port, false otherwise
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
  /// @return true if the event was handled, false otherwise
  ///
  virtual bool InputEvent(const game_input_event& event) { return false; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief **Callback to Kodi Function**<br>Notify the port of an input event
  ///
  /// @param[in] event The input event
  /// @return true if the event was handled, false otherwise
  ///
  /// @note Input events can arrive for the following sources:
  ///   - @ref GAME_INPUT_EVENT_MOTOR
  ///
  /// @remarks Only called from addon itself
  ///
  bool KodiInputEvent(const game_input_event& event)
  {
    return m_instanceData->toKodi->InputEvent(m_instanceData->toKodi->kodiInstance, &event);
  }
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  /// @defgroup cpp_kodi_addon_game_SerializationOperations 5. Serialization operations
  /// @ingroup cpp_kodi_addon_game
  /// @brief **Serialization operations**
  ///
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
  /// @return the number of bytes, or 0 if serialization is not supported
  ///
  virtual size_t SerializeSize() { return 0; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Serialize the state of the game
  ///
  /// @param[in] data The buffer receiving the serialized game data
  /// @param[in] size The size of the buffer
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game was serialized into the buffer
  ///
  virtual GAME_ERROR Serialize(uint8_t* data, size_t size) { return GAME_ERROR_NOT_IMPLEMENTED; }
  //----------------------------------------------------------------------------

  //============================================================================
  /// @brief Deserialize the game from the given state
  ///
  /// @param[in] data A buffer containing the game's new state
  /// @param[in] size The size of the buffer
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the game deserialized
  ///
  virtual GAME_ERROR Deserialize(const uint8_t* data, size_t size)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }
  //----------------------------------------------------------------------------

  ///@}

  //--==----==----==----==----==----==----==----==----==----==----==----==----==--

  //============================================================================
  /// @defgroup cpp_kodi_addon_game_CheatOperations 6. Cheat operations
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
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the cheat system was reset
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
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if data was set to a valid buffer
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
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the cheat was set
  ///
  virtual GAME_ERROR SetCheat(unsigned int index, bool enabled, const std::string& code)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }

  //============================================================================
  /// @brief Generates a RetroAchievements hash for a given game that
  ///        can be used to identify the game by RetroAchievements
  ///
  /// @param[out] hash The hash of the file. Its size must be >=33 characters
  /// @param[in] consoleID The console ID as it is defined by rcheevos for
  ///                      the console the ROM is made for
  /// @param[in] filePath The path of the rom
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the hash was generated
  ///         successfully
  ///
  virtual GAME_ERROR RCGenerateHashFromFile(std::string& hash,
                                            unsigned int consoleID,
                                            const std::string& filePath)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }

  //============================================================================
  /// @brief Gets a URL to the endpoint that returns the game ID
  ///
  /// @param[out] url The URL to GET the game ID
  /// @param[in] size The size of the URL char array
  /// @param[in] hash The hash of the rom
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the URL was created
  ///
  virtual GAME_ERROR RCGetGameIDUrl(std::string& url, const std::string& hash)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }

  //============================================================================
  /// @brief Gets a URL to the endpoint that returns the patch file
  ///
  /// @param[out] url The URL to GET the game patch file
  /// @param[in] size The size of the URL char array
  /// @param[in] username The RetroAchievements username of the user
  /// @param[in] token The login token to RetroAchievements of the user
  /// @param[in] gameID The ID of the game in RetroAchievements API
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the URL was created
  ///
  virtual GAME_ERROR RCGetPatchFileUrl(std::string& url,
                                       const std::string& username,
                                       const std::string& token,
                                       unsigned int gameID)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }

  //============================================================================
  /// @brief Gets a URL to the endpoint that updates the rich presence
  ///        in the user's RetroAchievements profile
  ///
  /// @param[out] url The URL to POST the rich presence to RetroAchievements
  /// @param[in] urlSize The size of the URL char array
  /// @param[out] postData The post data of the request
  /// @param[in] postSize The size of the post data char array
  /// @param[in] username The RetroAchievements username of the user
  /// @param[in] token The login token to RetroAchievements of the user
  /// @param[in] gameID The ID of the game in RetroAchievements API
  /// @param[in] richPresence The rich presence evaluation to POST
  ///
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the URL and post data
  ///         were created
  ///
  virtual GAME_ERROR RCPostRichPresenceUrl(std::string& url,
                                           std::string& postData,
                                           const std::string& username,
                                           const std::string& token,
                                           unsigned int gameID,
                                           const std::string& richPresence)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }

  //============================================================================
  /// @brief Enables rich presence
  ///
  /// @param[in] script The rich presence script from RetroAchievements
  ///
  /// @return the error, or GAME_ERROR_NO_ERROR if rich presence was enabled
  ///
  virtual GAME_ERROR RCEnableRichPresence(const std::string& script)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }

  //============================================================================
  /// @brief Gets the rich presence evaluation for the current frame.
  ///        Rich presence must be enabled first or this will fail.
  ///
  /// @param[out] evaluation The evaluation of what the player is doing in
  ///                        the game this frame
  /// @param[in] size The size of the evaluation char pointer
  /// @param[in] consoleID The console ID as it is defined by rcheevos for
  ///                      the console the rom is made for
  /// @return the error, or @ref GAME_ERROR_NO_ERROR if the evaluation was
  ///         created successfully
  ///
  virtual GAME_ERROR RCGetRichPresenceEvaluation(std::string& evaluation, unsigned int consoleID)
  {
    return GAME_ERROR_NOT_IMPLEMENTED;
  }

  //============================================================================
  /// @brief Resets the runtime. Must be called each time a new rom is starting
  ///        and when the savestate is changed
  ///
  /// @return the error, or GAME_ERROR_NO_ERROR if the runtim was reseted
  ///         successfully
  ///
  virtual GAME_ERROR RCResetRuntime() { return GAME_ERROR_NOT_IMPLEMENTED; }

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

    instance->game->toAddon->CheatReset = ADDON_CheatReset;
    instance->game->toAddon->GetMemory = ADDON_GetMemory;
    instance->game->toAddon->SetCheat = ADDON_SetCheat;

    instance->game->toAddon->RCGenerateHashFromFile = ADDON_RCGenerateHashFromFile;
    instance->game->toAddon->RCGetGameIDUrl = ADDON_RCGetGameIDUrl;
    instance->game->toAddon->RCGetPatchFileUrl = ADDON_RCGetPatchFileUrl;
    instance->game->toAddon->RCPostRichPresenceUrl = ADDON_RCPostRichPresenceUrl;
    instance->game->toAddon->RCEnableRichPresence = ADDON_RCEnableRichPresence;
    instance->game->toAddon->RCGetRichPresenceEvaluation = ADDON_RCGetRichPresenceEvaluation;
    instance->game->toAddon->RCResetRuntime = ADDON_RCResetRuntime;

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

  inline static GAME_ERROR ADDON_RCGenerateHashFromFile(const AddonInstance_Game* instance,
                                                        char** hash,
                                                        unsigned int consoleID,
                                                        const char* filePath)
  {
    std::string cppHash;

    GAME_ERROR ret = static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
                         ->RCGenerateHashFromFile(cppHash, consoleID, filePath);
    if (!cppHash.empty() && hash)
    {
      *hash = new char[cppHash.size() + 1];
      std::copy(cppHash.begin(), cppHash.end(), *hash);
      (*hash)[cppHash.size()] = '\0';
    }
    return ret;
  }

  inline static GAME_ERROR ADDON_RCGetGameIDUrl(const AddonInstance_Game* instance,
                                                char** url,
                                                const char* hash)
  {
    std::string cppUrl;
    GAME_ERROR ret =
        static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->RCGetGameIDUrl(cppUrl, hash);
    if (!cppUrl.empty() && url)
    {
      *url = new char[cppUrl.size() + 1];
      std::copy(cppUrl.begin(), cppUrl.end(), *url);
      (*url)[cppUrl.size()] = '\0';
    }
    return ret;
  }

  inline static GAME_ERROR ADDON_RCGetPatchFileUrl(const AddonInstance_Game* instance,
                                                   char** url,
                                                   const char* username,
                                                   const char* token,
                                                   unsigned int gameID)
  {
    std::string cppUrl;

    GAME_ERROR ret = static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
                         ->RCGetPatchFileUrl(cppUrl, username, token, gameID);
    if (!cppUrl.empty() && url)
    {
      *url = new char[cppUrl.size() + 1];
      std::copy(cppUrl.begin(), cppUrl.end(), *url);
      (*url)[cppUrl.size()] = '\0';
    }
    return ret;
  }

  inline static GAME_ERROR ADDON_RCPostRichPresenceUrl(const AddonInstance_Game* instance,
                                                       char** url,
                                                       char** postData,
                                                       const char* username,
                                                       const char* token,
                                                       unsigned int gameID,
                                                       const char* richPresence)
  {
    std::string cppUrl;
    std::string cppPostData;
    GAME_ERROR ret =
        static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
            ->RCPostRichPresenceUrl(cppUrl, cppPostData, username, token, gameID, richPresence);
    if (!cppUrl.empty())
    {
      *url = new char[cppUrl.size() + 1];
      std::copy(cppUrl.begin(), cppUrl.end(), *url);
      (*url)[cppUrl.size()] = '\0';
    }
    if (!cppPostData.empty())
    {
      *postData = new char[cppPostData.size() + 1];
      std::copy(cppPostData.begin(), cppPostData.end(), *postData);
      (*postData)[cppPostData.size()] = '\0';
    }

    return ret;
  }

  inline static GAME_ERROR ADDON_RCEnableRichPresence(const AddonInstance_Game* instance,
                                                      const char* script)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
        ->RCEnableRichPresence(script);
  }

  inline static GAME_ERROR ADDON_RCGetRichPresenceEvaluation(const AddonInstance_Game* instance,
                                                             char** evaluation,
                                                             unsigned int consoleID)
  {
    std::string cppEvaluation;
    GAME_ERROR ret = static_cast<CInstanceGame*>(instance->toAddon->addonInstance)
                         ->RCGetRichPresenceEvaluation(cppEvaluation, consoleID);
    if (!cppEvaluation.empty())
    {
      *evaluation = new char[cppEvaluation.size() + 1];
      std::copy(cppEvaluation.begin(), cppEvaluation.end(), *evaluation);
      (*evaluation)[cppEvaluation.size()] = '\0';
    }

    return ret;
  }

  inline static GAME_ERROR ADDON_RCResetRuntime(const AddonInstance_Game* instance)
  {
    return static_cast<CInstanceGame*>(instance->toAddon->addonInstance)->RCResetRuntime();
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
