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

#include "utils/Variant.h"

#include <map>
#include <string>

class CUrlOptions
{
public:
  typedef std::map<std::string, CVariant> UrlOptions;

  CUrlOptions();
  CUrlOptions(const std::string &options, const char *strLead = "");
  virtual ~CUrlOptions();

  void Clear() { m_options.clear(); m_strLead.clear(); }

  const UrlOptions& GetOptions() const { return m_options; }
  std::string GetOptionsString(bool withLeadingSeperator = false) const;

  virtual void AddOption(const std::string &key, const char *value);
  virtual void AddOption(const std::string &key, const std::string &value);
  virtual void AddOption(const std::string &key, int value);
  virtual void AddOption(const std::string &key, float value);
  virtual void AddOption(const std::string &key, double value);
  virtual void AddOption(const std::string &key, bool value);
  virtual void AddOptions(const std::string &options);
  virtual void AddOptions(const CUrlOptions &options);
  virtual void RemoveOption(const std::string &key);

  bool HasOption(const std::string &key) const;
  bool GetOption(const std::string &key, CVariant &value) const;

protected:
  UrlOptions m_options;
  std::string m_strLead;
};
