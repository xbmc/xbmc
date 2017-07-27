/*
 *      Copyright (C) 2015-2017 Team Kodi
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

#include "ControllerLayout.h"
#include "Controller.h"
#include "ControllerDefinitions.h"
#include "ControllerTranslator.h"
#include "guilib/LocalizeStrings.h"
#include "utils/log.h"
#include "utils/XMLUtils.h"

#include <algorithm>
#include <sstream>

using namespace KODI;
using namespace GAME;
using namespace JOYSTICK;

// --- FeatureTypeEqual --------------------------------------------------------

struct FeatureTypeEqual
{
  FeatureTypeEqual(FEATURE_TYPE type, INPUT_TYPE inputType) : type(type), inputType(inputType) { }

  bool operator()(const CControllerFeature& feature) const
  {
    if (type == FEATURE_TYPE::UNKNOWN)
      return true; // Match all feature types

    if (type == FEATURE_TYPE::SCALAR && feature.Type() == FEATURE_TYPE::SCALAR)
    {
      if (inputType == INPUT_TYPE::UNKNOWN)
        return true; // Match all input types

      return inputType == feature.InputType();
    }

    return type == feature.Type();
  }

  const FEATURE_TYPE type;
  const INPUT_TYPE   inputType;
};

// --- CControllerLayout ---------------------------------------------------

void CControllerLayout::Reset(void)
{
  m_label = 0;
  m_strImage.clear();
  m_strOverlay.clear();
  m_width = 0;
  m_height = 0;
  m_features.clear();
}

unsigned int CControllerLayout::FeatureCount(FEATURE_TYPE type      /* = FEATURE_TYPE::UNKNOWN */,
                                             INPUT_TYPE   inputType /* = INPUT_TYPE::UNKNOWN */) const
{
  return std::count_if(m_features.begin(), m_features.end(), FeatureTypeEqual(type, inputType));
}

FEATURE_TYPE CControllerLayout::FeatureType(const std::string &featureName) const
{
  FEATURE_TYPE type = FEATURE_TYPE::UNKNOWN;

  auto it = std::find_if(m_features.begin(), m_features.end(),
    [&featureName](const CControllerFeature &feature)
    {
      return feature.Name() == featureName;
    });

  if (it != m_features.end())
    type = it->Type();

  return type;
}

bool CControllerLayout::Deserialize(const TiXmlElement* pElement, const CController* controller)
{
  Reset();

  if (!pElement)
    return false;

  // Label
  std::string strLabel = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_LAYOUT_LABEL);
  if (strLabel.empty())
  {
    CLog::Log(LOGERROR, "<%s> tag has no \"%s\" attribute", LAYOUT_XML_ROOT, LAYOUT_XML_ATTR_LAYOUT_LABEL);
    return false;
  }
  std::istringstream(strLabel) >> m_label;

  // Image
  m_strImage = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_LAYOUT_IMAGE);
  if (m_strImage.empty())
    CLog::Log(LOGDEBUG, "<%s> tag has no \"%s\" attribute", LAYOUT_XML_ROOT, LAYOUT_XML_ATTR_LAYOUT_IMAGE);

  // Features
  for (const TiXmlElement* pCategory = pElement->FirstChildElement(); pCategory != nullptr; pCategory = pCategory->NextSiblingElement())
  {
    if (pCategory->ValueStr() != LAYOUT_XML_ELM_CATEGORY)
    {
      CLog::Log(LOGDEBUG, "Ignoring <%s> tag", pCategory->ValueStr().c_str());
      continue;
    }

    // Category
    std::string strCategory = XMLUtils::GetAttribute(pCategory, LAYOUT_XML_ATTR_CATEGORY_NAME);
    FEATURE_CATEGORY category = CControllerTranslator::TranslateFeatureCategory(strCategory);

    // Category label
    std::string strCategoryLabel;

    std::string strCategoryLabelId = XMLUtils::GetAttribute(pCategory, LAYOUT_XML_ATTR_CATEGORY_LABEL);
    if (!strCategoryLabelId.empty())
    {
      unsigned int categoryLabelId;
      std::istringstream(strCategoryLabelId) >> categoryLabelId;
      strCategoryLabel = g_localizeStrings.GetAddonString(controller->ID(), categoryLabelId);
      if (strCategoryLabel.empty())
        strCategoryLabel = g_localizeStrings.Get(categoryLabelId);
    }

    for (const TiXmlElement* pFeature = pCategory->FirstChildElement(); pFeature != nullptr; pFeature = pFeature->NextSiblingElement())
    {
      CControllerFeature feature;

      if (feature.Deserialize(pFeature, controller, category, strCategoryLabel))
        m_features.push_back(feature);
    }
  }

  return true;
}
