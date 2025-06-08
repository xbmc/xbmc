/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "DialogGameVideoFilter.h"

#include "ServiceBroker.h"
#include "URL.h"
#include "cores/RetroPlayer/guibridge/GUIGameVideoHandle.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "cores/RetroPlayer/shaders/ShaderPresetFactory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "games/GameServices.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <stdlib.h>

using namespace KODI;
using namespace GAME;

#define PRESETS_ADDON_NAME "game.shader.presets"

namespace
{
struct ScalingMethodProperties
{
  int nameIndex;
  int categoryIndex;
  int descriptionIndex;
  RETRO::SCALINGMETHOD scalingMethod;
};

const std::vector<ScalingMethodProperties> scalingMethods = {
    {16301, 16296, 16298, RETRO::SCALINGMETHOD::NEAREST},
    {16302, 16297, 16299, RETRO::SCALINGMETHOD::LINEAR},
};
} // namespace

CDialogGameVideoFilter::CDialogGameVideoFilter()
  : CDialogGameVideoSelect(WINDOW_DIALOG_GAME_VIDEO_FILTER)
{
}

std::string CDialogGameVideoFilter::GetHeading()
{
  return g_localizeStrings.Get(35225); // "Video filter"
}

void CDialogGameVideoFilter::PreInit()
{
  m_items.Clear();

  InitScalingMethods();
  InitVideoFilters();

  if (m_items.Size() == 0)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(231)); // "None"
    m_items.Add(std::move(item));
  }

  m_bHasDescription = false;
}

void CDialogGameVideoFilter::InitScalingMethods()
{
  if (m_gameVideoHandle)
  {
    for (const auto& scalingMethodProps : scalingMethods)
    {
      if (m_gameVideoHandle->SupportsScalingMethod(scalingMethodProps.scalingMethod))
      {
        RETRO::CRenderVideoSettings videoSettings;
        videoSettings.SetScalingMethod(scalingMethodProps.scalingMethod);

        CFileItemPtr item =
            std::make_shared<CFileItem>(g_localizeStrings.Get(scalingMethodProps.nameIndex));
        item->SetLabel2(g_localizeStrings.Get(scalingMethodProps.categoryIndex));
        item->SetProperty("game.videofilter", CVariant{videoSettings.GetVideoFilter()});
        item->SetProperty("game.videofilterdescription",
                          CVariant{g_localizeStrings.Get(scalingMethodProps.descriptionIndex)});
        m_items.Add(std::move(item));
      }
    }
  }
}

