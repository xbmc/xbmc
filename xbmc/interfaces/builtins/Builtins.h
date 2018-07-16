/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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

