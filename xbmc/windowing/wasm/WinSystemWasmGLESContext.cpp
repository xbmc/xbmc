/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemWasmGLESContext.h"

#include "OSScreenSaverWasm.h"
#include "VideoSyncWasm.h"
#include "WasmClipboard.h"
#include "WasmVsync.h"
#include "WebGLCommit.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecWebCodecs.h"
#include "cores/VideoPlayer/DVDCodecs/Video/DVDVideoCodecWebCodecsBridge.h"
#include "cores/VideoPlayer/Process/wasm/ProcessInfoWasm.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererWasmMedia.h"
#include "cores/VideoPlayer/VideoRenderers/HwDecRender/RendererWebCodecs.h"
#include "cores/VideoPlayer/VideoRenderers/LinuxRendererGLES.h"
#include "cores/VideoPlayer/VideoRenderers/RenderFactory.h"
#include "rendering/gles/ScreenshotSurfaceGLES.h"
#include "settings/DisplaySettings.h"
#include "utils/log.h"
#include "windowing/GraphicContext.h"
#include "windowing/WindowSystemFactory.h"

#include <cmath>
#include <cstdint>
#include <string>

#include <emscripten/em_asm.h>
#include <emscripten/emscripten.h>
#include <emscripten/html5.h>
#include <emscripten/html5_webgl.h>

using namespace KODI::WINDOWING::WASM;

namespace
{
constexpr double VSYNC_WAIT_TIMEOUT_MS = 100.0;
// Kodi commits within the display frame the tick started, the compositor picks
// the canvas up at the next frame boundary and the panel shows it one interval
// after that.
constexpr float DISPLAY_LATENCY_FRAMES = 2.0f;

// The context is created on the browser main thread and proxied to this pthread:
// a thread that never returns to its event loop (Kodi's modal dialog loops) must
// not own per-frame browser resources.
EMSCRIPTEN_WEBGL_CONTEXT_HANDLE CreateProxiedGLContext()
{
  EmscriptenWebGLContextAttributes attrs;
  emscripten_webgl_init_context_attributes(&attrs);
  // The <video> of a media backend shows through where the GUI is transparent. The
  // canvas has a black background otherwise.
  attrs.alpha = EM_TRUE;
  attrs.depth = EM_TRUE;
  attrs.stencil = EM_TRUE;
  attrs.antialias = EM_FALSE;
  attrs.premultipliedAlpha = EM_TRUE;
  attrs.preserveDrawingBuffer = EM_FALSE;
  attrs.powerPreference = EM_WEBGL_POWER_PREFERENCE_HIGH_PERFORMANCE;
  attrs.failIfMajorPerformanceCaveat = EM_FALSE;
  attrs.majorVersion = 2;
  attrs.minorVersion = 0;
  attrs.enableExtensionsByDefault = EM_TRUE;
  attrs.explicitSwapControl = EM_TRUE;
  attrs.renderViaOffscreenBackBuffer = EM_TRUE;
  attrs.proxyContextToMainThread = EMSCRIPTEN_WEBGL_CONTEXT_PROXY_ALWAYS;
  return emscripten_webgl_create_context("#canvas", &attrs);
}

void NotifyFirstFramePresented()
{
  MAIN_THREAD_ASYNC_EM_ASM({
    if (typeof Module.onKodiFirstFramePresented === 'function')
      Module.onKodiFirstFramePresented();
  });
}

// The canvas' size in device pixels, so the GUI renders as sharp as the display allows.
void GetCanvasSize(int* width, int* height)
{
  double w = 0;
  double h = 0;
  emscripten_get_element_css_size("#canvas", &w, &h);
  const double ratio = CWinEventsWasm::DevicePixelRatio();
  *width = static_cast<int>(std::lround(w * ratio));
  *height = static_cast<int>(std::lround(h * ratio));
  if (*width <= 0)
    *width = 1280;
  if (*height <= 0)
    *height = 720;
}
} // namespace

