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
#include "guilib/LocalizeStrings.h"
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

std::string CController::Label(void)
{
  if (m_layout->LabelID() >= 0)
    return g_localizeStrings.GetAddonString(ID(), m_layout->LabelID());
  return "";
}

std::string CController::ImagePath(void) const
{
  if (!m_layout->Image().empty())
    return URIUtils::AddFileToFolder(URIUtils::GetDirectory(LibPath()), m_layout->Image());
  return "";
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

      // Load models
      if (!m_layout->Models().empty())
      {
        std::string modelPath = URIUtils::AddFileToFolder(URIUtils::GetDirectory(LibPath()), m_layout->Models());
        LoadModels(modelPath);
      }
    }
    else
    {
      m_layout->Reset();
    }
  }

  return m_bLoaded;
}

void CController::LoadModels(const std::string &modelXmlPath)
{
  CLog::Log(LOGINFO, "Loading controller models: %s", CURL::GetRedacted(modelXmlPath).c_str());

  CXBMCTinyXML modelsDoc;
  if (!modelsDoc.LoadFile(modelXmlPath))
  {
    CLog::Log(LOGERROR, "Unable to load file: %s at line %d", modelsDoc.ErrorDesc(), modelsDoc.ErrorRow());
    return;
  }

  TiXmlElement* pModelsElement = modelsDoc.RootElement();
  if (pModelsElement == nullptr || pModelsElement->ValueStr() != MODELS_XML_ROOT)
  {
    CLog::Log(LOGERROR, "Can't find root <%s> tag", MODELS_XML_ROOT);
    return;
  }

  for (const TiXmlElement* pChild = pModelsElement->FirstChildElement(); pChild != nullptr; pChild = pChild->NextSiblingElement())
  {
    if (pChild->ValueStr() == MODELS_XML_ELM_MODEL)
    {
      // Model name
      std::string modelName = XMLUtils::GetAttribute(pChild, MODELS_XML_ATTR_MODEL_NAME);
      if (modelName.empty())
      {
        CLog::Log(LOGERROR, "Invalid <%s> tag: missing attribute \"%s\"", pChild->ValueStr().c_str(), MODELS_XML_ATTR_MODEL_NAME);
        continue;
      }

      if (m_models.find(modelName) != m_models.end())
      {
        CLog::Log(LOGERROR, "Duplicate model name: \"%s\"", modelName.c_str());
        continue;
      }

      const TiXmlElement* pLayout = pChild->FirstChildElement();

      // Duplicate primary layout
      std::unique_ptr<CControllerLayout> layout(new CControllerLayout(*m_layout));

      // Models can't override features
      std::vector<CControllerFeature> dummy;

      layout->Deserialize(pLayout, this, dummy);
      m_models.insert(std::make_pair(std::move(modelName), std::move(layout)));
    }
    else
    {
      CLog::Log(LOGERROR, "Invalid tag: <%s>", pChild->ValueStr().c_str());
    }
  }
}

std::vector<std::string> CController::Models() const
{
  std::vector<std::string> models;

  for (const auto &it : m_models)
    models.emplace_back(it.first);

  return models;
}

const CControllerLayout& CController::GetModel(const std::string& model) const
{
  auto it = m_models.find(model);

  if (it != m_models.end())
    return *it->second;

  return *m_layout;
}
