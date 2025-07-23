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

void InternalWrite(nlohmann::json& writer, const CVariant& value)
{
  switch (value.type())
  {
  case CVariant::VariantTypeInteger:
    writer = value.asInteger();
    break;
  case CVariant::VariantTypeUnsignedInteger:
    writer = value.asUnsignedInteger();
    break;
  case CVariant::VariantTypeDouble:
    writer = value.asDouble();
    break;
  case CVariant::VariantTypeBoolean:
    writer = value.asBoolean();
    break;
  case CVariant::VariantTypeString:
    writer = std::string(value.c_str(), value.size());
    break;
  case CVariant::VariantTypeArray:
    writer = nlohmann::json::array();

    for (CVariant::const_iterator_array itr = value.begin_array(); itr != value.end_array(); ++itr)
    {
      nlohmann::json& o = writer.emplace_back();
      InternalWrite(o, *itr);
    }

    break;
  case CVariant::VariantTypeObject:
    writer = nlohmann::json::object();

    for (CVariant::const_iterator_map itr = value.begin_map(); itr != value.end_map(); ++itr)
    {
      nlohmann::json& o = writer[itr->first];
      InternalWrite(o, itr->second);
    }

    break;

  case CVariant::VariantTypeConstNull:
  case CVariant::VariantTypeNull:
  default:
    writer = nullptr;
    break;
  }
}

bool CJSONVariantWriter::Write(const CVariant &value, std::string& output, bool compact)
{
  try
  {
    nlohmann::json json;
    InternalWrite(json, value);

    if (compact)
    {
      output = json.dump();
    }
    else
    {
      output = json.dump(1, '\t');
    }
  }
  catch (nlohmann::json::exception&)
  {
    return false;
  }

  return true;
}