void CWinSystemWasmGLESContext::Register()
{
  KODI::WINDOWING::CWindowSystemFactory::RegisterWindowSystem(CreateWinSystem);
}

std::unique_ptr<CWinSystemBase> CWinSystemWasmGLESContext::CreateWinSystem()
{
  return std::make_unique<CWinSystemWasmGLESContext>();
}

CWinSystemWasmGLESContext::CWinSystemWasmGLESContext()
{
  m_winEvents = std::make_unique<CWinEventsWasm>();
}

CWinSystemWasmGLESContext::~CWinSystemWasmGLESContext()
{
  if (m_webglContext > 0)
    emscripten_webgl_destroy_context(m_webglContext);
}

void CWinSystemWasmGLESContext::SetCanvasSize(int width, int height)
{
  // Pointer positions arrive in CSS pixels: scale them by the drawing buffer
  // pixels each CSS pixel of the canvas now holds.
  double cssWidth = 0;
  double cssHeight = 0;
  emscripten_get_element_css_size("#canvas", &cssWidth, &cssHeight);
  emscripten_set_canvas_element_size("#canvas", width, height);
  WinEvents().SetPointerScale(cssWidth > 0 ? width / cssWidth : 1.0);
}

bool CWinSystemWasmGLESContext::InitWindowSystem()
{
  VIDEOPLAYER::CRendererFactory::ClearRenderer();
  CProcessInfoWasm::Register();
  CDVDVideoCodecWebCodecs::Register();
  CLinuxRendererGLES::Register();
  CRendererWebCodecs::Register();
  CRendererWasmMedia::Register();
  CScreenshotSurfaceGLES::Register();

  int initW = 0;
  int initH = 0;
  GetCanvasSize(&initW, &initH);
  SetCanvasSize(initW, initH);

  m_webglContext = CreateProxiedGLContext();
  if (m_webglContext <= 0)
  {
    CLog::Log(LOGERROR, "WASM: failed to create proxied WebGL2 context ({})", m_webglContext);
    return false;
  }

  const EMSCRIPTEN_RESULT r = emscripten_webgl_make_context_current(m_webglContext);
  if (r != EMSCRIPTEN_RESULT_SUCCESS)
  {
    CLog::Log(LOGERROR, "WASM: emscripten_webgl_make_context_current failed ({})", r);
    return false;
  }

  // Must run before anything is queued on the context; the decoder reads the cached result.
  webcodecs_probe_texture_upload();

  VSYNC::InstallPump();

  CLog::Log(LOGINFO, "WASM: using main-thread WebGL2 context proxied to the Kodi thread ({}x{})",
            initW, initH);
  return CWinSystemBase::InitWindowSystem();
}

bool CWinSystemWasmGLESContext::DestroyWindowSystem()
{
  if (m_webglContext > 0)
  {
    emscripten_webgl_destroy_context(m_webglContext);
    m_webglContext = 0;
  }
  return CWinSystemBase::DestroyWindowSystem();
}

bool CWinSystemWasmGLESContext::CreateNewWindow(const std::string& name,
                                                bool fullScreen,
                                                RESOLUTION_INFO& res)
{
  // Render at the viewport's size in device pixels; pointer positions are scaled to match.
  int w = 0;
  int h = 0;
  GetCanvasSize(&w, &h);

  UpdateDesktopResolution(res, "Browser", w, h, static_cast<float>(VSYNC::RefreshRate()), 0);
  res.bFullScreen = fullScreen;

  SetCanvasSize(w, h);

  if (emscripten_webgl_get_current_context() != m_webglContext)
    emscripten_webgl_make_context_current(m_webglContext);

  m_nWidth = static_cast<unsigned int>(w);
  m_nHeight = static_cast<unsigned int>(h);
  m_bWindowCreated = true;

  CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP) = res;
  SetWindowResolution(w, h);
  CRenderSystemGLES::ResetRenderSystem(w, h);

  CLog::Log(LOGINFO, "WASM: created GLES window {}x{} ({})", w, h, name);
  return true;
}

