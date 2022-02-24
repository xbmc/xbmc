/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "interfaces/legacy/LanguageHook.h"
#include "threads/Event.h"

#include <map>
#include <mutex>
#include <set>

#include <Python.h>

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

      inline explicit PythonLanguageHook(PyInterpreterState* interp) : m_interp(interp)  {  }
      ~PythonLanguageHook() override;

      void DelayedCallOpen() override;
      void DelayedCallClose() override;
      void MakePendingCalls() override;

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
      XBMCAddon::CallbackHandler* GetCallbackHandler() override;

      String GetAddonId() override;
      String GetAddonVersion() override;
      long GetInvokerId() override;

      void RegisterPlayerCallback(IPlayerCallback* player) override;
      void UnregisterPlayerCallback(IPlayerCallback* player) override;
      void RegisterMonitorCallback(XBMCAddon::xbmc::Monitor* monitor) override;
      void UnregisterMonitorCallback(XBMCAddon::xbmc::Monitor* monitor) override;
      bool WaitForEvent(CEvent& hEvent, unsigned int milliseconds) override;

      static AddonClass::Ref<PythonLanguageHook> GetIfExists(PyInterpreterState* interp);
      static bool IsAddonClassInstanceRegistered(AddonClass* obj);

      void RegisterAddonClassInstance(AddonClass* obj);
      void UnregisterAddonClassInstance(AddonClass* obj);
      bool HasRegisteredAddonClassInstance(AddonClass* obj);
      inline bool HasRegisteredAddonClasses()
      {
        std::unique_lock<CCriticalSection> l(*this);
        return !currentObjects.empty();
      }

      // You should hold the lock on the LanguageHook itself if you're
      // going to do anything with the set that gets returned.
      inline std::set<AddonClass*>& GetRegisteredAddonClasses() { return currentObjects; }

      void UnregisterMe();
      void RegisterMe();
    };
  }
}

