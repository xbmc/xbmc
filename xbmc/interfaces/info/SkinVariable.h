#pragma once
/*
 *      Copyright (C) 2005-present Team Kodi
 *      This file is part of Kodi - https://kodi.tv
 *
 *  Kodi is free software: you can redistribute it and/or modify it
 *  under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  Kodi is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with Kodi. If not, see <https://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>
#include "guilib/guiinfo/GUIInfoTypes.h"
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
