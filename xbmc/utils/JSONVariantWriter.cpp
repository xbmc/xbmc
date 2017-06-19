/*
 *      Copyright (C) 2015 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "JSONVariantWriter.h"

#include <rapidjson/prettywriter.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/writer.h>

#include "utils/Variant.h"

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
