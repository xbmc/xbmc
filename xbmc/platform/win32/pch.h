/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#define _CRT_RAND_S
#include <vector>
#include <map>
#include <string>
#include <iostream>
#include <fstream>
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
#include "winrt/Windows.ApplicationModel.h"
#include "winrt/Windows.ApplicationModel.Core.h"
#include "winrt/Windows.ApplicationModel.Activation.h"
#include "winrt/Windows.Foundation.h"
#include "winrt/Windows.Graphics.Display.h"
#include "winrt/Windows.System.h"
#include "winrt/Windows.System.Threading.h"
#include "winrt/Windows.UI.Core.h"
#include "winrt/Windows.UI.Input.h"
#include "winrt/Windows.UI.ViewManagement.h"
WINRT_WARNING_PUSH
#endif
// anything below here should be headers that very rarely (hopefully never)
// change yet are included almost everywhere.
/* empty */
