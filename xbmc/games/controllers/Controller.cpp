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

#include "Controller.h"
#include "ControllerDefinitions.h"
#include "ControllerLayout.h"
#include "ControllerTopology.h"
#include "utils/log.h"
#include "utils/URIUtils.h"
#include "utils/XBMCTinyXML.h"
#include "utils/XMLUtils.h"
#include "URL.h"

#include <algorithm>

using namespace KODI;
using namespace GAME;

// --- FeatureTypeEqual --------------------------------------------------------

struct FeatureTypeEqual
{
  FeatureTypeEqual(FEATURE_TYPE type, JOYSTICK::INPUT_TYPE inputType) :
    type(type),
    inputType(inputType)
  {
  }

  bool operator()(const CControllerFeature& feature) const
  {
    if (type == FEATURE_TYPE::UNKNOWN)
      return true; // Match all feature types

    if (type == FEATURE_TYPE::SCALAR && feature.Type() == FEATURE_TYPE::SCALAR)
    {
      if (inputType == JOYSTICK::INPUT_TYPE::UNKNOWN)
        return true; // Match all input types

      return inputType == feature.InputType();
    }

    return type == feature.Type();
  }

  const FEATURE_TYPE type;
  const JOYSTICK::INPUT_TYPE inputType;
};

// --- CController -------------------------------------------------------------

const ControllerPtr CController::EmptyPtr;

std::unique_ptr<CController> CController::FromExtension(ADDON::CAddonInfo addonInfo, const cp_extension_t* ext)
{
  return std::unique_ptr<CController>(new CController(std::move(addonInfo)));
}

CController::CController(ADDON::CAddonInfo addonInfo) :
  CAddon(std::move(addonInfo)),
  m_layout(new CControllerLayout)
{
}

CController::~CController() = default;

const CControllerFeature& CController::GetFeature(const std::string &name) const
{
  auto it = std::find_if(m_features.begin(), m_features.end(),
    [&name](const CControllerFeature &feature)
    {
      return name == feature.Name();
    });

  if (it != m_features.end())
    return *it;

  static const CControllerFeature invalid{};
  return invalid;
}

unsigned int CController::FeatureCount(FEATURE_TYPE type /* = FEATURE_TYPE::UNKNOWN */,
                                       JOYSTICK::INPUT_TYPE inputType /* = JOYSTICK::INPUT_TYPE::UNKNOWN */) const
{
  return std::count_if(m_features.begin(), m_features.end(), FeatureTypeEqual(type, inputType));
}

void CController::GetFeatures(std::vector<std::string>& features,
                              FEATURE_TYPE type /* = FEATURE_TYPE::UNKNOWN */) const
{
  for (const CControllerFeature& feature : m_features)
  {
    if (type == FEATURE_TYPE::UNKNOWN || type == feature.Type())
      features.push_back(feature.Name());
  }
}

JOYSTICK::FEATURE_TYPE CController::FeatureType(const std::string &feature) const
{
  for (auto it = m_features.begin(); it != m_features.end(); ++it)
  {
    if (feature == it->Name())
      return it->Type();
  }
  return JOYSTICK::FEATURE_TYPE::UNKNOWN;
}

JOYSTICK::INPUT_TYPE CController::GetInputType(const std::string& feature) const
{
  for (auto it = m_features.begin(); it != m_features.end(); ++it)
  {
    if (feature == it->Name())
      return it->InputType();
  }
  return JOYSTICK::INPUT_TYPE::UNKNOWN;
}

bool CController::LoadLayout(void)
{
  if (!m_bLoaded)
  {
    std::string strLayoutXmlPath = LibPath();

    CLog::Log(LOGINFO, "Loading controller layout: %s", CURL::GetRedacted(strLayoutXmlPath).c_str());

    CXBMCTinyXML xmlDoc;
    if (!xmlDoc.LoadFile(strLayoutXmlPath))
    {
      CLog::Log(LOGDEBUG, "Unable to load file: %s at line %d", xmlDoc.ErrorDesc(), xmlDoc.ErrorRow());
      return false;
    }

    TiXmlElement* pRootElement = xmlDoc.RootElement();
    if (!pRootElement || pRootElement->NoChildren() || pRootElement->ValueStr() != LAYOUT_XML_ROOT)
    {
      CLog::Log(LOGERROR, "Can't find root <%s> tag", LAYOUT_XML_ROOT);
      return false;
    }

    m_layout->Deserialize(pRootElement, this, m_features);
    if (m_layout->IsValid(true))
    {
      m_bLoaded = true;
    }
    else
    {
      m_layout->Reset();
    }
  }

  return m_bLoaded;
}

const CControllerTopology& CController::Topology() const
{
  return m_layout->Topology();
}
