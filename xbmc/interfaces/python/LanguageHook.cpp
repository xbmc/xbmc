/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


#include "LanguageHook.h"

#include "CallbackHandler.h"
#include "PyContext.h"
#include "XBPython.h"
#include "interfaces/legacy/AddonUtils.h"

namespace XBMCAddon
{
  namespace Python
  {
    static AddonClass::Ref<PythonLanguageHook> instance;

    static CCriticalSection hooksMutex;
    static std::map<PyInterpreterState*,AddonClass::Ref<PythonLanguageHook> > hooks;

    // vtab instantiation
    PythonLanguageHook::~PythonLanguageHook()
    {
      XBMC_TRACE;
      XBMCAddon::LanguageHook::deallocating();
    }

    void PythonLanguageHook::MakePendingCalls()
    {
      XBMC_TRACE;
      PythonCallbackHandler::makePendingCalls();
    }

    void PythonLanguageHook::DelayedCallOpen()
    {
      XBMC_TRACE;
      PyGILLock::releaseGil();
    }

    void PythonLanguageHook::DelayedCallClose()
    {
      XBMC_TRACE;
      PyGILLock::acquireGil();
    }

    void PythonLanguageHook::RegisterMe()
    {
      XBMC_TRACE;
      CSingleLock lock(hooksMutex);
      hooks[m_interp] = AddonClass::Ref<PythonLanguageHook>(this);
    }

    void PythonLanguageHook::UnregisterMe()
    {
      XBMC_TRACE;
      CSingleLock lock(hooksMutex);
      hooks.erase(m_interp);
    }

    static AddonClass::Ref<XBMCAddon::Python::PythonLanguageHook> g_languageHook;

    // Ok ... we're going to get it even if it doesn't exist. If it doesn't exist then
    // we're going to assume we're not in control of the interpreter. This (apparently)
    // can be the case. E.g. Libspotify manages to call into a script using a ctypes
    // extension but under the control of an Interpreter we know nothing about. In
    // cases like this we're going to use a global interpreter
    AddonClass::Ref<PythonLanguageHook> PythonLanguageHook::GetIfExists(PyInterpreterState* interp)
    {
      XBMC_TRACE;
      CSingleLock lock(hooksMutex);
      std::map<PyInterpreterState*,AddonClass::Ref<PythonLanguageHook> >::iterator iter = hooks.find(interp);
      if (iter != hooks.end())
        return iter->second;

      // if we got here then we need to use the global one.
      if (g_languageHook.isNull())
        g_languageHook = new XBMCAddon::Python::PythonLanguageHook();

      return g_languageHook;
    }

    bool PythonLanguageHook::IsAddonClassInstanceRegistered(AddonClass* obj)
    {
      for (const auto& iter : hooks)
      {
        if (iter.second->HasRegisteredAddonClassInstance(obj))
          return true;
      }
      return false;
    }

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
    XBMCAddon::CallbackHandler* PythonLanguageHook::GetCallbackHandler()
    {
      XBMC_TRACE;
      return new PythonCallbackHandler();
    }

    String PythonLanguageHook::GetAddonId()
    {
      XBMC_TRACE;

      // Get a reference to the main module
      // and global dictionary
      PyObject* main_module = PyImport_AddModule("__main__");
      PyObject* global_dict = PyModule_GetDict(main_module);
      // Extract a reference to the function "func_name"
      // from the global dictionary
      PyObject* pyid = PyDict_GetItemString(global_dict, "__xbmcaddonid__");
      if (pyid)
        return PyUnicode_AsUTF8(pyid);
      return "";
    }

    String PythonLanguageHook::GetAddonVersion()
    {
      XBMC_TRACE;
      // Get a reference to the main module
      // and global dictionary
      PyObject* main_module = PyImport_AddModule("__main__");
      PyObject* global_dict = PyModule_GetDict(main_module);
      // Extract a reference to the function "func_name"
      // from the global dictionary
      PyObject* pyversion = PyDict_GetItemString(global_dict, "__xbmcapiversion__");
      if (pyversion)
        return PyUnicode_AsUTF8(pyversion);
      return "";
    }

    long PythonLanguageHook::GetInvokerId()
    {
      XBMC_TRACE;

      // Get a reference to the main module
      // and global dictionary
      PyObject* main_module = PyImport_AddModule("__main__");
      PyObject* global_dict = PyModule_GetDict(main_module);
      // Extract a reference to the function "func_name"
      // from the global dictionary
      PyObject* pyid = PyDict_GetItemString(global_dict, "__xbmcinvokerid__");
      if (pyid)
        return PyLong_AsLong(pyid);
      return -1;
    }


    void PythonLanguageHook::RegisterPlayerCallback(IPlayerCallback* player) { XBMC_TRACE; g_pythonParser.RegisterPythonPlayerCallBack(player); }
    void PythonLanguageHook::UnregisterPlayerCallback(IPlayerCallback* player) { XBMC_TRACE; g_pythonParser.UnregisterPythonPlayerCallBack(player); }
    void PythonLanguageHook::RegisterMonitorCallback(XBMCAddon::xbmc::Monitor* monitor) { XBMC_TRACE; g_pythonParser.RegisterPythonMonitorCallBack(monitor); }
    void PythonLanguageHook::UnregisterMonitorCallback(XBMCAddon::xbmc::Monitor* monitor) { XBMC_TRACE; g_pythonParser.UnregisterPythonMonitorCallBack(monitor); }

    bool PythonLanguageHook::WaitForEvent(CEvent& hEvent, unsigned int milliseconds)
    {
      XBMC_TRACE;
      return g_pythonParser.WaitForEvent(hEvent,milliseconds);
    }

    void PythonLanguageHook::RegisterAddonClassInstance(AddonClass* obj)
    {
      XBMC_TRACE;
      CSingleLock l(*this);
      obj->Acquire();
      currentObjects.insert(obj);
    }

    void PythonLanguageHook::UnregisterAddonClassInstance(AddonClass* obj)
    {
      XBMC_TRACE;
      CSingleLock l(*this);
      if (currentObjects.erase(obj) > 0)
        obj->Release();
    }

    bool PythonLanguageHook::HasRegisteredAddonClassInstance(AddonClass* obj)
    {
      XBMC_TRACE;
      CSingleLock l(*this);
      return currentObjects.find(obj) != currentObjects.end();
    }
  }
}
