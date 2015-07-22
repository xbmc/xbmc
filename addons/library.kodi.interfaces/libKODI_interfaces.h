#pragma once
/*
 *      Copyright (C) 2005-2014 Team KODI
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

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef BUILD_KODI_ADDON
#else
#endif
#include "libXBMC_addon.h"

#ifdef _WIN32
#define INTERFACES_HELPER_DLL "\\library.kodi.interfaces\\libKODI_interfaces" ADDON_HELPER_EXT
#else
#define INTERFACES_HELPER_DLL_NAME "libKODI_interfaces-" ADDON_HELPER_ARCH ADDON_HELPER_EXT
#define INTERFACES_HELPER_DLL "/library.kodi.interfaces/" INTERFACES_HELPER_DLL_NAME
#endif

class CAddonPythonInterpreter;

class CHelper_libKODI_interfaces
{
public:
  CHelper_libKODI_interfaces()
  {
    m_libKODI_interfaces = NULL;
    m_Handle             = NULL;
  }

  ~CHelper_libKODI_interfaces(void)
  {
    if(m_libKODI_interfaces)
    {
      Interfaces_unregister_me(m_Handle, m_Callbacks);
      dlclose(m_libKODI_interfaces);
    }
  }

  /*!
   * @brief Resolve all callback methods
   * @param handle Pointer to the add-on
   * @return True when all methods were resolved, false otherwise.
   */
  bool RegisterMe(void* handle)
  {
    m_Handle = handle;

    std::string libBasePath;
    libBasePath  = ((cb_array*)m_Handle)->libPath;
    libBasePath += INTERFACES_HELPER_DLL;

#if defined(ANDROID)
      struct stat st;
      if(stat(libBasePath.c_str(),&st) != 0)
      {
        std::string tempbin = getenv("XBMC_ANDROID_LIBS");
        libBasePath = tempbin + "/" + INTERFACES_HELPER_DLL;
      }
#endif

    m_libKODI_interfaces = dlopen(libBasePath.c_str(), RTLD_LAZY);
    if (m_libKODI_interfaces == NULL)
    {
      fprintf(stderr, "Unable to load %s\n", dlerror());
      return false;
    }

    Interfaces_register_me = (void* (*)(void *))
      dlsym(m_libKODI_interfaces, "Interfaces_register_me");
    if (Interfaces_register_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Interfaces_unregister_me = (void(*)(void*, void*))
      dlsym(m_libKODI_interfaces, "Interfaces_unregister_me");
    if (Interfaces_unregister_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Interfaces_Execute_Script_Sync = (int(*)(void*, void*, const char*, const char*, const char**, uint32_t, bool))
      dlsym(m_libKODI_interfaces, "Interfaces_Execute_Script_Sync");
    if (Interfaces_Execute_Script_Sync == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Interfaces_Execute_Script_Async = (int(*)(void*, void*, const char*, const char*, const char**))
      dlsym(m_libKODI_interfaces, "Interfaces_Execute_Script_Async");
    if (Interfaces_Execute_Script_Async == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    Interfaces_Get_Python_Interpreter = (int(*)(void*, void*))
      dlsym(m_libKODI_interfaces, "Interfaces_Get_Python_Interpreter");
    if(Interfaces_Get_Python_Interpreter == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

    m_Callbacks = Interfaces_register_me(m_Handle);
    return m_Callbacks != NULL;
  }

  /*!
   * \brief Executes the given script asynchronously in a separate thread.
   *
   * \param script Path to the script to be executed
   * \param addon (Optional) Addon to which the script belongs
   * \param arguments (Optional) List of arguments passed to the script
   * \return -1 if an error occurred, otherwise the ID of the script
   */
  int ExecuteScriptAsync( const std::string &Script, const std::string &AddonName=std::string(""), 
                          const std::vector<std::string> &Arguments = std::vector<std::string>())
  {
    char **arguments = NULL;
    if(Arguments.size() > 0)
    {
      arguments = new char*[Arguments.size() +1];
      for(int strIdx = 0; strIdx < (int)Arguments.size(); strIdx++)
      {
        arguments[strIdx] = new char[Arguments.at(strIdx).size() +1];
        strcpy(arguments[strIdx], Arguments.at(strIdx).c_str());
      }
      arguments[Arguments.size()] = NULL;
    }

    std::string addonName = AddonName;
    if(addonName.empty() || addonName == "")
    {
      addonName = ""; // ToDo: Add addon name
    }

    int iRet = Interfaces_Execute_Script_Async(m_Handle, m_Callbacks, Script.c_str(), addonName.c_str(), (const char**)arguments);

    if(arguments)
    {
      for(int strIdx = 0; strIdx < (int)Arguments.size(); strIdx++)
      {
        if(arguments[strIdx])
        {
          delete [] arguments[strIdx];
          arguments[strIdx] = NULL;
        }
      }

      delete [] arguments;
    }

    return iRet;
  }

  /*!
   * \brief Executes the given script synchronously.
   *
   * \details The script is actually executed asynchronously but the calling
   * thread is blocked until either the script has finished or the given timeout
   * has expired. If the given timeout has expired the script's execution is
   * stopped and depending on the specified wait behaviour we wait for the
   * script's execution to finish or not.
   *
   * \param script Path to the script to be executed
   * \param addon (Optional) Addon to which the script belongs
   * \param arguments (Optional) List of arguments passed to the script
   * \param timeout (Optional) Timeout (in milliseconds) for the script's execution
   * \param waitShutdown (Optional) Whether to wait when having to forcefully stop the script's execution or not.
   * \return -1 if an error occurred, 0 if the script terminated or ETIMEDOUT if the given timeout expired
   */
  int ExecuteScriptSync(const std::string &Script, const std::string &AddonName = std::string(""),
                        const std::vector<std::string> &Arguments = std::vector<std::string>(), uint32_t TimeoutMs = 0, bool WaitShutdown = false)
  {
    char **arguments = NULL;
    if(Arguments.size() > 0)
    {
      arguments = new char*[Arguments.size() +1];
      for(int strIdx = 0; strIdx < (int)Arguments.size(); strIdx++)
      {
        arguments[strIdx] = new char[Arguments.at(strIdx).size() +1];
        strcpy(arguments[strIdx], Arguments.at(strIdx).c_str());
      }
      arguments[Arguments.size()] = NULL;
    }

    std::string addonName = AddonName;
    if(addonName.empty() || addonName == "")
    {
      addonName = ""; // ToDo: Add addon name
    }

    int iRet = Interfaces_Execute_Script_Sync(m_Handle, m_Callbacks, Script.c_str(), addonName.c_str(), (const char**)arguments, TimeoutMs, WaitShutdown);

    if(arguments)
    {
      for(int strIdx = 0; strIdx < (int)Arguments.size(); strIdx++)
      {
        if(arguments[strIdx])
        {
          delete [] arguments[strIdx];
          arguments[strIdx] = NULL;
        }
      }
      
      delete [] arguments;
    }

    return iRet;
  }

protected:
  int   (*Interfaces_Execute_Script_Sync)(void *hdl, void *cb, const char *AddonName, const char *Script, const char **Arguments, uint32_t TimeoutMs, bool WaitShutdown);
  int   (*Interfaces_Execute_Script_Async)(void *hdl, void *cb, const char *AddonName, const char *Script, const char **Arguments);
  void  *(*Interfaces_register_me)(void *HANDLE);
  void  (*Interfaces_unregister_me)(void* HANDLE, void* CB);
  int   (*Interfaces_Get_Python_Interpreter)(void *hdl, void *cb);
  
private:
  void *m_libKODI_interfaces;
  void *m_Handle;
  void *m_Callbacks;
  struct cb_array
  {
    const char* libPath;
  };
};

class CAddonPythonInterpreter
{
public:
  CAddonPythonInterpreter(void *Addon, void *Callbacks, int InterpreterId);
  virtual ~CAddonPythonInterpreter();

  /**
  * TODO
  * @return true, when the Python Interpreter is active and usable. If something goes wrong the method returns false
  *         and detailed error message is written to Kodi's log file.
  */
  virtual bool Activate();

  /**
  * TODO
  * @return true, when the Python Interpreter is deactivated. If something goes wrong the method returns false
  *         and detailed error message is written to Kodi's log file.
  */
  virtual bool Deactivate();

private:
  int   m_InterpreterId;
  void  *m_AddonHandle;
  void  *m_Callbacks;
};