bool CWinSystemWasmGLESContext::DestroyWindow()
{
  m_bWindowCreated = false;
  return true;
}

bool CWinSystemWasmGLESContext::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop)
{
  SetCanvasSize(newWidth, newHeight);
  SetWindowResolution(newWidth, newHeight);
  CRenderSystemGLES::ResetRenderSystem(newWidth, newHeight);
  m_nWidth = static_cast<unsigned int>(newWidth);
  m_nHeight = static_cast<unsigned int>(newHeight);
  return true;
}

bool CWinSystemWasmGLESContext::SetFullScreen(bool fullScreen,
                                              RESOLUTION_INFO& res,
                                              bool blankOtherDisplays)
{
  return CreateNewWindow("", fullScreen, res);
}

void CWinSystemWasmGLESContext::ForceFullScreen(const RESOLUTION_INFO& resInfo)
{
  // AppInboundProtocol calls us with the cached RES_DESKTOP on browser resize,
  // so resInfo is stale.  Re-query the live canvas size and update
  // RES_DESKTOP + the cached mouse resolution so mouse events stay mapped to
  // Kodi's GUI coordinate space.
  int w = 0;
  int h = 0;
  GetCanvasSize(&w, &h);
  if (w <= 0 || h <= 0)
    return;

  RESOLUTION_INFO& desktop = CDisplaySettings::GetInstance().GetResolutionInfo(RES_DESKTOP);
  const std::string output = desktop.strOutput.empty() ? "Browser" : desktop.strOutput;
  const uint32_t flags = desktop.dwFlags;
  UpdateDesktopResolution(desktop, output, w, h, static_cast<float>(VSYNC::RefreshRate()), flags);
  GetGfxContext().ResetOverscan(desktop);

  ResizeWindow(w, h, 0, 0);

  GetGfxContext().ApplyModeChange(RES_DESKTOP);
}

bool CWinSystemWasmGLESContext::MessagePump()
{
  if (m_winEvents)
    return m_winEvents->MessagePump();
  return false;
}

void CWinSystemWasmGLESContext::SetVSyncImpl(bool enable)
{
}

void CWinSystemWasmGLESContext::PresentRenderImpl(bool rendered)
{
  if (rendered)
    m_framePending = true;
  if (!m_framePending || m_webglContext <= 0)
    return;

  // The timeout keeps Kodi running when rAF is throttled (hidden tab). The
  // frame stays pending since the GUI only redraws what changes after it.
  const uint32_t next = VSYNC::WaitForTick(m_lastVsyncSeen, VSYNC_WAIT_TIMEOUT_MS);
  if (next == m_lastVsyncSeen)
    return;
  m_lastVsyncSeen = next;
  m_framePending = false;

  // Synchronous: the main thread runs every queued GL call, then blits the
  // offscreen framebuffer to the canvas.
  if (wasm_webgl_commit_frame() == 0)
  {
    CLog::Log(LOGWARNING, "WASM: no WebGL context to commit the frame to");
    return;
  }
  if (!m_firstFramePresented)
  {
    m_firstFramePresented = true;
    NotifyFirstFramePresented();
  }
}

int CWinSystemWasmGLESContext::GetBufferAge()
{
  return 1;
}

float CWinSystemWasmGLESContext::GetDisplayLatency()
{
  return DISPLAY_LATENCY_FRAMES * 1000.0f / static_cast<float>(VSYNC::RefreshRate());
}

std::unique_ptr<CVideoSync> CWinSystemWasmGLESContext::GetVideoSync(CVideoReferenceClock* clock)
{
  return std::make_unique<CVideoSyncWasm>(clock);
}

std::string CWinSystemWasmGLESContext::GetClipboardText()
{
  return WASM_CLIPBOARD::ConsumePendingPasteText();
}

std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> CWinSystemWasmGLESContext::GetOSScreenSaverImpl()
{
  return std::make_unique<COSScreenSaverWasm>();
}
