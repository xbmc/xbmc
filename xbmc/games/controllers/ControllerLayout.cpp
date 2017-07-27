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
  m_label = 0;
  m_strImage.clear();
  m_strOverlay.clear();
  m_width = 0;
  m_height = 0;
}

bool CControllerLayout::Deserialize(const TiXmlElement* pElement, const CController* controller, std::vector<CControllerFeature> &features)
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

  return true;
}
