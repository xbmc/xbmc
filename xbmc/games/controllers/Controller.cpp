/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "Controller.h"

#include "ControllerDefinitions.h"
#include "ControllerLayout.h"
#include "URL.h"
#include "addons/addoninfo/AddonType.h"
#include "games/controllers/input/PhysicalTopology.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/XMLUtils.h"
#include "utils/log.h"

#include <algorithm>
#include <cstring>

using namespace KODI;
using namespace GAME;

// --- FeatureTypeEqual --------------------------------------------------------

struct FeatureTypeEqual
{
  FeatureTypeEqual(FEATURE_TYPE type, JOYSTICK::INPUT_TYPE inputType)
    : type(type), inputType(inputType)
  {
  }

  bool operator()(const CPhysicalFeature& feature) const
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

CController::CController(const ADDON::AddonInfoPtr& addonInfo)
  : CAddon(addonInfo, ADDON::AddonType::GAME_CONTROLLER), m_layout(new CControllerLayout)
{
}

CController::~CController() = default;

const CPhysicalFeature& CController::GetFeature(const std::string& name) const
{
  auto it =
      std::find_if(m_features.begin(), m_features.end(),
                   [&name](const CPhysicalFeature& feature) { return name == feature.Name(); });

  if (it != m_features.end())
    return *it;

  static const CPhysicalFeature invalid{};
  return invalid;
}

unsigned int CController::FeatureCount(
    FEATURE_TYPE type /* = FEATURE_TYPE::UNKNOWN */,
    JOYSTICK::INPUT_TYPE inputType /* = JOYSTICK::INPUT_TYPE::UNKNOWN */) const
{
  auto featureCount =
      std::count_if(m_features.begin(), m_features.end(), FeatureTypeEqual(type, inputType));
  return static_cast<unsigned int>(featureCount);
}

void CController::GetFeatures(std::vector<std::string>& features,
                              FEATURE_TYPE type /* = FEATURE_TYPE::UNKNOWN */) const
{
  for (const CPhysicalFeature& feature : m_features)
  {
    if (type == FEATURE_TYPE::UNKNOWN || type == feature.Type())
      features.push_back(feature.Name());
  }
}

JOYSTICK::FEATURE_TYPE CController::FeatureType(const std::string& feature) const
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

    CLog::Log(LOGINFO, "Loading controller layout: {}", CURL::GetRedacted(strLayoutXmlPath));

    CXBMCTinyXML2 xmlDoc;
    if (!xmlDoc.LoadFile(strLayoutXmlPath))
    {
      CLog::Log(LOGDEBUG, "Unable to load file: {} at line {}", xmlDoc.ErrorStr(),
                xmlDoc.ErrorLineNum());
      return false;
    }

    auto* pRootElement = xmlDoc.RootElement();
    if (pRootElement == nullptr || pRootElement->NoChildren() ||
        std::strcmp(pRootElement->Value(), LAYOUT_XML_ROOT) != 0)
    {
      CLog::Log(LOGERROR, "Can't find root <{}> tag", LAYOUT_XML_ROOT);
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

const CPhysicalTopology& CController::Topology() const
{
  return m_layout->Topology();
}
