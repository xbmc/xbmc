/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "windowing/WinSystemHeadless.h"
#include "WinSystemX11.h"

#include <string>
#include <vector>

class CWinSystemHeadlessX11 : public CWinSystemX11, public CRenderSystemBase
{
public:
  CWinSystemHeadlessX11();
  ~CWinSystemHeadlessX11() override;

  // CWinSystemBase
  CRenderSystemBase *GetRenderSystem() override { return this; }
  bool InitWindowSystem() override;
  bool DestroyWindowSystem() override;
  bool CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) override;
  bool DestroyWindow() override;
  bool ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) override;
  void FinishWindowResize(int newWidth, int newHeight) override;
  bool SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) override;
  void UpdateResolutions() override;
  void ShowOSMouse(bool show) override;

  void NotifyAppActiveChange(bool bActivated) override;
  void NotifyAppFocusChange(bool bGaining) override;

  bool Minimize() override;
  bool Restore() override;
  bool Hide() override;
  bool Show(bool raise = true) override;
  void Register(IDispResource *resource) override;
  void Unregister(IDispResource *resource) override;
  bool HasCalibration(const RESOLUTION_INFO &resInfo) override;
  bool UseLimitedColor() override;

  // winevents override
  //bool MessagePump() override;

  // render
  bool InitRenderSystem() override { return true;} ;
  bool DestroyRenderSystem() override { return true;} ;
  bool ResetRenderSystem(int width, int height) override { return true;} ;

  bool BeginRender() override { return true;} ;
  bool EndRender() override { return true;} ;
  void PresentRender(bool rendered, bool videoLayer) override {} ;
  bool ClearBuffers(UTILS::Color color) override { return true;} ;
  bool IsExtSupported(const char* extension) const override { return false;} ;

  void SetViewPort(const CRect& viewPort) override {} ;
  void GetViewPort(CRect& viewPort) override {} ;

  void SetScissors(const CRect &rect) override {} ;
  void ResetScissors() override {} ;

  void CaptureStateBlock() override {} ;
  void ApplyStateBlock() override {} ;

  void SetCameraPosition(const CPoint &camera, int screenWidth, int screenHeight, float stereoFactor = 0.f) override {} ;
  void ShowSplash(const std::string& message) override;

  // CWinSystemX11
  virtual bool SetWindow(int width, int height, bool fullscreen, const std::string &output, int *winstate = NULL) override {return true;};
  virtual XVisualInfo* GetVisual() override {return (XVisualInfo*) nullptr;};

protected:

private:

};
