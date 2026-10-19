/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <string_view>

class CURL;

/*! \brief Kodi credential removal and redaction, including resource wrappers. */
class CURIRedactor final
{
public:
  static std::string GetRedacted(std::string_view path);
  static std::string GetWithoutUserDetails(const CURL& url, bool redact);
};
