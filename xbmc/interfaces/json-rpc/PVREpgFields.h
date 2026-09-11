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

/*! \brief Translate an EPG tag's cast string into a Video.Cast array.

 The EPG format carries names only, so the role is empty and the order is the position in
 the string.

 \param cast the cast as CPVREpgInfoTag::Serialize writes it
 \return a Video.Cast array, empty when there are no names
 */
CVariant TranslateEpgCast(const std::string& cast);

/*! \brief The fields a broadcast nested inside a channel answers with.

 Everything PVR.Fields.Broadcast declares, read from the service description so a field
 added there reaches the nested copy.

 \return the field names, empty when the service description has not been parsed
 */
std::set<std::string> BroadcastFields();

} // namespace JSONRPC
