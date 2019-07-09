/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONVariantWriter.h"

#include "utils/Variant.h"

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

template<class TWriter>
bool InternalWrite(TWriter& writer, const CVariant &value)
{
  switch (value.type())
  {
  case CVariant::VariantTypeInteger:
    return writer.Int64(value.asInteger());

  case CVariant::VariantTypeUnsignedInteger:
    return writer.Uint64(value.asUnsignedInteger());

  case CVariant::VariantTypeDouble:
    return writer.Double(value.asDouble());

  case CVariant::VariantTypeBoolean:
    return writer.Bool(value.asBoolean());

  case CVariant::VariantTypeString:
    return writer.String(value.c_str(), value.size());

  case CVariant::VariantTypeArray:
    if (!writer.StartArray())
      return false;

    for (CVariant::const_iterator_array itr = value.begin_array(); itr != value.end_array(); ++itr)
    {
      if (!InternalWrite(writer, *itr))
        return false;
    }

    return writer.EndArray(value.size());

  case CVariant::VariantTypeObject:
    if (!writer.StartObject())
      return false;

    for (CVariant::const_iterator_map itr = value.begin_map(); itr != value.end_map(); ++itr)
    {
      if (!writer.Key(itr->first.c_str()) ||
        !InternalWrite(writer, itr->second))
        return false;
    }

    return writer.EndObject(value.size());

  case CVariant::VariantTypeConstNull:
  case CVariant::VariantTypeNull:
  default:
    return writer.Null();
  }

  return false;
}

bool CJSONVariantWriter::Write(const CVariant &value, std::string& output, bool compact)
{
  rapidjson::StringBuffer stringBuffer;
  if (compact)
  {
    rapidjson::Writer<rapidjson::StringBuffer> writer(stringBuffer);

    if (!InternalWrite(writer, value) || !writer.IsComplete())
      return false;
  }
  else
  {
    rapidjson::PrettyWriter<rapidjson::StringBuffer> writer(stringBuffer);
    writer.SetIndent('\t', 1);

    if (!InternalWrite(writer, value) || !writer.IsComplete())
      return false;
  }

  output = stringBuffer.GetString();
  return true;
}
