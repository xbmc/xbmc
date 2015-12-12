/*
*      Copyright (C) 2005-2015 Team XBMC
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