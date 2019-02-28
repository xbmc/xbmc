/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemHeadless.h"

#include "utils/log.h"

#include <vector>
#include <string>

//using namespace KODI::MESSAGING;
using namespace KODI::WINDOWING;

#define EGL_NO_CONFIG (EGLConfig)0

CWinSystemHeadless::CWinSystemHeadless() : CWinSystemBase(), CRenderSystemBase()
{
}

CWinSystemHeadless::~CWinSystemHeadless() = default;

// bool CWinSystemHeadless::MessagePump()
// {
//   return m_winEvents->MessagePump();
// }

bool CWinSystemHeadless::InitWindowSystem() {return true;};
bool CWinSystemHeadless::DestroyWindowSystem() {return true;};
bool CWinSystemHeadless::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) {return true;};
bool CWinSystemHeadless::DestroyWindow() {return true;};
bool CWinSystemHeadless::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) {return true;};
void CWinSystemHeadless::FinishWindowResize(int newWidth, int newHeight) {};
bool CWinSystemHeadless::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) {return true;};
void CWinSystemHeadless::UpdateResolutions() {};
void CWinSystemHeadless::ShowOSMouse(bool show) {};

void CWinSystemHeadless::NotifyAppActiveChange(bool bActivated) {};
void CWinSystemHeadless::NotifyAppFocusChange(bool bGaining) {};

bool CWinSystemHeadless::Minimize() {return true;};
bool CWinSystemHeadless::Restore() {return true;};
bool CWinSystemHeadless::Hide() {return true;};
bool CWinSystemHeadless::Show(bool raise) {return true;};
void CWinSystemHeadless::Register(IDispResource *resource) {};
void CWinSystemHeadless::Unregister(IDispResource *resource) {};
bool CWinSystemHeadless::HasCalibration(const RESOLUTION_INFO &resInfo) {return true;};
bool CWinSystemHeadless::UseLimitedColor() {return true;};

void CWinSystemHeadless::ShowSplash(const std::string& message)
{
  CLog::Log(LOGDEBUG, message.c_str());
};
