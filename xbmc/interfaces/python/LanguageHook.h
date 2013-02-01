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
#include "threads/Event.h"

#include <set>
#include <map>

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
      PyInterpreterState* m_interp;
      CCriticalSection crit;
      std::set<AddonClass*> currentObjects;

      // This constructor is only used to instantiate the global LanguageHook
      inline LanguageHook() : 
        XBMCAddon::LanguageHook("Python::LanguageHook(Global)"), m_interp(NULL)  {  }

    public:

      inline LanguageHook(PyInterpreterState* interp) : 
        XBMCAddon::LanguageHook("Python::LanguageHook"), m_interp(interp)  {  }

      virtual ~LanguageHook();

      virtual void DelayedCallOpen();
      virtual void DelayedCallClose();
      virtual void MakePendingCalls();
      
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
      virtual XBMCAddon::CallbackHandler* GetCallbackHandler();

      virtual String GetAddonId();
      virtual String GetAddonVersion();

      virtual void RegisterPlayerCallback(IPlayerCallback* player);
      virtual void UnregisterPlayerCallback(IPlayerCallback* player);
      virtual void RegisterMonitorCallback(XBMCAddon::xbmc::Monitor* monitor);
      virtual void UnregisterMonitorCallback(XBMCAddon::xbmc::Monitor* monitor);
      virtual bool WaitForEvent(CEvent& hEvent, unsigned int milliseconds);

      static AddonClass::Ref<LanguageHook> GetIfExists(PyInterpreterState* interp);
      static bool IsAddonClassInstanceRegistered(AddonClass* obj);

      void RegisterAddonClassInstance(AddonClass* obj);
      void UnregisterAddonClassInstance(AddonClass* obj);
      bool HasRegisteredAddonClassInstance(AddonClass* obj);
      inline bool HasRegisteredAddonClasses() { Synchronize l(*this); return currentObjects.size() > 0; }

      // You should hold the lock on the LanguageHook itself if you're
      // going to do anything with the set that gets returned.
      inline std::set<AddonClass*>& GetRegisteredAddonClasses() { return currentObjects; }

      void UnregisterMe();
      void RegisterMe();
    };
  }
}

