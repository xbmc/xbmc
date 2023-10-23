/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "URL.h"
#include "utils/UrlOptions.h"

#include <string>

class CVariant;

class CDbUrl : CUrlOptions
{
public:
  CDbUrl();
  ~CDbUrl() override;

  bool IsValid() const { return m_valid; }
  void Reset();

  std::string ToString() const;
  bool FromString(const std::string &dbUrl);

  const std::string& GetType() const { return m_type; }
  void AppendPath(const std::string &subPath);

  using CUrlOptions::GetOption;
  using CUrlOptions::GetOptions;
  using CUrlOptions::GetOptionsString;
  using CUrlOptions::HasOption;

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
