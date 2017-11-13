/*
 *      Copyright (C) 2017 Team XBMC
 *      http://xbmc.org
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

#include "WinSystemWaylandEGLContextGL.h"

#include <EGL/egl.h>

#include "cores/RetroPlayer/process/RPProcessInfo.h"
#include "cores/RetroPlayer/rendering/VideoRenderers/RPRendererGuiTexture.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGL.h"
#include "utils/log.h"

#if defined(HAVE_LIBVA)
#include "cores/VideoPlayer/DVDCodecs/Video/VAAPI.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererVAAPIGL.h"
using namespace KODI::WINDOWING::WAYLAND;
class CVaapiProxy : public VAAPI::IVaapiWinSystem
{
public:
  CVaapiProxy(CWinSystemWaylandEGLContextGL &winSystem) : m_winSystem(winSystem) {};
  VADisplay GetVADisplay() override { return m_winSystem.GetVaDisplay(); };
  void *GetEGLDisplay() override { return m_winSystem.GetEGLDisplay(); };
protected:
  CWinSystemWaylandEGLContextGL &m_winSystem;
};
#else
class CVaapiProxy
{
};
#endif

using namespace KODI::WINDOWING::WAYLAND;

std::unique_ptr<CWinSystemBase> CWinSystemBase::CreateWinSystem()
{
  std::unique_ptr<CWinSystemBase> winSystem(new CWinSystemWaylandEGLContextGL());
  return winSystem;
}

bool CWinSystemWaylandEGLContextGL::InitWindowSystem()
{
  if (!CWinSystemWaylandEGLContext::InitWindowSystemEGL(EGL_OPENGL_BIT, EGL_OPENGL_API))
  {
    return false;
  }

  CLinuxRendererGL::Register();
  RETRO::CRPProcessInfo::RegisterRendererFactory(new RETRO::CRendererFactoryGuiTexture);

#if defined(HAVE_LIBVA)
  bool general, hevc;
  m_vaapiProxy.reset(new CVaapiProxy(*this));
  CRendererVAAPI::Register(m_vaapiProxy.get(), GetVaDisplay(), m_eglContext.GetEGLDisplay(), general, hevc);
  if (general)
  {
    VAAPI::CDecoder::Register(m_vaapiProxy.get(), hevc);
  }
#endif

  return true;
}

void CWinSystemWaylandEGLContextGL::SetContextSize(CSizeInt size)
{
  CWinSystemWaylandEGLContext::SetContextSize(size);

  // Propagate changed dimensions to render system if necessary
  if (CRenderSystemGL::m_width != size.Width() || CRenderSystemGL::m_height != size.Height())
  {
    CLog::LogF(LOGDEBUG, "Resetting render system to %dx%d", size.Width(), size.Height());
    CRenderSystemGL::ResetRenderSystem(size.Width(), size.Height());
  }
}

void CWinSystemWaylandEGLContextGL::SetVSyncImpl(bool enable)
{
  m_eglContext.SetVSync(enable);
}

void CWinSystemWaylandEGLContextGL::PresentRenderImpl(bool rendered)
{
  PresentFrame(rendered);
}
