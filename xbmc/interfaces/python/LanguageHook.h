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
    class PythonLanguageHook : public XBMCAddon::LanguageHook
    {
      PyInterpreterState* m_interp;
      CCriticalSection crit;
      std::set<AddonClass*> currentObjects;

      // This constructor is only used to instantiate the global LanguageHook
      inline PythonLanguageHook() : m_interp(NULL)  {  }

    public:

      inline PythonLanguageHook(PyInterpreterState* interp) : m_interp(interp)  {  }
      virtual ~PythonLanguageHook();

      virtual void DelayedCallOpen();
      virtual void DelayedCallClose();
      virtual void MakePendingCalls();
      
      /**
       * PythonCallbackHandler expects to be instantiated PER AddonClass instance
       *  that is to be used as a callback. This is why this cannot be instantiated
       *  once.
       *
       * There is an expectation that this method is called from the Python thread
       *  that instantiated an AddonClass that has the potential for a callback.
       *
       * See RetardedAsyncCallbackHandler for more details.
       * See PythonCallbackHandler for more details
       * See PythonCallbackHandler::PythonCallbackHandler for more details
       */
      virtual XBMCAddon::CallbackHandler* GetCallbackHandler();

      virtual String GetAddonId();
      virtual String GetAddonVersion();
      virtual long GetInvokerId();

      virtual void RegisterPlayerCallback(IPlayerCallback* player);
      virtual void UnregisterPlayerCallback(IPlayerCallback* player);
      virtual void RegisterMonitorCallback(XBMCAddon::xbmc::Monitor* monitor);
      virtual void UnregisterMonitorCallback(XBMCAddon::xbmc::Monitor* monitor);
      virtual bool WaitForEvent(CEvent& hEvent, unsigned int milliseconds);

      static AddonClass::Ref<PythonLanguageHook> GetIfExists(PyInterpreterState* interp);
      static bool IsAddonClassInstanceRegistered(AddonClass* obj);

      void RegisterAddonClassInstance(AddonClass* obj);
      void UnregisterAddonClassInstance(AddonClass* obj);
      bool HasRegisteredAddonClassInstance(AddonClass* obj);
      inline bool HasRegisteredAddonClasses() { CSingleLock l(*this); return !currentObjects.empty(); }

      // You should hold the lock on the LanguageHook itself if you're
      // going to do anything with the set that gets returned.
      inline std::set<AddonClass*>& GetRegisteredAddonClasses() { return currentObjects; }

      void UnregisterMe();
      void RegisterMe();
    };
  }
}

