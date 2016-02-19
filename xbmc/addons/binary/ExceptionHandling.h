#pragma once
/*
 *      Copyright (C) 2015-2016 Team KODI
 *      http://kodi.tv
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "utils/log.h"
#include "commons/Exception.h"

namespace ADDON
{
  class CAddon;

  /**
   * WrongValueException Becomes used for pointer checks and check of parts which
   * can cause segmention faults.
   * Is to disable add-on before a segmention fault comes.
   */
  class WrongValueException : public XbmcCommons::Exception
  {
  public:
    inline WrongValueException(const WrongValueException& other) : Exception(other) { }
    inline WrongValueException(CAddon* addon, const char* _message, ...)
      : Exception("WrongValueException"),
        m_addon(addon)
    {
      XBMCCOMMONS_COPYVARARGS(_message);
    }
    inline WrongValueException(const char* _message, ...)
      : Exception("WrongValueException"),
        m_addon(nullptr)
    {
      XBMCCOMMONS_COPYVARARGS(_message);
    }
    inline const CAddon* GetRelatedAddon() const { return m_addon; }

  private:
    int m_signum;
    CAddon *m_addon;
  };

  /**
   * UnimplementedException Can be used in places like the control hierarchy
   * where the requirements of dynamic language usage force us to add
   * unimplmenented methods to a class hierarchy. See the detailed explanation
   * on the class Control for more.
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
   * This is what callback exceptions from the scripting language are translated
   * to.
   */
  class UnhandledException : public XbmcCommons::Exception
  {
  public:
    inline UnhandledException(const UnhandledException& other) : Exception(other) { }
    inline UnhandledException(const char* _message,...) : Exception("UnhandledException") { XBMCCOMMONS_COPYVARARGS(_message); }
  };

  class CAddon;

  class CAddonExceptionHandler
  {
  public:
    static void Handle(const ADDON::WrongValueException& e);
    static void Handle(const ADDON::UnimplementedException e);
    static void Handle(const XbmcCommons::Exception& e);
    static void HandleUnknown(std::string functionName);

  private:
    static void DestroyAddon(const CAddon* addon);
  };

/**
 * These macros allow the easy declaration (and definition) of parent class
 * virtual methods that are not implemented until the child class.
 *
 * This is to support the idosyncracies of dynamically typed scripting
 * languages. See the comment in AddonControl.h for more details.
 */
#define THROW_UNIMP(classname) throw ADDON::UnimplementedException(classname, __FUNCTION__)

#define HANDLE_ADDON_EXCEPTION                                       \
  XBMCCOMMONS_HANDLE_UNCHECKED                                       \
  catch (const ADDON::WrongValueException& e)                        \
  {                                                                  \
    CAddonExceptionHandler::Handle(e);                               \
  }                                                                  \
  catch (const ADDON::UnimplementedException& e)                     \
  {                                                                  \
    CAddonExceptionHandler::Handle(e);                               \
  }                                                                  \
  catch (const XbmcCommons::Exception& e)                            \
  {                                                                  \
    CAddonExceptionHandler::Handle(e);                               \
  }                                                                  \
  catch (...)                                                        \
  {                                                                  \
    CAddonExceptionHandler::HandleUnknown(__PRETTY_FUNCTION__);      \
  }

}; /* namespace ADDON */
