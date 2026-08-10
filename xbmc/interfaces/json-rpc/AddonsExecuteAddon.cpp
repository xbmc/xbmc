/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "AddonsExecuteAddon.h"

#include "utils/StringUtils.h"
#include "utils/Variant.h"

#include <string>

namespace JSONRPC
{

namespace
{
std::string BuildArguments(const CVariant& params)
{
  std::string argv;

  if (params.isObject())
  {
    for (CVariant::const_iterator_map it = params.begin_map(); it != params.end_map(); ++it)
    {
      if (it != params.begin_map())
      {
        argv += ",";
      }
      argv += StringUtils::Paramify(it->first + "=" + it->second.asString());
    }
  }
  else if (params.isArray())
  {
    for (CVariant::const_iterator_array it = params.begin_array(); it != params.end_array(); ++it)
    {
      if (it != params.begin_array())
      {
        argv += ",";
      }
      argv += StringUtils::Paramify(it->asString());
    }
  }
  else if (params.isString())
  {
    if (!params.empty())
    {
      argv = StringUtils::Paramify(params.asString());
    }
  }

  return argv;
}
} // unnamed namespace

ParsedExecuteAddon ParseExecuteAddonParams(const CVariant& parameterObject)
{
  const std::string id = parameterObject["addonid"].asString();
  const CVariant& params = parameterObject["params"];

  ParsedExecuteAddon parsed;
  parsed.wait = parameterObject["wait"].asBoolean();

  const std::string argv = BuildArguments(params);
  if (params.empty())
  {
    parsed.command = StringUtils::Format("RunAddon({})", id);
  }
  else
  {
    parsed.command = StringUtils::Format("RunAddon({}, {})", id, argv);
  }

  return parsed;
}

} // namespace JSONRPC
