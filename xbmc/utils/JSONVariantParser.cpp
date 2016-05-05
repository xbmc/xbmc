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

yajl_callbacks CJSONVariantParser::callbacks = {
  CJSONVariantParser::ParseNull,
  CJSONVariantParser::ParseBoolean,
  CJSONVariantParser::ParseInteger,
  CJSONVariantParser::ParseDouble,
  NULL,
  CJSONVariantParser::ParseString,
  CJSONVariantParser::ParseMapStart,
  CJSONVariantParser::ParseMapKey,
  CJSONVariantParser::ParseMapEnd,
  CJSONVariantParser::ParseArrayStart,
  CJSONVariantParser::ParseArrayEnd
};

CJSONVariantParser::CJSONVariantParser(IParseCallback *callback)
{
  m_callback = callback;

  m_handler = yajl_alloc(&callbacks, NULL, this);

  yajl_config(m_handler, yajl_allow_comments, 1);
  yajl_config(m_handler, yajl_dont_validate_strings, 0);

  m_status = ParseVariable;
}

CJSONVariantParser::~CJSONVariantParser()
{
  yajl_complete_parse(m_handler);
  yajl_free(m_handler);
}

void CJSONVariantParser::push_buffer(const unsigned char *buffer, unsigned int length)
{
  yajl_parse(m_handler, buffer, length);
}

CVariant CJSONVariantParser::Parse(const std::string& json)
{
  return Parse(reinterpret_cast<const unsigned char*>(json.c_str()), json.length());
}

CVariant CJSONVariantParser::Parse(const unsigned char *json, unsigned int length)
{
  CSimpleParseCallback callback;
  CJSONVariantParser parser(&callback);

  parser.push_buffer(json, length);

  return callback.GetOutput();
}

int CJSONVariantParser::ParseNull(void * ctx)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant::VariantTypeNull);
  parser->PopObject();

  return 1;
}

int CJSONVariantParser::ParseBoolean(void * ctx, int boolean)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant(boolean != 0));
  parser->PopObject();

  return 1;
}

int CJSONVariantParser::ParseInteger(void * ctx, long long integerVal)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant((int64_t)integerVal));
  parser->PopObject();

  return 1;
}

int CJSONVariantParser::ParseDouble(void * ctx, double doubleVal)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant((float)doubleVal));
  parser->PopObject();

  return 1;
}

int CJSONVariantParser::ParseString(void * ctx, const unsigned char * stringVal, size_t stringLen)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant((const char *)stringVal, stringLen));
  parser->PopObject();

  return 1;
}

int CJSONVariantParser::ParseMapStart(void * ctx)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant::VariantTypeObject);

  return 1;
}

int CJSONVariantParser::ParseMapKey(void * ctx, const unsigned char * stringVal, size_t stringLen)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->m_key = std::string((const char *)stringVal, 0, stringLen);

  return 1;
}

int CJSONVariantParser::ParseMapEnd(void * ctx)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PopObject();

  return 1;
}

int CJSONVariantParser::ParseArrayStart(void * ctx)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PushObject(CVariant::VariantTypeArray);

  return 1;
}

int CJSONVariantParser::ParseArrayEnd(void * ctx)
{
  CJSONVariantParser *parser = (CJSONVariantParser *)ctx;

  parser->PopObject();

  return 1;
}

void CJSONVariantParser::PushObject(CVariant variant)
{
  if (m_status == ParseObject)
  {
    (*m_parse[m_parse.size() - 1])[m_key] = variant;
    m_parse.push_back(&(*m_parse[m_parse.size() - 1])[m_key]);
  }
  else if (m_status == ParseArray)
  {
    CVariant *temp = m_parse[m_parse.size() - 1];
    temp->push_back(variant);
    m_parse.push_back(&(*temp)[temp->size() - 1]);
  }
  else if (m_parse.empty())
  {
    m_parse.push_back(new CVariant(variant));
  }

  if (variant.isObject())
    m_status = ParseObject;
  else if (variant.isArray())
    m_status = ParseArray;
  else
    m_status = ParseVariable;
}

void CJSONVariantParser::PopObject()
{
  CVariant *variant = m_parse[m_parse.size() - 1];
  m_parse.pop_back();

  if (m_parse.size())
  {
    variant = m_parse[m_parse.size() - 1];
    if (variant->isObject())
      m_status = ParseObject;
    else if (variant->isArray())
      m_status = ParseArray;
    else
      m_status = ParseVariable;
  }
  else if (m_callback)
  {
    m_callback->onParsed(variant);
    delete variant;

    m_parse.clear();
    m_status = ParseVariable;
  }
}
