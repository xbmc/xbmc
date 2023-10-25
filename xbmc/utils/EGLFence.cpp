/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "EGLFence.h"

#include "EGLUtils.h"
#include "utils/log.h"

using namespace KODI::UTILS::EGL;

CEGLFence::CEGLFence(EGLDisplay display)
  : m_display(display),
    m_eglCreateSyncKHR(
        CEGLUtils::GetRequiredProcAddress<PFNEGLCREATESYNCKHRPROC>("eglCreateSyncKHR")),
    m_eglDestroySyncKHR(
        CEGLUtils::GetRequiredProcAddress<PFNEGLDESTROYSYNCKHRPROC>("eglDestroySyncKHR")),
    m_eglGetSyncAttribKHR(
        CEGLUtils::GetRequiredProcAddress<PFNEGLGETSYNCATTRIBKHRPROC>("eglGetSyncAttribKHR"))
{
#if defined(EGL_ANDROID_native_fence_sync) && defined(EGL_KHR_fence_sync)
  m_eglDupNativeFenceFDANDROID =
      CEGLUtils::GetRequiredProcAddress<PFNEGLDUPNATIVEFENCEFDANDROIDPROC>(
          "eglDupNativeFenceFDANDROID");
  m_eglClientWaitSyncKHR =
      CEGLUtils::GetRequiredProcAddress<PFNEGLCLIENTWAITSYNCKHRPROC>("eglClientWaitSyncKHR");
  m_eglWaitSyncKHR = CEGLUtils::GetRequiredProcAddress<PFNEGLWAITSYNCKHRPROC>("eglWaitSyncKHR");
#endif
}

void CEGLFence::CreateFence()
{
  m_fence = m_eglCreateSyncKHR(m_display, EGL_SYNC_FENCE_KHR, nullptr);
  if (m_fence == EGL_NO_SYNC_KHR)
  {
    CEGLUtils::Log(LOGERROR, "failed to create egl sync fence");
    throw std::runtime_error("failed to create egl sync fence");
  }
}

void CEGLFence::DestroyFence()
{
  if (m_fence == EGL_NO_SYNC_KHR)
  {
    return;
  }

  if (m_eglDestroySyncKHR(m_display, m_fence) != EGL_TRUE)
  {
    CEGLUtils::Log(LOGERROR, "failed to destroy egl sync fence");
  }

  m_fence = EGL_NO_SYNC_KHR;
}

bool CEGLFence::IsSignaled()
{
  // fence has been destroyed so return true immediately so buffer can be used
  if (m_fence == EGL_NO_SYNC_KHR)
  {
    return true;
  }

  EGLint status = EGL_UNSIGNALED_KHR;
  if (m_eglGetSyncAttribKHR(m_display, m_fence, EGL_SYNC_STATUS_KHR, &status) != EGL_TRUE)
  {
    CEGLUtils::Log(LOGERROR, "failed to query egl sync fence");
    return false;
  }

  if (status == EGL_SIGNALED_KHR)
  {
    return true;
  }

  return false;
}

#if defined(EGL_ANDROID_native_fence_sync) && defined(EGL_KHR_fence_sync)
EGLSyncKHR CEGLFence::CreateFence(int fd)
{
  CEGLAttributes<1> attributeList;
  attributeList.Add({{EGL_SYNC_NATIVE_FENCE_FD_ANDROID, fd}});

  EGLSyncKHR fence =
      m_eglCreateSyncKHR(m_display, EGL_SYNC_NATIVE_FENCE_ANDROID, attributeList.Get());

  if (fence == EGL_NO_SYNC_KHR)
  {
    CEGLUtils::Log(LOGERROR, "failed to create EGL sync object");
    return nullptr;
  }

  return fence;
}

void CEGLFence::CreateGPUFence()
{
  m_gpuFence = CreateFence(EGL_NO_NATIVE_FENCE_FD_ANDROID);
}

void CEGLFence::CreateKMSFence(int fd)
{
  m_kmsFence = CreateFence(fd);
}

EGLint CEGLFence::FlushFence()
{
  EGLint fd = m_eglDupNativeFenceFDANDROID(m_display, m_gpuFence);
  if (fd == EGL_NO_NATIVE_FENCE_FD_ANDROID)
    CEGLUtils::Log(LOGERROR, "failed to duplicate EGL fence fd");

  m_eglDestroySyncKHR(m_display, m_gpuFence);

  return fd;
}

void CEGLFence::WaitSyncGPU()
{
  if (!m_kmsFence)
    return;

  if (m_eglWaitSyncKHR(m_display, m_kmsFence, 0) != EGL_TRUE)
    CEGLUtils::Log(LOGERROR, "failed to create EGL sync point");
}

void CEGLFence::WaitSyncCPU()
{
  if (!m_kmsFence)
    return;

  EGLint status{EGL_FALSE};

  while (status != EGL_CONDITION_SATISFIED_KHR)
    status = m_eglClientWaitSyncKHR(m_display, m_kmsFence, 0, EGL_FOREVER_KHR);

  m_eglDestroySyncKHR(m_display, m_kmsFence);
}
#endif
