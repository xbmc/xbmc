/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <set>
#include <string>

class CVariant;

namespace JSONRPC
{

//! A Video.Cast array. The EPG format carries names only, so every role is empty and the
//! order is the name's position. Empty when there are no names.
CVariant TranslateEpgCast(const std::string& cast);

//! Everything PVR.Fields.Broadcast declares, so a field added there reaches the broadcast
//! nested inside a channel. Empty until the service description has been parsed.
std::set<std::string> BroadcastFields();

} // namespace JSONRPC
