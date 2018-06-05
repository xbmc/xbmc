/*
 *      Copyright (C) 2014-2017 Team Kodi
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

#include "kodi_game_types.h"

#ifdef __cplusplus
extern "C" {
#endif

// --- Game operations ---------------------------------------------------------

/*!
 * \brief Load a game
 *
 * \param url The URL to load
 *
 * return the error, or GAME_ERROR_NO_ERROR if the game was loaded
 */
GAME_ERROR LoadGame(const char* url);

/*!
 * \brief Load a game that requires multiple files
 *
 * \param type The game type
 * \param urls An array of urls
 * \param urlCount The number of urls in the array
 *
 * \return the error, or GAME_ERROR_NO_ERROR if the game was loaded
 */
GAME_ERROR LoadGameSpecial(SPECIAL_GAME_TYPE type, const char** urls, size_t urlCount);

/*!
 * \brief Begin playing without a game file
 *
 * If the add-on supports standalone mode, it must add the <supports_standalone>
 * tag to the extension point in addon.xml:
 *
 *     <supports_no_game>false</supports_no_game>
 *
 * \return the error, or GAME_ERROR_NO_ERROR if the game add-on was loaded
 */
GAME_ERROR LoadStandalone(void);

/*!
 * \brief Unload the current game
 *
 * \return the error, or GAME_ERROR_NO_ERROR if the game was unloaded
 */
/*! Unloads a currently loaded game */
GAME_ERROR UnloadGame(void);

/*!
 * \brief Get timing information about the loaded game
 *
 * \param info The info structure to fill
 *
 * \return the error, or GAME_ERROR_NO_ERROR if info was filled
 */
GAME_ERROR GetGameTiming(game_system_timing* timing_info);

/*!
 * \brief Get region of the loaded game
 *
 * \return the region, or GAME_REGION_UNKNOWN if unknown or no game is loaded
 */
GAME_REGION GetRegion(void);

/*!
 * \brief Return true if the client requires the frontend to provide a game loop
 *
 * The game loop is a thread that calls RunFrame() in a loop at a rate
 * determined by the playback speed and the client's FPS.
 *
 * \return true if the frontend should provide a game loop, false otherwise
 */
bool RequiresGameLoop(void);

/*!
 * \brief Run a single frame for add-ons that use a game loop
 *
 * \return the error, or GAME_ERROR_NO_ERROR if there was no error
 */
GAME_ERROR RunFrame(void);

/*!
 * \brief Reset the current game
 *
 * \return the error, or GAME_ERROR_NO_ERROR if the game was reset
 */
GAME_ERROR Reset(void);

// --- Hardware rendering operations -------------------------------------------

/*!
 * \brief Invalidates the current HW context and reinitializes GPU resources
 *
 * Any GL state is lost, and must not be deinitialized explicitly.
 *
 * \return the error, or GAME_ERROR_NO_ERROR if the HW context was reset
 */
GAME_ERROR HwContextReset(void);

/*!
 * \brief Called before the context is destroyed
 *
 * Resources can be deinitialized at this step.
 *
 * \return the error, or GAME_ERROR_NO_ERROR if the HW context was destroyed
 */
GAME_ERROR HwContextDestroy(void);

// --- Input operations --------------------------------------------------------

/*!
 * \brief Check if input is accepted for a feature on the controller
 *
 * If only a subset of the controller profile is used, this can return false
 * for unsupported features to not absorb their input.
 *
 * If the entire controller profile is used, this should always return true.
 *
 * \param controller_id The ID of the controller profile
 * \param feature_name The name of a feature in that profile
 * \return true if input is accepted for the feature, false otherwise
 */
bool HasFeature(const char* controller_id, const char* feature_name);

/*!
 * \brief Get the input topology that specifies which controllers can be connected
 *
 * \return The input topology, or null to use the default
 *
 * If this returns non-null, topology must be freed using FreeTopology().
 *
 * If this returns null, the topology will default to a single port that can
 * accept all controllers imported by addon.xml. The port ID is set to
 * the DEFAULT_PORT_ID constant.
 */
game_input_topology* GetTopology();

/*!
 * \brief Free the topology's resources
 *
 * \param topology The topology returned by GetTopology()
 */
void FreeTopology(game_input_topology* topology);

/*!
 * \brief Enable/disable keyboard input using the specified controller
 *
 * \param enable True to enable input, false otherwise
 * \param controller The controller info if enabling, or unused if disabling
 *
 * \return True if keyboard input was enabled, false otherwise
 */
bool EnableKeyboard(bool enable, const game_controller* controller);

/*!
 * \brief Enable/disable mouse input using the specified controller
 *
 * \param enable True to enable input, false otherwise
 * \param controller The controller info if enabling, or unused if disabling
 *
 * \return True if mouse input was enabled, false otherwise
 */
bool EnableMouse(bool enable, const game_controller* controller);

