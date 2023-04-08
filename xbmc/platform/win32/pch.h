/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// The times in comments are how much time was spent parsing
// the header file according to C++ Build Insights in VS2019
#define _CRT_RAND_S
#include <algorithm> // 32 seconds
#include <chrono> // 72 seconds
#include <fstream>
#include <iostream>
#include <map>
#include <mutex> // 19 seconds
#include <string>
#include <vector>

// workaround for broken [[deprecated]] in coverity
#if defined(__COVERITY__)
#undef FMT_DEPRECATED
#define FMT_DEPRECATED
#endif
#include <fmt/core.h>
#include <fmt/format.h> // 53 seconds
#include <intrin.h> // 97 seconds
#include <ppltasks.h> // 87seconds, not included by us
#if !(defined(_WINSOCKAPI_) || defined(_WINSOCK_H))
#include <winsock2.h>
#endif
#include <wrl/client.h>
#include <windows.h>
#include <TCHAR.H>
#include <locale>
#include <comdef.h>
#include <memory>

#ifdef TARGET_WINDOWS_STORE
#include <winrt/Windows.ApplicationModel.h>
#include <winrt/Windows.ApplicationModel.Core.h>
#include <winrt/Windows.ApplicationModel.Activation.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.Storage.h>
#include <winrt/Windows.System.h>
#include <winrt/Windows.System.Threading.h>
#include <winrt/Windows.UI.Core.h>
#include <winrt/Windows.UI.Input.h>
#include <winrt/Windows.UI.ViewManagement.h>
#endif
// anything below here should be headers that very rarely (hopefully never)
// change yet are included almost everywhere.
/* empty */

#include "FileItem.h" //63 seconds
#include "addons/addoninfo/AddonInfo.h" // 62 seconds
