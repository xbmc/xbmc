/*
 *  Copyright (C) 2026 Team Kodi
 *  SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <memory>

namespace XFILE
{
class IHttpClient;

std::unique_ptr<IHttpClient> CreateHttpClient();
} // namespace XFILE