/*!
 * \brief Connect/disconnect a controller to a port on the virtual game console
 *
 * \param connect True to connect a controller, false to disconnect
 * \param address The address of the port
 * \param controller The controller info if connecting, or unused if disconnecting
 *
 * The address is a string that allows traversal of the controller topology.
 * It is formed by alternating port IDs and controller IDs separated by "/".
 *
 * For example, assume that the topology represented in XML for Snes9x is:
 *
 *     <logicaltopology>
 *       <port type="controller" id="1">
 *         <accepts controller="game.controller.snes"/>
 *         <accepts controller="game.controller.snes.multitap">
 *           <port type="controller" id="1">
 *             <accepts controller="game.controller.snes"/>
 *           </port>
 *           <port type="controller" id="2">
 *             <accepts controller="game.controller.snes"/>
 *           </port>
 *           ...
 *         </accepts>
 *       </port>
 *     </logicaltopology>
 *
 * To connect a multitap to the console's first port, the multitap's controller
 * info is set using the port address:
 *
 *     1
 *
 * To connect a SNES controller to the second port of the multitap, the
 * controller info is next set using the address:
 *
 *     1/game.controller.multitap/2
 *
 * Any attempts to connect a controller to a port on a disconnected multitap
 * will return false.
 */
bool ConnectController(bool connect, const char* port_address, const game_controller* controller);

/*!
 * \brief Notify the add-on of an input event
 *
 * \param event The input event
 *
 * \return true if the event was handled, false otherwise
 */
bool InputEvent(const game_input_event* event);

// --- Serialization operations ------------------------------------------------

/*!
 * \brief Get the number of bytes required to serialize the game
 *
 * \return the number of bytes, or 0 if serialization is not supported
 */
size_t SerializeSize(void);

/*!
 * \brief Serialize the state of the game
 *
 * \param data The buffer receiving the serialized game data
 * \param size The size of the buffer
 *
 * \return the error, or GAME_ERROR_NO_ERROR if the game was serialized into the buffer
 */
GAME_ERROR Serialize(uint8_t* data, size_t size);

/*!
 * \brief Deserialize the game from the given state
 *
 * \param data A buffer containing the game's new state
 * \param size The size of the buffer
 *
 * \return the error, or GAME_ERROR_NO_ERROR if the game deserialized
 */
GAME_ERROR Deserialize(const uint8_t* data, size_t size);

// --- Cheat operations --------------------------------------------------------

/*!
 * \brief Reset the cheat system
 *
 * \return the error, or GAME_ERROR_NO_ERROR if the cheat system was reset
 */
GAME_ERROR CheatReset(void);

/*!
 * \brief Get a region of memory
 *
 * \param type The type of memory to retrieve
 * \param data Set to the region of memory; must remain valid until UnloadGame() is called
 * \param size Set to the size of the region of memory
 *
 * \return the error, or GAME_ERROR_NO_ERROR if data was set to a valid buffer
 */
GAME_ERROR GetMemory(GAME_MEMORY type, uint8_t** data, size_t* size);

/*!
 * \brief Set a cheat code
 *
 * \param index
 * \param enabled
 * \param code
 *
 * \return the error, or GAME_ERROR_NO_ERROR if the cheat was set
 */
GAME_ERROR SetCheat(unsigned int index, bool enabled, const char* code);

// --- Add-on helper implementation --------------------------------------------

/*!
 * \brief Called by Kodi to assign the function pointers of this add-on to pClient
 *
 * Note that get_addon() is defined here, so it will be available in all
 * compiled game clients.
 */
void __declspec(dllexport) get_addon(void* ptr)
{
  AddonInstance_Game* pClient = static_cast<AddonInstance_Game*>(ptr);

  pClient->toAddon.LoadGame                 = LoadGame;
  pClient->toAddon.LoadGameSpecial          = LoadGameSpecial;
  pClient->toAddon.LoadStandalone           = LoadStandalone;
  pClient->toAddon.UnloadGame               = UnloadGame;
  pClient->toAddon.GetGameTiming            = GetGameTiming;
  pClient->toAddon.GetRegion                = GetRegion;
  pClient->toAddon.RequiresGameLoop         = RequiresGameLoop;
  pClient->toAddon.RunFrame                 = RunFrame;
  pClient->toAddon.Reset                    = Reset;
  pClient->toAddon.HwContextReset           = HwContextReset;
  pClient->toAddon.HwContextDestroy         = HwContextDestroy;
  pClient->toAddon.HasFeature               = HasFeature;
  pClient->toAddon.GetTopology              = GetTopology;
  pClient->toAddon.FreeTopology             = FreeTopology;
  pClient->toAddon.EnableKeyboard           = EnableKeyboard;
  pClient->toAddon.EnableMouse              = EnableMouse;
  pClient->toAddon.ConnectController        = ConnectController;
  pClient->toAddon.InputEvent               = InputEvent;
  pClient->toAddon.SerializeSize            = SerializeSize;
  pClient->toAddon.Serialize                = Serialize;
  pClient->toAddon.Deserialize              = Deserialize;
  pClient->toAddon.CheatReset               = CheatReset;
  pClient->toAddon.GetMemory                = GetMemory;
  pClient->toAddon.SetCheat                 = SetCheat;
}

#ifdef __cplusplus
}
#endif

