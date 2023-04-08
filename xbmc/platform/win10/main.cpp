/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "pch.h"

#include "platform/win10/Win10App.h"

using namespace KODI::PLATFORM::WINDOWS10;

int __stdcall WinMain(HINSTANCE, HINSTANCE, PCSTR, int)
{
  winrt::init_apartment();
  winrt::Windows::ApplicationModel::Core::CoreApplication::Run(winrt::make<App>());
}
