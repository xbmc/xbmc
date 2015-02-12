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

#pragma once

//---------------------------------------------------------
// This include should be moved to commons but even as it is,
// it wont cause a linker circular dependency since it's just
// a header.
#include "utils/StringUtils.h"
//---------------------------------------------------------
#include "ilog.h"
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
    static ILogger* logger;

    inline Exception(const char* classname_) : classname(classname_) { }
    inline Exception(const char* classname_, const char* message_) : classname(classname_), message(message_) { }
    inline Exception(const Exception& other) : classname(other.classname), message(other.message) { }

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

    inline virtual void LogThrowMessage(const char* prefix = NULL) const
    {
      if (logger)
        logger->Log(LOGERROR,"EXCEPTION Thrown (%s) : %s", classname.c_str(), message.c_str());
    }

    inline virtual const char* GetMessage() const { return message.c_str(); }

    inline static void SetLogger(ILogger* exceptionLogger) { logger = exceptionLogger; }
  };

  /**
   * This class forms the base class for unchecked exceptions. Unchecked exceptions
   * are those that really shouldn't be handled explicitly. For example, on windows
   * when a access violaton is converted to a win32_exception, there's nothing
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

