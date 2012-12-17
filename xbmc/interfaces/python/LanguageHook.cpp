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


#include "LanguageHook.h"
#include "CallbackHandler.h"
#include "XBPython.h"

#include "interfaces/legacy/AddonUtils.h"
#include "utils/GlobalsHandling.h"
#include "PyContext.h"

namespace XBMCAddon
{
  namespace Python
  {
    static AddonClass::Ref<LanguageHook> instance;

    static CCriticalSection hooksMutex;
    static std::map<PyInterpreterState*,AddonClass::Ref<LanguageHook> > hooks;

    // vtab instantiation
    LanguageHook::~LanguageHook()
    {
      TRACE;
      XBMCAddon::LanguageHook::deallocating();
    }

    void LanguageHook::MakePendingCalls()
    {
      TRACE;
      PythonCallbackHandler::makePendingCalls();
    }

    void LanguageHook::DelayedCallOpen()
    {
      TRACE;
      PyGILLock::releaseGil();
    }

    void LanguageHook::DelayedCallClose()
    {
      TRACE;
      PyGILLock::acquireGil();
    }

    void LanguageHook::RegisterMe()
    {
      TRACE;
      CSingleLock lock(hooksMutex);
      hooks[m_interp] = AddonClass::Ref<LanguageHook>(this);
    }

    void LanguageHook::UnregisterMe()
    {
      TRACE;
      CSingleLock lock(hooksMutex);
      hooks.erase(m_interp);
    }

    static AddonClass::Ref<XBMCAddon::Python::LanguageHook> g_languageHook;

    // Ok ... we're going to get it even if it doesn't exist. If it doesn't exist then
    // we're going to assume we're not in control of the interpreter. This (apparently)
    // can be the case. E.g. Libspotify manages to call into a script using a ctypes
    // extention but under the control of an Interpreter we know nothing about. In
    // cases like this we're going to use a global interpreter 
    AddonClass::Ref<LanguageHook> LanguageHook::GetIfExists(PyInterpreterState* interp)
    {
      TRACE;
      CSingleLock lock(hooksMutex);
      std::map<PyInterpreterState*,AddonClass::Ref<LanguageHook> >::iterator iter = hooks.find(interp);
      if (iter != hooks.end())
        return AddonClass::Ref<LanguageHook>(iter->second);

      // if we got here then we need to use the global one.
      if (g_languageHook.isNull())
        g_languageHook = new XBMCAddon::Python::LanguageHook();

      return g_languageHook;
    }

    bool LanguageHook::IsAddonClassInstanceRegistered(AddonClass* obj)
    {
      for (std::map<PyInterpreterState*,AddonClass::Ref<LanguageHook> >::iterator iter = hooks.begin();
           iter != hooks.end(); iter++)
      {
        if ((iter->second)->HasRegisteredAddonClassInstance(obj))
          return true;
      }
      return false;
    }

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
    XBMCAddon::CallbackHandler* LanguageHook::GetCallbackHandler()
    { 
      TRACE;
      return new PythonCallbackHandler();
    }

    String LanguageHook::GetAddonId()
    {
      TRACE;
      const char* id = NULL;

      // Get a reference to the main module
      // and global dictionary
      PyObject* main_module = PyImport_AddModule((char*)"__main__");
      PyObject* global_dict = PyModule_GetDict(main_module);
      // Extract a reference to the function "func_name"
      // from the global dictionary
      PyObject* pyid = PyDict_GetItemString(global_dict, "__xbmcaddonid__");
      id = PyString_AsString(pyid);
      return id;
    }

    String LanguageHook::GetAddonVersion()
    {
      TRACE;
      // Get a reference to the main module
      // and global dictionary
      PyObject* main_module = PyImport_AddModule((char*)"__main__");
      PyObject* global_dict = PyModule_GetDict(main_module);
      // Extract a reference to the function "func_name"
      // from the global dictionary
      PyObject* pyversion = PyDict_GetItemString(global_dict, "__xbmcapiversion__");
      String version(PyString_AsString(pyversion));
      return version;
    }

    void LanguageHook::RegisterPlayerCallback(IPlayerCallback* player) { TRACE; g_pythonParser.RegisterPythonPlayerCallBack(player); }
    void LanguageHook::UnregisterPlayerCallback(IPlayerCallback* player) { TRACE; g_pythonParser.UnregisterPythonPlayerCallBack(player); }
    void LanguageHook::RegisterMonitorCallback(XBMCAddon::xbmc::Monitor* monitor) { TRACE; g_pythonParser.RegisterPythonMonitorCallBack(monitor); }
    void LanguageHook::UnregisterMonitorCallback(XBMCAddon::xbmc::Monitor* monitor) { TRACE; g_pythonParser.UnregisterPythonMonitorCallBack(monitor); }

    bool LanguageHook::WaitForEvent(CEvent& hEvent, unsigned int milliseconds)
    { 
      TRACE;
      return g_pythonParser.WaitForEvent(hEvent,milliseconds);
    }

    void LanguageHook::RegisterAddonClassInstance(AddonClass* obj)
    {
      TRACE;
      Synchronize l(*this);
      obj->Acquire();
      currentObjects.insert(obj);
    }

    void LanguageHook::UnregisterAddonClassInstance(AddonClass* obj)
    {
      TRACE;
      Synchronize l(*this);
      if (currentObjects.erase(obj) > 0)
        obj->Release();
    }

    bool LanguageHook::HasRegisteredAddonClassInstance(AddonClass* obj)
    {
      TRACE;
      Synchronize l(*this);
      return currentObjects.find(obj) != currentObjects.end();
    }
  }
}
