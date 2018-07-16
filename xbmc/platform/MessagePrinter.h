/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>
#include <utility>

class CMessagePrinter
{
public:

  /*! \brief Display a normal message to the user during startup
  *
  * \param[in] message  message to display
  */
  static void DisplayMessage(const std::string& message);

  /*! \brief Display a warning message to the user during startup
  *
  * \param[in] warning   warning to display
  */
  static void DisplayWarning(const std::string& warning);

  /*! \brief Display an error message to the user during startup
  *
  * \param[in] error  error to display
  */
  static void DisplayError(const std::string& error);

  /*! \brief Display the help message with command line options available
  *
  * \param[in] help  List of commands and explanations,
                     help.push_back(std::make_pair("--help", "this displays the help))
  */
  static void DisplayHelpMessage(const std::vector<std::pair<std::string, std::string>>& help);
};
