/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

//---------------------------------------------------------
// This include should be moved to commons but even as it is,
// it wont cause a linker circular dependency since it's just
// a header.
#include "utils/StringUtils.h"
//---------------------------------------------------------
#include <stdarg.h>


#ifdef __GNUC__
// The 'this' pointer counts as a parameter on member methods.
#define XBMCCOMMONS_ATTRIB_EXCEPTION_FORMAT __attribute__((format(printf,2,3)))
#else
#define XBMCCOMMONS_ATTRIB_EXCEPTION_FORMAT
#endif

#define XBMCCOMMONS_COPYVARARGS(fmt) va_list argList; va_start(argList, fmt); Set(fmt, argList); va_end(argList)
#define XBMCCOMMONS_STANDARD_EXCEPTION(E) \
  class E : public XbmcCommons::Exception \
  { \
  public: \
    inline E(const char* message,...) XBMCCOMMONS_ATTRIB_EXCEPTION_FORMAT : Exception(#E) { XBMCCOMMONS_COPYVARARGS(message); } \
    \
    inline E(const E& other) : Exception(other) {} \
  }

namespace XbmcCommons
{
  /**
   * This class a superclass for exceptions that want to utilize some
   * utility functionality including autologging with the specific
   * exception name.
   */
  class Exception
  {
  private:

    std::string classname;
    std::string message;

  protected:

    inline explicit Exception(const char* classname_) : classname(classname_) { }
    inline Exception(const char* classname_, const char* message_) : classname(classname_), message(message_) { }
    inline Exception(const Exception& other) = default;

    /**
     * This method is called from the constructor of subclasses. It
     * will set the message from varargs as well as call log message
     */
    inline void Set(const char* fmt, va_list& argList)
    {
      message = StringUtils::FormatV(fmt, argList);
    }

    /**
     * This message can be called from the constructor of subclasses.
     * It will set the message and log the throwing.
     */
    inline void SetMessage(const char* fmt, ...) XBMCCOMMONS_ATTRIB_EXCEPTION_FORMAT
    {
      // calls 'set'
      XBMCCOMMONS_COPYVARARGS(fmt);
    }

    inline void setClassname(const char* cn) { classname = cn; }

  public:
    virtual ~Exception();

    virtual void LogThrowMessage(const char* prefix = NULL) const;

    inline virtual const char* GetExMessage() const { return message.c_str(); }
  };

  /**
   * This class forms the base class for unchecked exceptions. Unchecked exceptions
   * are those that really shouldn't be handled explicitly. For example, on windows
   * when a access violation is converted to a win32_exception, there's nothing
   * that can be done in most code. The outer most stack frame might try to
   * do some error logging prior to shutting down, but that's really it.
   */
  XBMCCOMMONS_STANDARD_EXCEPTION(UncheckedException);

/**
 * In cases where you catch(...){} you will (may) inadvertently be
 * catching UncheckedException's. Therefore this macro will allow
 * you to do something equivalent to:
 *    catch (anything except UncheckedException) {}
 *
 * In order to avoid catching UncheckedException, use the macro as follows:
 *
 *    try { ... }
 *    XBMCCOMMONS_HANDLE_UNCHECKED
 *    catch(...){ ... }
 */
// Yes. I recognize that the name of this macro is an oxymoron.
#define  XBMCCOMMONS_HANDLE_UNCHECKED \
  catch (const XbmcCommons::UncheckedException& ) { throw; } \
  catch (const XbmcCommons::UncheckedException* ) { throw; }

}

