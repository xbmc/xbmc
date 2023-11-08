/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/GameTypes.h"

#include <string>

class CGUIDialogSelect;

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 */
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
  static std::string ShowAndGetGameClient(const std::string& gamePath,
                                          const GameClientVector& candidates,
                                          const GameClientVector& installable);

private:
  /*!
   * \brief Get an initialized select dialog
   *
   * \param title The title of the select dialog
   *
   * \return A select dialog with its properties initialized, or nullptr if
   *         the dialog isn't found
   */
  static CGUIDialogSelect* GetDialog(const std::string& title);

  /*!
   * \brief Log the candidates and installable game clients
   *
   * Other than logging, this has no side effects.
   */
  static void LogGameClients(const GameClientVector& candidates,
                             const GameClientVector& installable);
};
} // namespace GAME
} // namespace KODI
