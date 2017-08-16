#pragma once

/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <map>
#include <string>
#include <vector>

class CBuiltins
{
public:
  //! \brief Struct representing a command from handler classes.
  struct BUILT_IN
  {
    std::string description; //!< Description of command (help string)
    size_t parameters;       //!< Number of required parameters (can be 0)
    int (*Execute)(const std::vector<std::string>& params); //!< Function to handle command
  };

  //! \brief A map of commands
  typedef std::map<std::string,CBuiltins::BUILT_IN> CommandMap;

  static CBuiltins& GetInstance();

  bool HasCommand(const std::string& execString);
  bool IsSystemPowerdownCommand(const std::string& execString);
  void GetHelp(std::string &help);
  int Execute(const std::string& execString);

protected:
  CBuiltins();
  CBuiltins(const CBuiltins&) = delete;
  const CBuiltins& operator=(const CBuiltins&) = delete;
  virtual ~CBuiltins();

private:
  CommandMap m_command; //!< Map of registered commands


  //! \brief Convenience template used to register commands from providers
    template<class T>
  void RegisterCommands()
  {
    T t;
    CommandMap map = t.GetOperations();
    m_command.insert(map.begin(), map.end());
  }
};

