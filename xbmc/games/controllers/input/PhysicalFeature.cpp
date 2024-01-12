/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "PhysicalFeature.h"

#include "games/controllers/Controller.h"
#include "games/controllers/ControllerDefinitions.h"
#include "games/controllers/ControllerTranslator.h"
#include "guilib/LocalizeStrings.h"
#include "input/keyboard/KeyboardTranslator.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <sstream>

#include <tinyxml2.h>

using namespace KODI;
using namespace GAME;
using namespace JOYSTICK;

CPhysicalFeature::CPhysicalFeature(int labelId)
{
  Reset();
  m_labelId = labelId;
}

void CPhysicalFeature::Reset(void)
{
  *this = CPhysicalFeature();
}

CPhysicalFeature& CPhysicalFeature::operator=(const CPhysicalFeature& rhs)
{
  if (this != &rhs)
  {
    m_controller = rhs.m_controller;
    m_type = rhs.m_type;
    m_category = rhs.m_category;
    m_categoryLabelId = rhs.m_categoryLabelId;
    m_strName = rhs.m_strName;
    m_labelId = rhs.m_labelId;
    m_inputType = rhs.m_inputType;
    m_keycode = rhs.m_keycode;
  }
  return *this;
}

std::string CPhysicalFeature::CategoryLabel() const
{
  std::string categoryLabel;

  if (m_categoryLabelId >= 0 && m_controller != nullptr)
    categoryLabel = g_localizeStrings.GetAddonString(m_controller->ID(), m_categoryLabelId);

  if (categoryLabel.empty())
    categoryLabel = g_localizeStrings.Get(m_categoryLabelId);

  return categoryLabel;
}

std::string CPhysicalFeature::Label() const
{
  std::string label;

  if (m_labelId >= 0 && m_controller != nullptr)
    label = g_localizeStrings.GetAddonString(m_controller->ID(), m_labelId);

  if (label.empty())
    label = g_localizeStrings.Get(m_labelId);

  return label;
}

bool CPhysicalFeature::Deserialize(const tinyxml2::XMLElement* pElement,
                                   const CController* controller,
                                   FEATURE_CATEGORY category,
                                   int categoryLabelId)
{
  Reset();

  if (!pElement)
    return false;

  std::string strType(pElement->Value());

  // Type
  m_type = CControllerTranslator::TranslateFeatureType(strType);
  if (m_type == FEATURE_TYPE::UNKNOWN)
  {
    CLog::Log(LOGDEBUG, "Invalid feature: <{}> ", pElement->Value());
    return false;
  }

  // Category was obtained from parent XML node
  m_category = category;
  m_categoryLabelId = categoryLabelId;

  // Name
  m_strName = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_FEATURE_NAME);
  if (m_strName.empty())
  {
    CLog::Log(LOGERROR, "<{}> tag has no \"{}\" attribute", strType, LAYOUT_XML_ATTR_FEATURE_NAME);
    return false;
  }

  // Label ID
  std::string strLabel = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_FEATURE_LABEL);
  if (strLabel.empty())
    CLog::Log(LOGDEBUG, "<{}> tag has no \"{}\" attribute", strType, LAYOUT_XML_ATTR_FEATURE_LABEL);
  else
    std::istringstream(strLabel) >> m_labelId;

  // Input type
  if (m_type == FEATURE_TYPE::SCALAR)
  {
    std::string strInputType = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_INPUT_TYPE);
    if (strInputType.empty())
    {
      CLog::Log(LOGERROR, "<{}> tag has no \"{}\" attribute", strType, LAYOUT_XML_ATTR_INPUT_TYPE);
      return false;
    }
    else
    {
      m_inputType = CControllerTranslator::TranslateInputType(strInputType);
      if (m_inputType == INPUT_TYPE::UNKNOWN)
      {
        CLog::Log(LOGERROR, "<{}> tag - attribute \"{}\" is invalid: \"{}\"", strType,
                  LAYOUT_XML_ATTR_INPUT_TYPE, strInputType);
        return false;
      }
    }
  }

  // Keycode
  if (m_type == FEATURE_TYPE::KEY)
  {
    std::string strSymbol = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_KEY_SYMBOL);
    if (strSymbol.empty())
    {
      CLog::Log(LOGERROR, "<{}> tag has no \"{}\" attribute", strType, LAYOUT_XML_ATTR_KEY_SYMBOL);
      return false;
    }
    else
    {
      m_keycode = KEYBOARD::CKeyboardTranslator::TranslateKeysym(strSymbol);
      if (m_keycode == XBMCK_UNKNOWN)
      {
        CLog::Log(LOGERROR, "<{}> tag - attribute \"{}\" is invalid: \"{}\"", strType,
                  LAYOUT_XML_ATTR_KEY_SYMBOL, strSymbol);
        return false;
      }
    }
  }

  // Save controller for string translation
  m_controller = controller;

  return true;
}
