/*
 *      Copyright (C) 2015-2016 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ControllerFeature.h"
#include "Controller.h"
#include "ControllerDefinitions.h"
#include "ControllerTranslator.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/XMLUtils.h"

#include <sstream>

using namespace GAME;
using namespace JOYSTICK;

void CControllerFeature::Reset(void)
{
  m_type = FEATURE_TYPE::UNKNOWN;
  m_category = FEATURE_CATEGORY::UNKNOWN;
  m_strName.clear();
  m_strLabel.clear();
  m_labelId = 0;
  m_inputType = INPUT_TYPE::UNKNOWN;
}

CControllerFeature& CControllerFeature::operator=(const CControllerFeature& rhs)
{
  if (this != &rhs)
  {
    m_type       = rhs.m_type;
    m_category   = rhs.m_category;
    m_strName    = rhs.m_strName;
    m_strLabel   = rhs.m_strLabel;
    m_labelId    = rhs.m_labelId;
    m_inputType  = rhs.m_inputType;
  }
  return *this;
}

bool CControllerFeature::Deserialize(const TiXmlElement* pElement, const CController* controller, const std::string& strCategory)
{
  Reset();

  if (!pElement)
    return false;

  std::string strType(pElement->Value());

  // Type
  m_type = CControllerTranslator::TranslateFeatureType(strType);
  if (m_type == FEATURE_TYPE::UNKNOWN)
  {
    CLog::Log(LOGERROR, "Invalid feature: <%s> ", pElement->Value());
    return false;
  }

  // Category was obtained from parent XML node
  m_category = CControllerTranslator::TranslateCategory(strCategory);

  // Name
  m_strName = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_FEATURE_NAME);
  if (m_strName.empty())
  {
    CLog::Log(LOGERROR, "<%s> tag has no \"%s\" attribute", strType.c_str(), LAYOUT_XML_ATTR_FEATURE_NAME);
    return false;
  }

  // Label (not used for motors)
  if (m_type != FEATURE_TYPE::MOTOR)
  {
    // Label ID
    std::string strLabel = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_FEATURE_LABEL);
    if (strLabel.empty())
    {
      CLog::Log(LOGERROR, "<%s> tag has no \"%s\" attribute", strType.c_str(), LAYOUT_XML_ATTR_FEATURE_LABEL);
      return false;
    }
    std::istringstream(strLabel) >> m_labelId;

    // Label (string)
    m_strLabel = g_localizeStrings.GetAddonString(controller->ID(), m_labelId);
  }

  // Input type
  if (m_type == FEATURE_TYPE::SCALAR)
  {
    std::string strInputType = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_INPUT_TYPE);
    if (strInputType.empty())
    {
      CLog::Log(LOGERROR, "<%s> tag has no \"%s\" attribute", strType.c_str(), LAYOUT_XML_ATTR_INPUT_TYPE);
      return false;
    }
    else
    {
      m_inputType = CControllerTranslator::TranslateInputType(strInputType);
      if (m_inputType == INPUT_TYPE::UNKNOWN)
      {
        CLog::Log(LOGERROR, "<%s> tag - attribute \"%s\" is invalid: \"%s\"",
                  strType.c_str(), LAYOUT_XML_ATTR_INPUT_TYPE, strInputType.c_str());
        return false;
      }
    }
  }

  return true;
}
