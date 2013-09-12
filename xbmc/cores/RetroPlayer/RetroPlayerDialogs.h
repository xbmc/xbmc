/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "games/GameClient.h"

#include <string>
#include <vector>

class CFileItem;

class CRetroPlayerDialogs
{
public:
  /**
   * When a game is first launched, this is responsible for resolving the
   * appropriate game client by presenting the user with a series of menus and
   * dialogs (installing the client just-in-time, if necessary).
   * @param  file - the file being launched
   * @param  result - a pointer to the game client that will be used to run the game
   *         (valid pointer if function returns true, untouched if function returns false)
   * @return true if the loading should continue, false to abort
   */
  static bool GetGameClient(const CFileItem &file, GAMES::GameClientPtr &result);

private:
  /**
   * Ask the user if they would like to install a game client from a list, or
   * go to the add-on manager. If possible, the chosen client is then used to
   * launch the game. This enables just-in-time game client installation.
   * @param  file - used to generate the list of compatible game clients
   * @param  result - a pointer to the game client that will be used to run the game
   *         (valid pointer if function returns true, untouched if function returns false)
   * @return false if the user aborts the game launching process
   */
  static bool GameLauchDialog(const CFileItem &file, GAMES::GameClientPtr &result);

  /**
   * Download and install a game client from a remote repository.
   * @param  strId - the game client ID to download and install
   * @param  file - the file being opened, used to check game client's
             compatibility before downloading
   * @param  result - a pointer to the game client that will be used to run the game
   *         (valid pointer if function returns true, untouched if function returns false)
   * @return true if the game client is compatible and installs successfully,
   *         false if the client is already installed or fails to install.
   */
  static bool InstallGameClient(const std::string &strId, const CFileItem &file, GAMES::GameClientPtr &result);

  /**
   * Present the user with a list of game clients compatible with the specified
   * file and install the chosen game client.
   */
  static bool InstallGameClientDialog(const CFileItem &file, GAMES::GameClientPtr &result);

  /**
   * Present the user with a list of game clients and an option to go to the
   * add-on manager.
   * @param  clientIds - list of game client IDs
   * @param  file - game file being launched
   * @param  result - pointer to the game client that will be used to run the game
   *         (valid pointer if function returns true, untouched if function returns false)
   * @return true to continue launching the game, false to abort
   */
  static bool ChooseGameClientDialog(const std::vector<std::string> &clientIds, const CFileItem &file, GAMES::GameClientPtr &result);
};
