/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "StringValidation.h"
#include "utils/StringUtils.h"

bool StringValidation::IsInteger(const std::string &input, void *data)
{
  return StringUtils::IsInteger(input);
}

bool StringValidation::IsPositiveInteger(const std::string &input, void *data)
{
  return StringUtils::IsNaturalNumber(input);
}

bool StringValidation::IsTime(const std::string &input, void *data)
{
  std::string strTime = input;
  StringUtils::Trim(strTime);

  if (StringUtils::EndsWithNoCase(strTime, " min"))
  {
    strTime = StringUtils::Left(strTime, strTime.size() - 4);
    StringUtils::TrimRight(strTime);

    return IsPositiveInteger(strTime, NULL);
  }
  else
  {
    // support [[HH:]MM:]SS
    std::vector<std::string> bits = StringUtils::Split(input, ":");
    if (bits.size() > 3)
      return false;

    for (std::vector<std::string>::const_iterator i = bits.begin(); i != bits.end(); ++i)
      if (!IsPositiveInteger(*i, NULL))
        return false;

    return true;
  }
  return false;
}
