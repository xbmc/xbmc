/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#ifndef __GNUC__
#pragma warning(push)
#pragma warning(disable:4244)
#endif

#include "DynamicDll.h"
#include "utils/log.h"

#include <IL/OMX_Core.h>
#include <IL/OMX_Component.h>
#include <IL/OMX_Index.h>
#include <IL/OMX_Image.h>
#include <IL/OMX_Video.h>
#include <IL/OMX_Broadcom.h>

////////////////////////////////////////////////////////////////////////////////////////////

class DllOMXInterface
{
public:
  virtual ~DllOMXInterface() {}

  virtual OMX_ERRORTYPE OMX_Init(void) = 0;
  virtual OMX_ERRORTYPE OMX_Deinit(void) = 0;
  virtual OMX_ERRORTYPE OMX_GetHandle(OMX_HANDLETYPE *pHandle, OMX_STRING cComponentName, OMX_PTR pAppData, OMX_CALLBACKTYPE *pCallBacks) = 0;
  virtual OMX_ERRORTYPE OMX_FreeHandle(OMX_HANDLETYPE hComponent) = 0;
  virtual OMX_ERRORTYPE OMX_GetComponentsOfRole(OMX_STRING role, OMX_U32 *pNumComps, OMX_U8 **compNames) = 0;
  virtual OMX_ERRORTYPE OMX_GetRolesOfComponent(OMX_STRING compName, OMX_U32 *pNumRoles, OMX_U8 **roles) = 0;
  virtual OMX_ERRORTYPE OMX_ComponentNameEnum(OMX_STRING cComponentName, OMX_U32 nNameLength, OMX_U32 nIndex) = 0;
  virtual OMX_ERRORTYPE OMX_SetupTunnel(OMX_HANDLETYPE hOutput, OMX_U32 nPortOutput, OMX_HANDLETYPE hInput, OMX_U32 nPortInput) = 0;

};

#if (defined USE_EXTERNAL_OMX)
class DllOMX : public DllDynamic, DllOMXInterface
{
public:
  virtual OMX_ERRORTYPE OMX_Init(void)
    { return ::OMX_Init(); };
  virtual OMX_ERRORTYPE OMX_Deinit(void)
    { return ::OMX_Deinit(); };
  virtual OMX_ERRORTYPE OMX_GetHandle(OMX_HANDLETYPE *pHandle, OMX_STRING cComponentName, OMX_PTR pAppData, OMX_CALLBACKTYPE *pCallBacks)
    { return ::OMX_GetHandle(pHandle, cComponentName, pAppData, pCallBacks); };
  virtual OMX_ERRORTYPE OMX_FreeHandle(OMX_HANDLETYPE hComponent)
    { return ::OMX_FreeHandle(hComponent); };
  virtual OMX_ERRORTYPE OMX_GetComponentsOfRole(OMX_STRING role, OMX_U32 *pNumComps, OMX_U8 **compNames)
    { return ::OMX_GetComponentsOfRole(role, pNumComps, compNames); };
  virtual OMX_ERRORTYPE OMX_GetRolesOfComponent(OMX_STRING compName, OMX_U32 *pNumRoles, OMX_U8 **roles)
    { return ::OMX_GetRolesOfComponent(compName, pNumRoles, roles); };
  virtual OMX_ERRORTYPE OMX_ComponentNameEnum(OMX_STRING cComponentName, OMX_U32 nNameLength, OMX_U32 nIndex)
    { return ::OMX_ComponentNameEnum(cComponentName, nNameLength, nIndex); };
  virtual OMX_ERRORTYPE OMX_SetupTunnel(OMX_HANDLETYPE hOutput, OMX_U32 nPortOutput, OMX_HANDLETYPE hInput, OMX_U32 nPortInput)
    { return ::OMX_SetupTunnel(hOutput, nPortOutput, hInput, nPortInput); };
  virtual bool ResolveExports()
    { return true; }
  virtual bool Load()
  {
    CLog::Log(LOGDEBUG, "DllOMX: Using omx system library");
    return true;
  }
  virtual void Unload() {}
};
#else
class DllOMX : public DllDynamic, DllOMXInterface
{
  DECLARE_DLL_WRAPPER(DllOMX, "libopenmaxil.so")

  DEFINE_METHOD0(OMX_ERRORTYPE, OMX_Init)
  DEFINE_METHOD0(OMX_ERRORTYPE, OMX_Deinit)
  DEFINE_METHOD4(OMX_ERRORTYPE, OMX_GetHandle, (OMX_HANDLETYPE *p1, OMX_STRING p2, OMX_PTR p3, OMX_CALLBACKTYPE *p4))
  DEFINE_METHOD1(OMX_ERRORTYPE, OMX_FreeHandle, (OMX_HANDLETYPE p1))
  DEFINE_METHOD3(OMX_ERRORTYPE, OMX_GetComponentsOfRole, (OMX_STRING p1, OMX_U32 *p2, OMX_U8 **p3))
  DEFINE_METHOD3(OMX_ERRORTYPE, OMX_GetRolesOfComponent, (OMX_STRING p1, OMX_U32 *p2, OMX_U8 **p3))
  DEFINE_METHOD3(OMX_ERRORTYPE, OMX_ComponentNameEnum, (OMX_STRING p1, OMX_U32 p2, OMX_U32 p3))
  DEFINE_METHOD4(OMX_ERRORTYPE, OMX_SetupTunnel, (OMX_HANDLETYPE p1, OMX_U32 p2, OMX_HANDLETYPE p3, OMX_U32 p4));
  BEGIN_METHOD_RESOLVE()
    RESOLVE_METHOD(OMX_Init)
    RESOLVE_METHOD(OMX_Deinit)
    RESOLVE_METHOD(OMX_GetHandle)
    RESOLVE_METHOD(OMX_FreeHandle)
    RESOLVE_METHOD(OMX_GetComponentsOfRole)
    RESOLVE_METHOD(OMX_GetRolesOfComponent)
    RESOLVE_METHOD(OMX_ComponentNameEnum)
    RESOLVE_METHOD(OMX_SetupTunnel)
  END_METHOD_RESOLVE()

public:
  virtual bool Load()
  {
    return DllDynamic::Load();
  }
};
#endif
