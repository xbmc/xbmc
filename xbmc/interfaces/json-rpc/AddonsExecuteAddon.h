/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

class CVariant;

namespace JSONRPC
{

struct ParsedExecuteAddon
{
  std::string command;
  bool wait{false};
};

/*! \brief Build the RunAddon builtin that Addons.ExecuteAddon executes.

 Every parameter is quoted with StringUtils::Paramify: CUtil::SplitParams ends a parameter
 at any comma not inside quotes. The object form quotes the whole "key=value" pair, so a
 comma in either half is covered.

 The add-on id is not validated here; the caller resolves it against the add-on manager first.

 \param parameterObject the JSON-RPC parameters, with schema defaults already applied
 \return the builtin to execute, and whether the caller asked to wait for it
 */
ParsedExecuteAddon ParseExecuteAddonParams(const CVariant& parameterObject);

} // namespace JSONRPC
