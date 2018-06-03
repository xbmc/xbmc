/*
 *      Copyright (C) 2016-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */
#pragma once

#include "games/GameTypes.h"

#include <string>

class CGUIDialogSelect;

namespace KODI
{
namespace GAME
{
  class CGUIDialogSelectGameClient
  {
  public:
    /*!
     * \brief Show a series of dialogs that results in a game client being
     *        selected
     *
     * \param gamePath    The path of the file being played
     * \param candidates  A list of installed candidates that the user can
     *                    select from
     * \param installable A list of installable candidates that the user can
     *                    select from
     *
     * \return The ID of the selected game client, or empty if no game client
     *         was selected
     */
    static std::string ShowAndGetGameClient(const std::string &gamePath, const GameClientVector& candidates, const GameClientVector& installable);

  private:
    /*!
     * \brief Install the specified game client
     *
     * If the game client is not installed, a model dialog is shown installing
     * the game client. If the installation fails, an error dialog is shown.
     *
     * \param gameClient The game client to install
     *
     * \return True if the game client is installed, false otherwise
     */
    static bool Install(const std::string &gameClient);

    /*!
     * \brief Enable the specified game client
     *
     * \param gameClient the game client to enable
     *
     * \return True if the game client is enabled, false otherwise
     */
    static bool Enable(const std::string &gameClient);

    /*!
     * \brief Get an initialized select dialog
     *
     * \param title The title of the select dialog
     *
     * \return A select dialog with its properties initialized, or nullptr if
     *         the dialog isn't found
     */
    static CGUIDialogSelect *GetDialog(const std::string &title);

    /*!
     * \brief Log the candidates and installable game clients
     *
     * Other than logging, this has no side effects.
     */
    static void LogGameClients(const GameClientVector& candidates, const GameClientVector& installable);
  };
}
}