void CDialogGameVideoFilter::InitVideoFilters()
{

  std::vector<VideoFilterProperties> videoFilters;

  std::string xmlPath;
  std::unique_ptr<CXBMCTinyXML> xml;

  //! @todo Have the add-on give us the xml as a string (or parse it)
  std::string xmlFilename;
#ifdef TARGET_WINDOWS
  xmlFilename = "ShaderPresetsHLSLP.xml";
#else
  xmlFilename = "ShaderPresetsGLSLP.xml";
#endif

  const std::string homeAddonPath = CSpecialProtocol::TranslatePath(
      URIUtils::AddFileToFolder("special://home", "addons", PRESETS_ADDON_NAME));
  const std::string systemAddonPath = CSpecialProtocol::TranslatePath(
      URIUtils::AddFileToFolder("special://xbmc", "addons", PRESETS_ADDON_NAME));
  const std::string binAddonPath = CSpecialProtocol::TranslatePath(
      URIUtils::AddFileToFolder("special://xbmcbinaddons", PRESETS_ADDON_NAME));

  for (const auto& basePath : {homeAddonPath, systemAddonPath, binAddonPath})
  {
    xmlPath = URIUtils::AddFileToFolder(basePath, "resources", xmlFilename);

    CLog::LogF(LOGDEBUG, "Looking for shader preset XML at {}", CURL::GetRedacted(xmlPath));

    if (XFILE::CFile::Exists(xmlPath))
    {
      xml = std::make_unique<CXBMCTinyXML>(xmlPath);
      if (xml->LoadFile())
        break;

      CLog::LogF(LOGERROR, "Couldn't load shader presets from XML, {}", CURL::GetRedacted(xmlPath));
      xml.reset();
    }
  }

  if (!xml)
    return;

  auto root = xml->RootElement();
  TiXmlNode* child = nullptr;

  while ((child = root->IterateChildren(child)))
  {
    VideoFilterProperties videoFilter;

    if (child->FirstChild() == nullptr)
      continue;

    TiXmlNode* pathNode;
    if ((pathNode = child->FirstChild("path")))
      if ((pathNode = pathNode->FirstChild()))
        videoFilter.path =
            URIUtils::AddFileToFolder(URIUtils::GetBasePath(xmlPath), pathNode->Value());

    TiXmlNode* nameNode;
    if ((nameNode = child->FirstChild("name")))
      if ((nameNode = nameNode->FirstChild()))
        videoFilter.name = nameNode->Value();

    TiXmlNode* folderNode;
    if ((folderNode = child->FirstChild("folder")))
      if ((folderNode = folderNode->FirstChild()))
        videoFilter.folder = folderNode->Value();

    videoFilters.emplace_back(videoFilter);
  }

  CLog::Log(LOGDEBUG, "Loaded {} shader presets from default XML, {}", videoFilters.size(),
            CURL::GetRedacted(xmlPath));

  for (const auto& videoFilter : videoFilters)
  {
    bool canLoadPreset =
        CServiceBroker::GetGameServices().VideoShaders().CanLoadPreset(videoFilter.path);

    if (!canLoadPreset)
      continue;

    CFileItem item{videoFilter.name};
    item.SetLabel2(videoFilter.folder);
    item.SetProperty("game.videofilter", CVariant{videoFilter.path});

    m_items.Add(std::move(item));
  }
}

void CDialogGameVideoFilter::GetItems(CFileItemList& items)
{
  for (const auto& item : m_items)
    items.Add(item);
}

void CDialogGameVideoFilter::OnItemFocus(unsigned int index)
{
  if (static_cast<int>(index) < m_items.Size())
  {
    CFileItemPtr item = m_items[index];

    std::string videoFilter;
    std::string description;
    GetProperties(*item, videoFilter, description);

    ::CGameSettings& gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

    if (gameSettings.VideoFilter() != videoFilter)
    {
      gameSettings.SetVideoFilter(videoFilter);
      gameSettings.NotifyObservers(ObservableMessageSettingsChanged);

      OnDescriptionChange(description);
      m_bHasDescription = true;
    }
    else if (!m_bHasDescription)
    {
      OnDescriptionChange(description);
      m_bHasDescription = true;
    }
  }
}

unsigned int CDialogGameVideoFilter::GetFocusedItem() const
{
  ::CGameSettings& gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  for (int i = 0; i < m_items.Size(); i++)
  {
    std::string videoFilter;
    std::string description;
    GetProperties(*m_items[i], videoFilter, description);

    if (videoFilter == gameSettings.VideoFilter())
    {
      return i;
    }
  }

  return 0;
}

void CDialogGameVideoFilter::PostExit()
{
  m_items.Clear();
}

bool CDialogGameVideoFilter::OnClickAction()
{
  Close();
  return true;
}

std::string CDialogGameVideoFilter::GetLocalizedString(uint32_t code)
{
  return g_localizeStrings.GetAddonString(PRESETS_ADDON_NAME, code);
}

void CDialogGameVideoFilter::GetProperties(const CFileItem& item,
                                           std::string& videoFilter,
                                           std::string& description)
{
  videoFilter = item.GetProperty("game.videofilter").asString();
  description = item.GetProperty("game.videofilterdescription").asString();
}
