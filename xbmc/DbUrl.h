#pragma once
/*
 *      Copyright (C) 2012-2013 Team XBMC
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

#include "utils/UrlOptions.h"
#include "URL.h"

#include <string>

class CVariant;

class CDbUrl : CUrlOptions
{
public:
  CDbUrl();
  virtual ~CDbUrl();

  bool IsValid() const { return m_valid; }
  void Reset();

  std::string ToString() const;
  bool FromString(const std::string &dbUrl);

  const std::string& GetType() const { return m_type; }
  void AppendPath(const std::string &subPath);

  using CUrlOptions::HasOption;
  using CUrlOptions::GetOptions;
  using CUrlOptions::GetOptionsString;

  void AddOption(const std::string &key, const char *value) override;
  void AddOption(const std::string &key, const std::string &value) override;
  void AddOption(const std::string &key, int value) override;
  void AddOption(const std::string &key, float value) override;
  void AddOption(const std::string &key, double value) override;
  void AddOption(const std::string &key, bool value) override;
  void AddOptions(const std::string &options) override;
  void RemoveOption(const std::string &key) override;

protected:
  virtual bool parse() = 0;
  virtual bool validateOption(const std::string &key, const CVariant &value);
  
  CURL m_url;
  std::string m_type;

private:
  void updateOptions();

  bool m_valid;
};
