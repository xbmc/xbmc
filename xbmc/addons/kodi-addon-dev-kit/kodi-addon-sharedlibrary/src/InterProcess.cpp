/*
 *      Copyright (C) 2016 Team KODI
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

#include "InterProcess.h"

#include <stdio.h>

#ifdef _WIN32                   // windows
  #ifndef _SSIZE_T_DEFINED
    typedef intptr_t ssize_t;
    #define _SSIZE_T_DEFINED
  #endif // !_SSIZE_T_DEFINED
  #include "dlfcn-win32.h"
#else
  #include <dlfcn.h>            // linux+osx
#endif

extern "C"
{

CKODIAddon_InterProcess g_interProcess;

CKODIAddon_InterProcess::CKODIAddon_InterProcess()
  : m_libKODI_addon(NULL),
    m_Handle(NULL),
    m_Callbacks(NULL)
{

}

CKODIAddon_InterProcess::~CKODIAddon_InterProcess()
{
  Finalize();
}

//-- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - -- - --

int CKODIAddon_InterProcess::Init(int argc, char *argv[], addon_properties* props, const std::string &hostname)
{
  KODI_API_lasterror = API_ERR_TYPE;
  return KODI_API_lasterror;
}

int CKODIAddon_InterProcess::InitThread()
{
  if (m_Callbacks == NULL)
  {
    KODI_API_lasterror = API_ERR_BUFFER;
    return KODI_API_lasterror;
  }

  KODI_API_lasterror = API_SUCCESS;
  return KODI_API_lasterror;
}

int CKODIAddon_InterProcess::InitLibAddon(void* hdl)
{
  m_Handle = static_cast<ADDON::AddonCB*>(hdl);

  m_libKODI_addon = dlopen(NULL, RTLD_LAZY);
  if (m_libKODI_addon == NULL)
  {
    fprintf(stderr, "Unable to load %s\n", dlerror());
    KODI_API_lasterror = API_ERR_OP;
    return KODI_API_lasterror;
  }

  KODI_register = (_register_level*)
    dlsym(m_libKODI_addon, "AddOnLib_Register");
  if (KODI_register == NULL)
  {
    fprintf(stderr, "Unable to assign function %s\n", dlerror());
    KODI_API_lasterror = API_ERR_CONNECTION;
    return KODI_API_lasterror;
  }

  KODI_unregister = (_unregister_me*)
    dlsym(m_libKODI_addon, "AddOnLib_UnRegister");
  if (KODI_unregister == NULL)
  {
    fprintf(stderr, "Unable to assign function %s\n", dlerror());
    KODI_API_lasterror = API_ERR_CONNECTION;
    return KODI_API_lasterror;
  }

  m_Callbacks = KODI_register(m_Handle, 2);
  if (m_Callbacks == NULL)
  {
    KODI_API_lasterror = API_ERR_BUFFER;
    return KODI_API_lasterror;
  }

  KODI_API_lasterror = API_SUCCESS;
  return KODI_API_lasterror;
}

int CKODIAddon_InterProcess::Finalize()
{
  if (m_libKODI_addon)
  {
    KODI_unregister(m_Handle, m_Callbacks);
    dlclose(m_libKODI_addon);
    m_libKODI_addon = NULL;
  }

  KODI_API_lasterror = API_SUCCESS;
  return KODI_API_lasterror;
}

void CKODIAddon_InterProcess::Log(const addon_log loglevel, const char* string)
{
  m_Callbacks->addon_log_msg(m_Handle, loglevel, string);
}

}; /* extern "C" */
