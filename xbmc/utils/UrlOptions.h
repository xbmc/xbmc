/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

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
  std::string GetOptionsString(bool withLeadingSeparator = false) const;

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
