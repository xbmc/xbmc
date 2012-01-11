#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef _WIN32                   // windows
#include "dlfcn-win32.h"
#define ADDON_DLL               "\\library.xbmc.addon\\libXBMC_addon" ADDON_HELPER_EXT
#define ADDON_HELPER_PLATFORM   "win32"
#define ADDON_HELPER_EXT        ".dll"
#else
#if defined(__APPLE__)          // osx
#define ADDON_HELPER_PLATFORM   "osx"
#if defined(__POWERPC__)
#define ADDON_HELPER_ARCH       "powerpc"
#elif defined(__arm__)
#define ADDON_HELPER_ARCH       "arm"
#else
#define ADDON_HELPER_ARCH       "x86"
#endif
#else                           // linux
#define ADDON_HELPER_PLATFORM   "linux"
#if defined(__x86_64__)
#define ADDON_HELPER_ARCH       "x86_64"
#elif defined(_POWERPC)
#define ADDON_HELPER_ARCH       "powerpc"
#elif defined(_POWERPC64)
#define ADDON_HELPER_ARCH       "powerpc64"
#elif defined(_ARMEL)
#define ADDON_HELPER_ARCH       "arm"
#elif defined(_MIPSEL)
#define ADDON_HELPER_ARCH       "mipsel"
#else
#define ADDON_HELPER_ARCH       "i486"
#endif
#endif
#include <dlfcn.h>              // linux+osx
#define ADDON_HELPER_EXT        ".so"
#define ADDON_DLL "/library.xbmc.addon/libXBMC_addon-" ADDON_HELPER_ARCH "-" ADDON_HELPER_PLATFORM ADDON_HELPER_EXT
#endif

#ifdef LOG_DEBUG
#undef LOG_DEBUG
#endif
#ifdef LOG_INFO
#undef LOG_INFO
#endif
#ifdef LOG_NOTICE
#undef LOG_NOTICE
#endif
#ifdef LOG_ERROR
#undef LOG_ERROR
#endif

namespace ADDON
{
  typedef enum addon_log
  {
    LOG_DEBUG,
    LOG_INFO,
    LOG_NOTICE,
    LOG_ERROR
  } addon_log_t;

  typedef enum queue_msg
  {
    QUEUE_INFO,
    QUEUE_WARNING,
    QUEUE_ERROR
  } queue_msg_t;

  class CHelper_libXBMC_addon
  {
  public:
    CHelper_libXBMC_addon()
    {
      m_libXBMC_addon = NULL;
      m_Handle        = NULL;
    }

    ~CHelper_libXBMC_addon()
    {
      if (m_libXBMC_addon)
      {
        XBMC_unregister_me();
        dlclose(m_libXBMC_addon);
      }
    }

    bool RegisterMe(void *Handle)
    {
      m_Handle = Handle;

      std::string libBasePath;
      libBasePath  = ((cb_array*)m_Handle)->libPath;
      libBasePath += ADDON_DLL;

      m_libXBMC_addon = dlopen(libBasePath.c_str(), RTLD_LAZY);
      if (m_libXBMC_addon == NULL)
      {
        fprintf(stderr, "Unable to load %s\n", dlerror());
        return false;
      }

      XBMC_register_me   = (int (*)(void *HANDLE))
        dlsym(m_libXBMC_addon, "XBMC_register_me");
      if (XBMC_register_me == NULL)   { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_unregister_me = (void (*)())
        dlsym(m_libXBMC_addon, "XBMC_unregister_me");
      if (XBMC_unregister_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      Log                = (void (*)(const addon_log_t loglevel, const char *format, ... ))
        dlsym(m_libXBMC_addon, "XBMC_log");
      if (Log == NULL)                { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      GetSetting         = (bool (*)(const char* settingName, void *settingValue))
        dlsym(m_libXBMC_addon, "XBMC_get_setting");
      if (GetSetting == NULL)         { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      QueueNotification  = (void (*)(const queue_msg_t loglevel, const char *format, ... ))
        dlsym(m_libXBMC_addon, "XBMC_queue_notification");
      if (QueueNotification == NULL)  { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      UnknownToUTF8      = (void (*)(std::string &str))
        dlsym(m_libXBMC_addon, "XBMC_unknown_to_utf8");
      if (UnknownToUTF8 == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      GetLocalizedString = (const char* (*)(int dwCode))
        dlsym(m_libXBMC_addon, "XBMC_get_localized_string");
      if (GetLocalizedString == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      GetDVDMenuLanguage = (const char* (*)())
        dlsym(m_libXBMC_addon, "XBMC_get_dvd_menu_language");
      if (GetDVDMenuLanguage == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      return XBMC_register_me(m_Handle) > 0;
    }

    void (*Log)(const addon_log_t loglevel, const char *format, ... );
    bool (*GetSetting)(const char* settingName, void *settingValue);
    void (*QueueNotification)(const queue_msg_t type, const char *format, ... );
    void (*UnknownToUTF8)(std::string &str);
    const char* (*GetLocalizedString)(int dwCode);
    const char* (*GetDVDMenuLanguage)();

  protected:
    int (*XBMC_register_me)(void *HANDLE);
    void (*XBMC_unregister_me)();

  private:
    void *m_libXBMC_addon;
    void *m_Handle;
    struct cb_array
    {
      const char* libPath;
    };
  };
};
