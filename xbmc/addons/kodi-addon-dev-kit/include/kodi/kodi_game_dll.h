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
#ifndef KODI_GAME_DLL_H_
#define KODI_GAME_DLL_H_

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
 * \param type The game stype
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
 * \brief Get information about the loaded game
 *
 * \param info The info structure to fill
 *
 * \return the error, or GAME_ERROR_NO_ERROR if info was filled
 */
GAME_ERROR GetGameInfo(game_system_av_info* info);

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
 * \brief Notify the add-on of a status change on an open port
 *
 * Ports can be opened using the OpenPort() callback
 *
 * \param port Non-negative for a joystick port, or GAME_INPUT_PORT value otherwise
 * \param collected True if a controller was connected, false if disconnected
 * \param controller The connected controller
 */
void UpdatePort(int port, bool connected, const game_controller* controller);

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
  KodiToAddonFuncTable_Game* pClient = static_cast<KodiToAddonFuncTable_Game*>(ptr);

  pClient->LoadGame                 = LoadGame;
  pClient->LoadGameSpecial          = LoadGameSpecial;
  pClient->LoadStandalone           = LoadStandalone;
  pClient->UnloadGame               = UnloadGame;
  pClient->GetGameInfo              = GetGameInfo;
  pClient->GetRegion                = GetRegion;
  pClient->RequiresGameLoop         = RequiresGameLoop;
  pClient->RunFrame                 = RunFrame;
  pClient->Reset                    = Reset;
  pClient->HwContextReset           = HwContextReset;
  pClient->HwContextDestroy         = HwContextDestroy;
  pClient->UpdatePort               = UpdatePort;
  pClient->HasFeature               = HasFeature;
  pClient->InputEvent               = InputEvent;
  pClient->SerializeSize            = SerializeSize;
  pClient->Serialize                = Serialize;
  pClient->Deserialize              = Deserialize;
  pClient->CheatReset               = CheatReset;
  pClient->GetMemory                = GetMemory;
  pClient->SetCheat                 = SetCheat;
}

#ifdef __cplusplus
}
#endif

#endif // KODI_GAME_DLL_H_
