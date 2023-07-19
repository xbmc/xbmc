/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONVariantParser.h"

#include <rapidjson/reader.h>

class CJSONVariantParserHandler
{
public:
  explicit CJSONVariantParserHandler(CVariant& parsedObject);

  bool Null();
  bool Bool(bool b);
  bool Int(int i);
  bool Uint(unsigned u);
  bool Int64(int64_t i);
  bool Uint64(uint64_t u);
  bool Double(double d);
  bool RawNumber(const char* str, rapidjson::SizeType length, bool copy);
  bool String(const char* str, rapidjson::SizeType length, bool copy);
  bool StartObject();
  bool Key(const char* str, rapidjson::SizeType length, bool copy);
  bool EndObject(rapidjson::SizeType memberCount);
  bool StartArray();
  bool EndArray(rapidjson::SizeType elementCount);

private:
  template <typename... TArgs>
  bool Primitive(TArgs... args)
  {
    PushObject(CVariant(std::forward<TArgs>(args)...));
    PopObject();

    return true;
  }

  void PushObject(CVariant variant);
  void PopObject();

  CVariant& m_parsedObject;
  std::vector<CVariant *> m_parse;
  std::string m_key;
  CVariant m_root;

  enum class PARSE_STATUS
  {
    Variable,
    Array,
    Object
  };
  PARSE_STATUS m_status = PARSE_STATUS::Variable;
};

CJSONVariantParserHandler::CJSONVariantParserHandler(CVariant& parsedObject)
  : m_parsedObject(parsedObject), m_parse(), m_key()
{ }

bool CJSONVariantParserHandler::Null()
{
  PushObject(CVariant::ConstNullVariant);
  PopObject();

  return true;
}

bool CJSONVariantParserHandler::Bool(bool b)
{
  return Primitive(b);
}

bool CJSONVariantParserHandler::Int(int i)
{
  return Primitive(i);
}

bool CJSONVariantParserHandler::Uint(unsigned u)
{
  return Primitive(u);
}

bool CJSONVariantParserHandler::Int64(int64_t i)
{
  return Primitive(i);
}

bool CJSONVariantParserHandler::Uint64(uint64_t u)
{
  return Primitive(u);
}

bool CJSONVariantParserHandler::Double(double d)
{
  return Primitive(d);
}

bool CJSONVariantParserHandler::RawNumber(const char* str, rapidjson::SizeType length, bool copy)
{
  return Primitive(str, length);
}

bool CJSONVariantParserHandler::String(const char* str, rapidjson::SizeType length, bool copy)
{
  return Primitive(str, length);
}

bool CJSONVariantParserHandler::StartObject()
{
  PushObject(CVariant::VariantTypeObject);

  return true;
}

bool CJSONVariantParserHandler::Key(const char* str, rapidjson::SizeType length, bool copy)
{
  m_key = std::string(str, 0, length);

  return true;
}

bool CJSONVariantParserHandler::EndObject(rapidjson::SizeType memberCount)
{
  PopObject();

  return true;
}

bool CJSONVariantParserHandler::StartArray()
{
  PushObject(CVariant::VariantTypeArray);

  return true;
}

bool CJSONVariantParserHandler::EndArray(rapidjson::SizeType elementCount)
{
  PopObject();

  return true;
}

void CJSONVariantParserHandler::PushObject(CVariant variant)
{
  const auto variant_type = variant.type();

  if (m_status == PARSE_STATUS::Object)
  {
    (*m_parse.back())[m_key] = std::move(variant);
    m_parse.push_back(&((*m_parse.back())[m_key]));
  }
  else if (m_status == PARSE_STATUS::Array)
  {
    CVariant *temp = m_parse[m_parse.size() - 1];
    temp->push_back(std::move(variant));
    m_parse.push_back(&(*temp)[temp->size() - 1]);
  }
  else if (m_parse.empty())
  {
    m_root = std::move(variant);
    m_parse.push_back(&m_root);
  }

  if (variant_type == CVariant::VariantTypeObject)
    m_status = PARSE_STATUS::Object;
  else if (variant_type == CVariant::VariantTypeArray)
    m_status = PARSE_STATUS::Array;
  else
    m_status = PARSE_STATUS::Variable;
}

void CJSONVariantParserHandler::PopObject()
{
  assert(!m_parse.empty());
  CVariant* variant = m_parse.back();
  m_parse.pop_back();

  if (!m_parse.empty())
  {
    variant = m_parse[m_parse.size() - 1];
    if (variant->isObject())
      m_status = PARSE_STATUS::Object;
    else if (variant->isArray())
      m_status = PARSE_STATUS::Array;
    else
      m_status = PARSE_STATUS::Variable;
  }
  else
  {
    m_parsedObject = *variant;
    m_status = PARSE_STATUS::Variable;
  }
}

bool CJSONVariantParser::Parse(const char* json, CVariant& data)
{
  if (json == nullptr)
    return false;

  rapidjson::Reader reader;
  rapidjson::StringStream stringStream(json);

  CJSONVariantParserHandler handler(data);
  // use kParseIterativeFlag to eliminate possible stack overflow
  // from json parsing via reentrant calls
  if (reader.Parse<rapidjson::kParseIterativeFlag>(stringStream, handler))
    return true;

  return false;
}

bool CJSONVariantParser::Parse(const std::string& json, CVariant& data)
{
  return Parse(json.c_str(), data);
}
