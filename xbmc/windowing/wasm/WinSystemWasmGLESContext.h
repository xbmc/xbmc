/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include "WinEventsWasm.h"
#include "rendering/gles/RenderSystemGLES.h"
#include "windowing/WinSystem.h"

#include <cstdint>
#include <memory>
#include <string>

#include <emscripten/html5.h>

class CWinSystemWasmGLESContext : public CWinSystemBase, public CRenderSystemGLES
{
public:
  CWinSystemWasmGLESContext();
  ~CWinSystemWasmGLESContext() override;

  static void Register();
  static std::unique_ptr<CWinSystemBase> CreateWinSystem();

  CRenderSystemBase* GetRenderSystem() override { return this; }

  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;

  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  bool DestroyWindow() override;

  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  void ForceFullScreen(const RESOLUTION_INFO& resInfo) override;

  bool MessagePump() override;
  bool HasCursor() override { return false; }

  void Register(IDispResource* resource) override {}
  void Unregister(IDispResource* resource) override {}

  /*!
   * \brief WebGL cannot report a buffer age, but the offscreen framebuffer Kodi
   * renders into survives emscripten_webgl_commit_frame(), which only blits it to
   * the canvas. The previous frame is therefore always intact and the GUI needs
   * to redraw dirty regions only.
   */
  int GetBufferAge() override;

  float GetDisplayLatency() override;
  std::unique_ptr<CVideoSync> GetVideoSync(CVideoReferenceClock* clock) override;

  std::string GetClipboardText() override;

protected:
  std::unique_ptr<KODI::WINDOWING::IOSScreenSaver> GetOSScreenSaverImpl() override;
  void SetVSyncImpl(bool enable) override;
  void PresentRenderImpl(bool rendered) override;

private:
  EMSCRIPTEN_WEBGL_CONTEXT_HANDLE m_webglContext{0};
  uint32_t m_lastVsyncSeen{0};
  bool m_firstFramePresented{false};
};
