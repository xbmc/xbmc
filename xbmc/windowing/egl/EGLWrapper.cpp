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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "system.h"

#ifdef HAS_EGL

#include "utils/log.h"
#include "EGLNativeTypeAndroid.h"
#include "EGLNativeTypeAmlogic.h"
#include "EGLNativeTypeRaspberryPI.h"
#include "EGLWrapper.h"

#define CheckError() m_result = eglGetError(); if(m_result != EGL_SUCCESS) CLog::Log(LOGERROR, "EGL error in %s: %x",__FUNCTION__, m_result);

CEGLWrapper::CEGLWrapper()
{
  m_nativeTypes = NULL;
  m_result = EGL_SUCCESS;
}

CEGLWrapper::~CEGLWrapper()
{
  Destroy();
}

bool CEGLWrapper::Initialize(const std::string &implementation)
{
  bool ret = false;
  CEGLNativeType *nativeGuess = NULL;

  nativeGuess = new CEGLNativeTypeAndroid;
  if (nativeGuess->CheckCompatibility())
  {
    if(implementation == nativeGuess->GetNativeName() || implementation == "auto")
    {
      m_nativeTypes = nativeGuess;
      ret = true;
    }
  }

  if (!ret)
  {
    delete nativeGuess;
    nativeGuess = new CEGLNativeTypeAmlogic;
    if (nativeGuess->CheckCompatibility())
    {
      if(implementation == nativeGuess->GetNativeName() || implementation == "auto")
      {
        m_nativeTypes = nativeGuess;
        ret = true;
      }
    }
  }

  if (!ret)
  {
    delete nativeGuess;
    nativeGuess = new CEGLNativeTypeRaspberryPI;
    if (nativeGuess->CheckCompatibility())
    {
      if(implementation == nativeGuess->GetNativeName() || implementation == "auto")
      {
        m_nativeTypes = nativeGuess;
        ret = true;
      }
    }
  }

  if (ret && m_nativeTypes)
    m_nativeTypes->Initialize();

  return ret;
}

bool CEGLWrapper::Destroy()
{
  if (!m_nativeTypes)
    return false;

  m_nativeTypes->Destroy();

  delete m_nativeTypes;
  m_nativeTypes = NULL;
  return true;
}

std::string CEGLWrapper::GetNativeName()
{
  if (m_nativeTypes)
    return m_nativeTypes->GetNativeName();
  return "";
}

bool CEGLWrapper::CreateNativeDisplay()
{
  if(!m_nativeTypes)
    return false;

  return m_nativeTypes->CreateNativeDisplay();
}

bool CEGLWrapper::CreateNativeWindow()
{
  if(!m_nativeTypes)
    return false;

  return m_nativeTypes->CreateNativeWindow();
}

void CEGLWrapper::DestroyNativeDisplay()
{
  if(m_nativeTypes)
    m_nativeTypes->DestroyNativeDisplay();
}

void CEGLWrapper::DestroyNativeWindow()
{
  if(m_nativeTypes)
    m_nativeTypes->DestroyNativeWindow();
}

bool CEGLWrapper::SetNativeResolution(RESOLUTION_INFO& res)
{
  if (!m_nativeTypes)
    return false;
  return m_nativeTypes->SetNativeResolution(res);
}

bool CEGLWrapper::ProbeResolutions(std::vector<RESOLUTION_INFO> &resolutions)
{
  if (!m_nativeTypes)
    return false;
  return m_nativeTypes->ProbeResolutions(resolutions);
}

bool CEGLWrapper::GetPreferredResolution(RESOLUTION_INFO *res)
{
  if(!m_nativeTypes || !res)
    return false;

  return m_nativeTypes->GetPreferredResolution(res);
}

bool CEGLWrapper::GetNativeResolution(RESOLUTION_INFO *res)
{
  if(!m_nativeTypes || !res)
    return false;

  return m_nativeTypes->GetNativeResolution(res);
}

bool CEGLWrapper::ShowWindow(bool show)
{
  if (!m_nativeTypes)
    return false;
  
  return m_nativeTypes->ShowWindow(show);
}

bool CEGLWrapper::GetQuirks(int *quirks)
{
  if (!m_nativeTypes || !quirks)
    return false;
  *quirks = m_nativeTypes->GetQuirks();
  return true;
}

bool CEGLWrapper::InitDisplay(EGLDisplay *display)
{
  if (!display || !m_nativeTypes)
    return false;

  //nativeDisplay can be (and usually is) NULL. Don't use if(nativeDisplay) as a test!
  EGLint status;
  EGLNativeDisplayType *nativeDisplay = NULL;
  if (!m_nativeTypes->GetNativeDisplay((XBNativeDisplayType**)&nativeDisplay))
    return false;

  *display = eglGetDisplay(*nativeDisplay);
  CheckError();
  if (*display == EGL_NO_DISPLAY) 
  {
    CLog::Log(LOGERROR, "EGL failed to obtain display");
    return false;
  }

  status = eglInitialize(*display, 0, 0);
  CheckError();
  return status;
}

