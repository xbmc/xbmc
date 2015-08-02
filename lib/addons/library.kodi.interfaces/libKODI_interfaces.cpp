/*
 *      Copyright (C) 2013-2014 Team KODI
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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string>
#include "../../../addons/library.kodi.interfaces/libKODI_interfaces.h"
#include "addons/AddonCallbacks.h"

#ifdef _WIN32
#include <windows.h>
#define DLLEXPORT __declspec(dllexport)
#else
#define DLLEXPORT
#endif

using namespace std;

#define LIBRARY_NAME "libKODI_interfaces"

extern "C"
{

DLLEXPORT void* Interfaces_register_me(void *hdl)
{
  CB_InterfacesLib *cb = NULL;
  if (!hdl)
  {
    fprintf(stderr, "%s-ERROR: Interfaces_register_me is called with NULL handle !!!\n", LIBRARY_NAME);
  }
  else
  {
    cb = ((AddonCB*)hdl)->InterfacesLib_RegisterMe(((AddonCB*)hdl)->addonData);
    if (!cb)
      fprintf(stderr, "%s-ERROR: Interfaces_register_me can't get callback table from KODI !!!\n", LIBRARY_NAME);
  }
  return cb;
}

DLLEXPORT void Interfaces_unregister_me(void *hdl, void* cb)
{
  if (hdl && cb)
  {
    ((AddonCB*)hdl)->InterfacesLib_UnRegisterMe(((AddonCB*)hdl)->addonData, (CB_InterfacesLib*)cb);
  }
  else
  {
    fprintf(stderr, "%s-ERROR: Interfaces_unregister_me is called with NULL handle !!!\n", LIBRARY_NAME);
  }
}

DLLEXPORT int Interfaces_Execute_Script_Sync(void *hdl, void* cb, const char *Script, const char *AddonName, const char **Arguments, uint32_t TimeoutMs, bool WaitShutdown)
{
  if(!hdl || !cb)
  {
    return -1;
  }

  return ((CB_InterfacesLib*)cb)->ExecuteScriptSync(hdl, AddonName, Script, Arguments, TimeoutMs, WaitShutdown);
}

DLLEXPORT int Interfaces_Execute_Script_Async(void *hdl, void* cb, const char *Script, const char *AddonName, const char **Arguments)
{
  if(!hdl || !cb)
  {
    return -1;
  }

  return ((CB_InterfacesLib*)cb)->ExecuteScriptAsync(hdl, AddonName, Script, Arguments);
}

DLLEXPORT int Interfaces_Get_Python_Interpreter(void *hdl, void* cb)
{
  if(!cb)
  {
    return -1;
  }

  return ((CB_InterfacesLib*)cb)->GetPythonInterpreter(hdl);
}

CAddonPythonInterpreter::CAddonPythonInterpreter(void *Addon, void *Callbacks, int InterpreterId)
{
  if(!Addon || !Callbacks)
  {
    // TODO: error message!
  }

  if(InterpreterId < 0)
  {
    // TODO: error message!
  }
  else
  {
    m_InterpreterId = InterpreterId;
  }

  m_Callbacks   = Callbacks;
  m_AddonHandle = Addon;
}

CAddonPythonInterpreter::~CAddonPythonInterpreter()
{
  if(m_Callbacks && m_InterpreterId >= 0)
  {
    ((CB_InterfacesLib*)m_Callbacks)->DeactivatePythonInterpreter(m_AddonHandle, m_InterpreterId);
  }
}

bool CAddonPythonInterpreter::Activate()
{
  if(!m_Callbacks)
  {
    // TODO: error message!
    return false;
  }

  return ((CB_InterfacesLib*)m_Callbacks)->ActivatePythonInterpreter(m_AddonHandle, m_InterpreterId);
}

bool CAddonPythonInterpreter::Deactivate()
{
  if(!m_Callbacks)
  {
    // TODO: error message!
    return false;
  }

  return ((CB_InterfacesLib*)m_Callbacks)->DeactivatePythonInterpreter(m_AddonHandle, m_InterpreterId);
}

};