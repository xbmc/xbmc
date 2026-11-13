/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ServiceDescription.h"
#include "interfaces/json-rpc/JSONServiceDescription.h"
#include "utils/JSONVariantParser.h"
#include "utils/JSONVariantWriter.h"
#include "utils/Variant.h"

#include <iterator>
#include <set>
#include <string>

#include <gtest/gtest.h>

namespace JSONRPC
{

inline std::string ToJson(const CVariant& variant)
{
  std::string json;
  CJSONVariantWriter::Write(variant, json, true);
  return json;
}

/*!
 \brief One definition out of a JSONRPC_SERVICE_* table, by name.

 Read rather than restated, so that a declaration which never reaches the schema fails the test.
 */
inline CVariant ShippedDefinition(const char* const entries[],
                                  size_t count,
                                  const std::string& name)
{
  for (size_t index = 0; index < count; ++index)
  {
    CVariant parsed;
    if (!CJSONVariantParser::Parse("{" + std::string(entries[index]) + "}", parsed))
      continue;

    if (parsed.isMember(name))
      return parsed[name];
  }

  ADD_FAILURE() << name << " is not declared in the service description";
  return {};
}

inline CVariant ShippedType(const std::string& name)
{
  return ShippedDefinition(JSONRPC_SERVICE_TYPES, std::size(JSONRPC_SERVICE_TYPES), name);
}

//! \brief A type's entry as JSON text, ready for CJSONServiceDescription::AddType
inline std::string ShippedDefinition(const std::string& type)
{
  CVariant wrapper{CVariant::VariantTypeObject};
  wrapper[type] = ShippedType(type);
  return ToJson(wrapper);
}

inline std::set<std::string> EnumValues(const CVariant& schema)
{
  std::set<std::string> values;
  const CVariant& list{schema["enum"]};
  for (auto value = list.begin_array(); value != list.end_array(); ++value)
    values.insert(value->asString());
  return values;
}

/*!
 \brief Fixture isolating the global schema registry of CJSONServiceDescription.
 */
class JSONServiceDescriptionTestBase : public ::testing::Test
{
public:
  void SetUp() override { CJSONServiceDescription::Cleanup(); }
  void TearDown() override { CJSONServiceDescription::Cleanup(); }
};

} // namespace JSONRPC
