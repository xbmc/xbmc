/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <map>
#include "AddonString.h"

namespace XBMCAddon
{
  // This is a hack in order to handle int's as strings. The correct fix for
  // this is to get rid of Alternative all together and make the codegenerator
  // finally handle overloading correctly.
  typedef String StringOrInt;

  /**
   * This is a bit of a hack for dynamically typed languages. In some
   * cases python addon api calls handle dictionaries with variable
   * value types. In this case we coerce all of these types into
   * strings and then convert them back in the api. Yes, this is messy
   * and maybe we should use the CVariant here. But for now the
   * native api handles these calls by converting the string to the
   * appropriate types.
   */
  template<class T> class Dictionary : public std::map<String,T> {};

  typedef Dictionary<StringOrInt> Properties;
}
