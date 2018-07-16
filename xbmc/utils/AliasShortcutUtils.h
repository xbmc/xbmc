/*
 *  Copyright (C) 2009-2013 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

bool IsAliasShortcut(const std::string& path, bool isdirectory);
void TranslateAliasShortcut(std::string &path);
