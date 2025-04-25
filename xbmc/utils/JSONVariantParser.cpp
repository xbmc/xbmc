/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONVariantParser.h"

#include <nlohmann/json.hpp>

class CJSONVariantParserHandler : public nlohmann::json::json_sax_t
{
public:
  explicit CJSONVariantParserHandler(CVariant& parsedObject);

  bool null() override;
  bool binary(binary_t& b) override;
  bool boolean(bool b) override;
  bool number_integer(number_integer_t i) override;
  bool number_unsigned(number_unsigned_t u) override;
  bool number_float(number_float_t d, const string_t& s) override;
  bool string(std::string& str) override;
  bool start_object(std::size_t elements) override;
  bool key(string_t& str) override;
  bool end_object() override;
  bool start_array(std::size_t elements) override;
  bool end_array() override;
  bool parse_error(std::size_t position,
                   const std::string& last_token,
                   const nlohmann::json::exception& ex) override;

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

bool CJSONVariantParserHandler::null()
{
  PushObject(CVariant::ConstNullVariant);
  PopObject();

  return true;
}

bool CJSONVariantParserHandler::boolean(bool b)
{
  return Primitive(b);
}

bool CJSONVariantParserHandler::number_integer(number_integer_t i)
{
  return Primitive(i);
}

bool CJSONVariantParserHandler::number_unsigned(number_unsigned_t u)
{
  return Primitive(u);
}

bool CJSONVariantParserHandler::number_float(number_float_t d, const string_t& s)
{
  return Primitive(d);
}

bool CJSONVariantParserHandler::string(std::string& str)
{
  return Primitive(str);
}

bool CJSONVariantParserHandler::binary(binary_t& b)
{
  return true;
}

bool CJSONVariantParserHandler::start_object(std::size_t elements)
{
  PushObject(CVariant::VariantTypeObject);

  return true;
}

bool CJSONVariantParserHandler::key(std::string& str)
{
  m_key = str;

  return true;
}

bool CJSONVariantParserHandler::end_object()
{
  PopObject();

  return true;
}

bool CJSONVariantParserHandler::start_array(std::size_t elements)
{
  PushObject(CVariant::VariantTypeArray);

  return true;
}

bool CJSONVariantParserHandler::end_array()
{
  PopObject();

  return true;
}

bool CJSONVariantParserHandler::parse_error(std::size_t position,
                                            const std::string& last_token,
                                            const nlohmann::json::exception& ex)
{
  return false;
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

  CJSONVariantParserHandler handler(data);
  return nlohmann::json::sax_parse(json, &handler);
}

bool CJSONVariantParser::Parse(const std::string& json, CVariant& data)
{
  return Parse(json.c_str(), data);
}
