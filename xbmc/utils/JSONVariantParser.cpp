/*
 *      Copyright (C) 2005-2015 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
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

  enum class PARSE_STATUS
  {
    Variable,
    Array,
    Object
  };
  PARSE_STATUS m_status;
};

CJSONVariantParserHandler::CJSONVariantParserHandler(CVariant& parsedObject)
  : m_parsedObject(parsedObject),
    m_parse(),
    m_key(),
    m_status(PARSE_STATUS::Variable)
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
  if (m_status == PARSE_STATUS::Object)
  {
    (*m_parse[m_parse.size() - 1])[m_key] = variant;
    m_parse.push_back(&(*m_parse[m_parse.size() - 1])[m_key]);
  }
  else if (m_status == PARSE_STATUS::Array)
  {
    CVariant *temp = m_parse[m_parse.size() - 1];
    temp->push_back(variant);
    m_parse.push_back(&(*temp)[temp->size() - 1]);
  }
  else if (m_parse.empty())
    m_parse.push_back(new CVariant(variant));

  if (variant.isObject())
    m_status = PARSE_STATUS::Object;
  else if (variant.isArray())
    m_status = PARSE_STATUS::Array;
  else
    m_status = PARSE_STATUS::Variable;
}

void CJSONVariantParserHandler::PopObject()
{
  CVariant *variant = m_parse[m_parse.size() - 1];
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
    delete variant;

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
  if (reader.Parse(stringStream, handler))
    return true;

  return false;
}

bool CJSONVariantParser::Parse(const std::string& json, CVariant& data)
{
  return Parse(json.c_str(), data);
}
