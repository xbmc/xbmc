/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>

#ifdef _cplusplus
extern "C"
{
#endif

uintptr_t create_dummy_function(const char* strDllName, const char* strFunctionName);
uintptr_t get_win_function_address(const char* strDllName, const char* strFunctionName);

#ifdef _cplusplus
}
#endif

