/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "commons/Exception.h"
#include "utils/log.h"

#ifndef SWIG
namespace XBMCAddon
{
  XBMCCOMMONS_STANDARD_EXCEPTION(WrongTypeException);

  /**
   * UnimplementedException Can be used in places like the
   *  Control hierarchy where the
   *  requirements of dynamic language usage force us to add
   *  unimplemented methods to a class hierarchy. See the
   *  detailed explanation on the class Control for more.
   */
  class UnimplementedException : public XbmcCommons::Exception
  {
  public:
    inline UnimplementedException(const UnimplementedException& other) = default;
    inline UnimplementedException(const char* classname, const char* methodname) :
      Exception("UnimplementedException")
    { SetMessage("Unimplemented method: %s::%s(...)", classname, methodname); }
  };

  /**
   * This is what callback exceptions from the scripting language
   *  are translated to.
   */
  class UnhandledException : public XbmcCommons::Exception
  {
  public:
    inline UnhandledException(const UnhandledException& other) = default;
    inline UnhandledException(const char* _message,...) : Exception("UnhandledException") { XBMCCOMMONS_COPYVARARGS(_message); }
  };
}
#endif

/**
 * These macros allow the easy declaration (and definition) of parent
 *  class virtual methods that are not implemented until the child class.
 *  This is to support the idosyncracies of dynamically typed scripting
 *  languages. See the comment in AddonControl.h for more details.
 */
#define THROW_UNIMP(classname) throw UnimplementedException(classname, __FUNCTION__)