bool CEGLWrapper::ChooseConfig(EGLDisplay display, EGLint *configAttrs, EGLConfig *config)
{
  EGLBoolean eglStatus = true;
  EGLint     configCount = 0;
  EGLConfig* configList = NULL;

  // Find out how many configurations suit our needs  
  eglStatus = eglChooseConfig(display, configAttrs, NULL, 0, &configCount);
  CheckError();

  if (!eglStatus || !configCount)
  {
    CLog::Log(LOGERROR, "EGL failed to return any matching configurations: %i", configCount);
    return false;
  }

  // Allocate room for the list of matching configurations
  configList = (EGLConfig*)malloc(configCount * sizeof(EGLConfig));
  if (!configList)
  {
    CLog::Log(LOGERROR, "EGL failure obtaining configuration list");
    return false;
  }

  // Obtain the configuration list from EGL
  eglStatus = eglChooseConfig(display, configAttrs, configList, configCount, &configCount);
  CheckError();
  if (!eglStatus || !configCount)
  {
    CLog::Log(LOGERROR, "EGL failed to populate configuration list: %d", eglStatus);
    return false;
  }

  // Select an EGL configuration that matches the native window
  *config = configList[0];

  free(configList);
  return m_result == EGL_SUCCESS;
}

bool CEGLWrapper::CreateContext(EGLDisplay display, EGLConfig config, EGLint *contextAttrs, EGLContext *context)
{
  if (!context)
    return false;

  *context = eglCreateContext(display, config, NULL, contextAttrs);
  CheckError();
  return *context != EGL_NO_CONTEXT;
}

bool CEGLWrapper::CreateSurface(EGLDisplay display, EGLConfig config, EGLSurface *surface)
{
  if (!surface || !m_nativeTypes)
    return false;

  EGLNativeWindowType *nativeWindow=NULL;
  if (!m_nativeTypes->GetNativeWindow((XBNativeWindowType**)&nativeWindow))
    return false;

  *surface = eglCreateWindowSurface(display, config, *nativeWindow, NULL);
  CheckError();
  return *surface != EGL_NO_SURFACE;
}

bool CEGLWrapper::GetSurfaceSize(EGLDisplay display, EGLSurface surface, EGLint *width, EGLint *height)
{
  if (!width || !height)
    return false;

  if (!eglQuerySurface(display, surface, EGL_WIDTH, width)     ||
        !eglQuerySurface(display, surface, EGL_HEIGHT, height) ||
        *width <= 0 || *height <= 0)
  return false;

  return true;
}

bool CEGLWrapper::BindContext(EGLDisplay display, EGLSurface surface, EGLContext context)
{
  EGLBoolean status;
  status = eglMakeCurrent(display, surface, surface, context);
  CheckError();
  return status;
}

bool CEGLWrapper::BindAPI(EGLint type)
{
  EGLBoolean status;
  status = eglBindAPI(type);
  CheckError();
  return status && m_result == EGL_SUCCESS;
}

bool CEGLWrapper::ReleaseContext(EGLDisplay display)
{
  EGLBoolean status;
  if (display == EGL_NO_DISPLAY)
    return false;
  status = eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  CheckError();
  return status;
}

bool CEGLWrapper::DestroyContext(EGLDisplay display, EGLContext context)
{
  EGLBoolean status;
  if (display == EGL_NO_DISPLAY)
    return false;
  status = eglDestroyContext(display, context);
  CheckError();
  return status;
}

bool CEGLWrapper::DestroySurface(EGLSurface surface, EGLDisplay display)
{
  EGLBoolean status;

  status = eglDestroySurface(display, surface);
  CheckError();
  return status;
}

bool CEGLWrapper::DestroyDisplay(EGLDisplay display)
{
  EGLBoolean eglStatus;

  eglStatus = eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
  CheckError();
  if (!eglStatus)
    return false;

  eglStatus = eglTerminate(display);
  CheckError();
  if (!eglStatus)
    return false;

  return true;
}

std::string CEGLWrapper::GetExtensions(EGLDisplay display)
{
  std::string extensions = eglQueryString(display, EGL_EXTENSIONS);
  CheckError();
  return " " + extensions + " ";
}

bool CEGLWrapper::SetVSync(EGLDisplay display, bool enable)
{
  EGLBoolean status;
  // depending how buffers are setup, eglSwapInterval
  // might fail so let caller decide if this is an error.
  status = eglSwapInterval(display, enable ? 1 : 0);
  CheckError();
  return status;
}

void CEGLWrapper::SwapBuffers(EGLDisplay display, EGLSurface surface)
{
  if ((display == EGL_NO_DISPLAY) || (surface == EGL_NO_SURFACE))
    return;
  eglSwapBuffers(display, surface);
}

bool CEGLWrapper::GetConfigAttrib(EGLDisplay display, EGLConfig config, EGLint attribute, EGLint *value)
{
  if (display == EGL_NO_DISPLAY || !config || !attribute)
    return eglGetConfigAttrib(display, config, attribute, value);
  return false;
}
#endif

