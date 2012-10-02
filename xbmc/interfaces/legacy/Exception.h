/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */
#pragma once

#include "AddonString.h"
#include "AddonUtils.h"
#include "swighelper.h"

#include "utils/log.h"
#include "utils/StdString.h"
#include "commons/Exception.h"

#ifndef SWIG
namespace XBMCAddon
{
  XBMCCOMMONS_STANDARD_EXCEPTION(WrongTypeException);

  /**
   * UnimplementedException Can be used in places like the 
   *  Control hierarchy where the
   *  requirements of dynamic language usage force us to add 
   *  unimplmenented methods to a class hierarchy. See the 
   *  detailed explanation on the class Control for more.
   */
  class UnimplementedException : public XbmcCommons::Exception
  {
  public:
    inline UnimplementedException(const UnimplementedException& other) : Exception(other) { }
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
    inline UnhandledException(const UnhandledException& other) : Exception(other) { }
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
#define DECL_UNIMP(classname) throw(UnimplementedException) { throw UnimplementedException(classname, __FUNCTION__); }
#define DECL_UNIMP2(classname,otherexception) throw(UnimplementedException,otherexception) { throw UnimplementedException(classname, __FUNCTION__); }

