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

#if (defined HAVE_CONFIG_H) && (!defined WIN32)
  #include "config.h"
#endif

#include <Python.h>

#include "interfaces/legacy/LanguageHook.h"
#include "interfaces/python/CallbackHandler.h"
#include "threads/ThreadLocal.h"
#include "threads/Event.h"

namespace XBMCAddon
{
  namespace Python
  {
    struct MutableInteger;

    /**
     * This class supplies the python specific functionality for
     *  plugging into the API. It's got a static only implementation
     *  and uses the singleton pattern for access.
     */
    class LanguageHook : public XBMCAddon::LanguageHook
    {
      LanguageHook() : XBMCAddon::LanguageHook("Python::LanguageHook")  {  }

      XbmcThreads::ThreadLocal<PyThreadState> pyThreadStateTls;
      XbmcThreads::ThreadLocal<MutableInteger> tlsCount;
    public:

      virtual ~LanguageHook();

      virtual void delayedCallOpen();
      virtual void delayedCallClose();
      virtual void makePendingCalls();
      
      /**
       * PythonCallbackHandler expects to be instantiated PER AddonClass instance
       *  that is to be used as a callback. This is why this cannot be instantited
       *  once.
       *
       * There is an expectation that this method is called from the Python thread
       *  that instantiated an AddonClass that has the potential for a callback.
       *
       * See RetardedAsynchCallbackHandler for more details.
       * See PythonCallbackHandler for more details
       * See PythonCallbackHandler::PythonCallbackHandler for more details
       */
      virtual XBMCAddon::CallbackHandler* getCallbackHandler();

      virtual String getAddonId();
      virtual String getAddonVersion();

      virtual void registerPlayerCallback(IPlayerCallback* player);
      virtual void unregisterPlayerCallback(IPlayerCallback* player);
      virtual void registerMonitorCallback(XBMCAddon::xbmc::Monitor* monitor);
      virtual void unregisterMonitorCallback(XBMCAddon::xbmc::Monitor* monitor);
      virtual bool waitForEvent(CEvent& hEvent, unsigned int milliseconds);

      static LanguageHook* getInstance();
    };
  }
}

