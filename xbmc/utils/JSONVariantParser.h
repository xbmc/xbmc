#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include <string>
#include <vector>

#include "utils/Variant.h"

#include <yajl/yajl_parse.h>

class IParseCallback
{
public:
  virtual ~IParseCallback() { }

  virtual void onParsed(CVariant *variant) = 0;
};

class CSimpleParseCallback : public IParseCallback
{
public:
  virtual void onParsed(CVariant *variant) { m_parsed = *variant; }
  CVariant &GetOutput() { return m_parsed; }

private:
  CVariant m_parsed;
};

class CJSONVariantParser
{
public:
  CJSONVariantParser(IParseCallback *callback);
  ~CJSONVariantParser();

  void push_buffer(const unsigned char *buffer, unsigned int length);

  static CVariant Parse(const unsigned char *json, unsigned int length);

  static CVariant Parse(const std::string& json);

private:
  static int ParseNull(void * ctx);
  static int ParseBoolean(void * ctx, int boolean);
  static int ParseInteger(void * ctx, long long integerVal);
  static int ParseDouble(void * ctx, double doubleVal);
  static int ParseString(void * ctx, const unsigned char * stringVal, size_t stringLen);
  static int ParseMapStart(void * ctx);
  static int ParseMapKey(void * ctx, const unsigned char * stringVal, size_t stringLen);
  static int ParseMapEnd(void * ctx);
  static int ParseArrayStart(void * ctx);
  static int ParseArrayEnd(void * ctx);

  void PushObject(CVariant variant);
  void PopObject();

  static yajl_callbacks callbacks;

  IParseCallback *m_callback;
  yajl_handle m_handler;

  CVariant m_parsedObject;
  std::vector<CVariant *> m_parse;
  std::string m_key;

  enum PARSE_STATUS
  {
    ParseArray = 1,
    ParseObject = 2,
    ParseVariable = 0
  };
  PARSE_STATUS m_status;
};
