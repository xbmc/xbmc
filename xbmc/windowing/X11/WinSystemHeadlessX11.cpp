/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "WinSystemHeadlessX11.h"
#include "utils/log.h"
#include <vector>
#include <string>

//using namespace KODI::MESSAGING;
using namespace KODI::WINDOWING;

CWinSystemHeadlessX11::CWinSystemHeadlessX11() : 
  CWinSystemX11(),
  CRenderSystemBase()
{
}

CWinSystemHeadlessX11::~CWinSystemHeadlessX11() = default;

bool CWinSystemHeadlessX11::InitWindowSystem() {return true;};
bool CWinSystemHeadlessX11::DestroyWindowSystem() {return true;};
bool CWinSystemHeadlessX11::CreateNewWindow(const std::string& name, bool fullScreen, RESOLUTION_INFO& res) {return true;};
bool CWinSystemHeadlessX11::DestroyWindow() {return true;};
bool CWinSystemHeadlessX11::ResizeWindow(int newWidth, int newHeight, int newLeft, int newTop) {return true;};
void CWinSystemHeadlessX11::FinishWindowResize(int newWidth, int newHeight) {};
bool CWinSystemHeadlessX11::SetFullScreen(bool fullScreen, RESOLUTION_INFO& res, bool blankOtherDisplays) {return true;};
void CWinSystemHeadlessX11::UpdateResolutions() {};
void CWinSystemHeadlessX11::ShowOSMouse(bool show) {};

void CWinSystemHeadlessX11::NotifyAppActiveChange(bool bActivated) {};
void CWinSystemHeadlessX11::NotifyAppFocusChange(bool bGaining) {};

bool CWinSystemHeadlessX11::Minimize() {return true;};
bool CWinSystemHeadlessX11::Restore() {return true;};
bool CWinSystemHeadlessX11::Hide() {return true;};
bool CWinSystemHeadlessX11::Show(bool raise) {return true;};
void CWinSystemHeadlessX11::Register(IDispResource *resource) {};
void CWinSystemHeadlessX11::Unregister(IDispResource *resource) {};
bool CWinSystemHeadlessX11::HasCalibration(const RESOLUTION_INFO &resInfo) {return true;};
bool CWinSystemHeadlessX11::UseLimitedColor() {return true;};

void CWinSystemHeadlessX11::ShowSplash(const std::string& message)
{
  CLog::Log(LOGDEBUG, message.c_str());
};
