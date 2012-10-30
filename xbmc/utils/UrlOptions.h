#pragma once
/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <map>
#include <string>

#include "utils/Variant.h"

class CUrlOptions
{
public:
  typedef std::map<std::string, CVariant> UrlOptions;

  CUrlOptions();
  CUrlOptions(const std::string &options);
  virtual ~CUrlOptions();

  virtual void Clear() { m_options.clear(); }

  virtual const UrlOptions& GetOptions() const { return m_options; }
  virtual std::string GetOptionsString(bool withLeadingSeperator = false) const;

  virtual void AddOption(const std::string &key, const char *value);
  virtual void AddOption(const std::string &key, const std::string &value);
  virtual void AddOption(const std::string &key, int value);
  virtual void AddOption(const std::string &key, float value);
  virtual void AddOption(const std::string &key, double value);
  virtual void AddOption(const std::string &key, bool value);
  virtual void AddOptions(const std::string &options);
  virtual void AddOptions(const CUrlOptions &options);

  virtual bool HasOption(const std::string &key) const;
  virtual bool GetOption(const std::string &key, CVariant &value) const;

protected:
  UrlOptions m_options;
  std::string m_strLead;
};
