/*
 *  Copyright (C) 2013-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/logtypes.h"

#include <string>

class TiXmlNode;

enum class SettingUpdateType {
  Unknown = 0,
  Rename,
  Change
};

class CSettingUpdate
{
public:
  CSettingUpdate();
  virtual ~CSettingUpdate() = default;

  inline bool operator<(const CSettingUpdate& rhs) const
  {
    return m_type < rhs.m_type && m_value < rhs.m_value;
  }

  virtual bool Deserialize(const TiXmlNode *node);

  SettingUpdateType GetType() const { return m_type; }
  const std::string& GetValue() const { return m_value; }

private:
  bool setType(const std::string &type);

  SettingUpdateType m_type = SettingUpdateType::Unknown;
  std::string m_value;

  static Logger s_logger;
};
