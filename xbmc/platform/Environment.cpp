/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

/**
 * \file platform\Environment.cpp
 * \brief Implements CEnvironment class functions.
 *
 *  Some ideas were inspired by PostgreSQL's pgwin32_putenv function.
 *  Refined, updated, enhanced and modified for XBMC by Karlson2k.
 */

#include "Environment.h"
#include <stdlib.h>


// --------------------- Main Functions ---------------------

int CEnvironment::setenv(const std::string &name, const std::string &value, int overwrite /*= 1*/)
{
#ifdef TARGET_WINDOWS
  return (win_setenv(name, value, overwrite ? autoDetect : addOnly) == 0) ? 0 : -1;
#else
  if (value.empty() && overwrite != 0)
    return ::unsetenv(name.c_str());
  return ::setenv(name.c_str(), value.c_str(), overwrite);
#endif
}

std::string CEnvironment::getenv(const std::string &name)
{
#ifdef TARGET_WINDOWS
  return win_getenv(name);
#else
  char * str = ::getenv(name.c_str());
  if (str == NULL)
    return "";
  return str;
#endif
}

int CEnvironment::unsetenv(const std::string &name)
{
#ifdef TARGET_WINDOWS
  return (win_setenv(name, "", deleteVariable)) == 0 ? 0 : -1;
#else
  return ::unsetenv(name.c_str());
#endif
}

int CEnvironment::putenv(const std::string &envstring)
{
  if (envstring.empty())
    return 0;
  size_t pos = envstring.find('=');
  if (pos == 0) // '=' is the first character
    return -1;
  if (pos == std::string::npos)
    return unsetenv(envstring);
  if (pos == envstring.length()-1) // '=' is in last position
  {
    std::string name(envstring);
    name.erase(name.length()-1, 1);
    return unsetenv(name);
  }
  std::string name(envstring, 0, pos), value(envstring, pos+1);

  return setenv(name, value);
}

