/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONVariantWriter.h"

#include "utils/Variant.h"

#include <nlohmann/json.hpp>

bool InternalWrite(nlohmann::json& writer, const CVariant& value)
{
  switch (value.type())
  {
  case CVariant::VariantTypeInteger:
    writer = value.asInteger();
    return true;
  case CVariant::VariantTypeUnsignedInteger:
    writer = value.asUnsignedInteger();
    return true;
  case CVariant::VariantTypeDouble:
    writer = value.asDouble();
    return true;
  case CVariant::VariantTypeBoolean:
    writer = value.asBoolean();
    return true;
  case CVariant::VariantTypeString:
    writer = std::string(value.c_str(), value.size());
    return true;
  case CVariant::VariantTypeArray:
    writer = nlohmann::json::array();

    for (CVariant::const_iterator_array itr = value.begin_array(); itr != value.end_array(); ++itr)
    {
      auto o = nlohmann::json::object();
      if (!InternalWrite(o, *itr))
        return false;
      writer.push_back(o);
    }

    return true;
  case CVariant::VariantTypeObject:
    writer = nlohmann::json::object();

    for (CVariant::const_iterator_map itr = value.begin_map(); itr != value.end_map(); ++itr)
    {
      writer[itr->first] = nlohmann::json::object();
      if (!InternalWrite(writer[itr->first], itr->second))
        return false;
    }

    return true;

  case CVariant::VariantTypeConstNull:
  case CVariant::VariantTypeNull:
  default:
    writer = nullptr;
    return true;
  }

  return false;
}

bool CJSONVariantWriter::Write(const CVariant &value, std::string& output, bool compact)
{
  try
  {
    nlohmann::json json;
    if (!InternalWrite(json, value))
      return false;

    if (compact)
    {
      output = json.dump();
    }
    else
    {
      output = json.dump(1, '\t');
    }
  }
  catch (nlohmann::json::exception& e)
  {
    return false;
  }

  return true;
}
