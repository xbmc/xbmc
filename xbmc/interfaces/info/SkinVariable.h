/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>
#include <vector>
#include "guilib/guiinfo/GUIInfoLabel.h"
#include "interfaces/info/InfoBool.h"

class TiXmlElement;

namespace INFO
{
class CSkinVariableString;

class CSkinVariable
{
public:
  static const CSkinVariableString* CreateFromXML(const TiXmlElement& node, int context);
};

class CSkinVariableString
{
public:
  const std::string& GetName() const;
  int GetContext() const;
  std::string GetValue(bool preferImage = false, const CGUIListItem *item = nullptr) const;
private:
  CSkinVariableString();

  std::string m_name;
  int m_context;

  struct ConditionLabelPair
  {
    INFO::InfoPtr m_condition;
    KODI::GUILIB::GUIINFO::CGUIInfoLabel m_label;
  };

  typedef std::vector<ConditionLabelPair> VECCONDITIONLABELPAIR;
  VECCONDITIONLABELPAIR m_conditionLabelPairs;

  friend class CSkinVariable;
};

}
