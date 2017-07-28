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
#include "utils/log.h"
#include "utils/XMLUtils.h"

#include <sstream>

using namespace KODI;
using namespace GAME;

void CControllerLayout::Reset(void)
{
  m_labelId = -1;
  m_icon.clear();
  m_strImage.clear();
  m_models.clear();
}

bool CControllerLayout::IsValid(bool bLog) const
{
  if (m_labelId < 0)
  {
    if (bLog)
      CLog::Log(LOGERROR, "<%s> tag has no \"%s\" attribute", LAYOUT_XML_ROOT, LAYOUT_XML_ATTR_LAYOUT_LABEL);
    return false;
  }

  if (m_strImage.empty())
  {
    if (bLog)
      CLog::Log(LOGDEBUG, "<%s> tag has no \"%s\" attribute", LAYOUT_XML_ROOT, LAYOUT_XML_ATTR_LAYOUT_IMAGE);
    return false;
  }

  return true;
}

void CControllerLayout::Deserialize(const TiXmlElement* pElement, const CController* controller, std::vector<CControllerFeature> &features)
{
  if (pElement == nullptr || controller == nullptr)
    return;

  // Label
  std::string strLabel = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_LAYOUT_LABEL);
  if (!strLabel.empty())
    std::istringstream(strLabel) >> m_labelId;

  // Icon
  std::string icon = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_LAYOUT_ICON);
  if (!icon.empty())
    m_icon = icon;

  // Fallback icon, use add-on icon
  if (m_icon.empty())
    m_icon = controller->Icon();

  // Image
  std::string image = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_LAYOUT_IMAGE);
  if (!image.empty())
    m_strImage = image;

  // Models
  std::string models = XMLUtils::GetAttribute(pElement, LAYOUT_XML_ATTR_LAYOUT_MODELS);
  if (!models.empty())
    m_models = models;

  // Features
  for (const TiXmlElement* pChild = pElement->FirstChildElement(); pChild != nullptr; pChild = pChild->NextSiblingElement())
  {
    if (pChild->ValueStr() == LAYOUT_XML_ELM_CATEGORY)
    {
      // Category
      std::string strCategory = XMLUtils::GetAttribute(pChild, LAYOUT_XML_ATTR_CATEGORY_NAME);
      JOYSTICK::FEATURE_CATEGORY category = CControllerTranslator::TranslateFeatureCategory(strCategory);

      // Category label
      int categoryLabelId = -1;

      std::string strCategoryLabelId = XMLUtils::GetAttribute(pChild, LAYOUT_XML_ATTR_CATEGORY_LABEL);
      if (!strCategoryLabelId.empty())
        std::istringstream(strCategoryLabelId) >> categoryLabelId;

      for (const TiXmlElement* pFeature = pChild->FirstChildElement(); pFeature != nullptr; pFeature = pFeature->NextSiblingElement())
      {
        CControllerFeature feature;

        if (feature.Deserialize(pFeature, controller, category, categoryLabelId))
          features.push_back(feature);
      }
    }
    else
    {
      CLog::Log(LOGDEBUG, "Ignoring <%s> tag", pChild->ValueStr().c_str());
    }
  }
}
