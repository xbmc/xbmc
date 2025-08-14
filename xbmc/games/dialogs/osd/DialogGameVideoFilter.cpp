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
#include "addons/AddonInstaller.h"
#include "addons/AddonManager.h"
#include "addons/addoninfo/AddonType.h"
#include "cores/RetroPlayer/guibridge/GUIGameVideoHandle.h"
#include "cores/RetroPlayer/rendering/RenderVideoSettings.h"
#include "cores/RetroPlayer/shaders/ShaderPresetFactory.h"
#include "filesystem/File.h"
#include "filesystem/SpecialProtocol.h"
#include "games/GameServices.h"
#include "games/dialogs/DialogGameDefines.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "guilib/WindowIDs.h"
#include "jobs/JobManager.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "threads/SystemClock.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML.h"
#include "utils/log.h"

#include <stdlib.h>

using namespace KODI;
using namespace GAME;
using namespace std::chrono_literals;

namespace
{

constexpr const char* PRESETS_ADDON_NAME = "game.shader.presets";
constexpr const char* ICON_VIDEO = "DefaultVideo.png";
constexpr const char* ICON_GET_MORE = "DefaultAddSource.png";

struct ScalingMethodProperties
{
  int nameIndex;
  int categoryIndex;
  RETRO::SCALINGMETHOD scalingMethod;
};

const std::vector<ScalingMethodProperties> scalingMethods = {
    {16301, 16296, RETRO::SCALINGMETHOD::NEAREST},
    {16302, 16297, RETRO::SCALINGMETHOD::LINEAR},
};

// Helper function
void GetProperties(const CFileItem& item, std::string& videoFilter)
{
  videoFilter = item.GetProperty("game.videofilter").asString();
}

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
  InitGetMoreButton();

  if (m_items.Size() == 0)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(g_localizeStrings.Get(231)); // "None"
    m_items.Add(std::move(item));
  }
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
        item->SetArt("icon", ICON_VIDEO);
        m_items.Add(std::move(item));
      }
    }
  }
}

namespace
{
TiXmlNode* GetFirstChildOfNode(TiXmlNode& node, const char* childName)
{
  TiXmlNode* ret{node.FirstChild(childName)};
  if (ret)
    return ret->FirstChild();

  return ret;
}
} // unnamed namespace

void CDialogGameVideoFilter::InitVideoFilters()
{

  std::vector<VideoFilterProperties> videoFilters;

  std::string xmlPath;
  std::unique_ptr<CXBMCTinyXML> xml;

  //! @todo Have the add-on give us the xml as a string (or parse it)
  std::string xmlFilename;
#if defined(HAS_GLES)
  xmlFilename = "ShaderPresetsGLSLP_GLES.xml";
#elif defined(HAS_GL)
  xmlFilename = "ShaderPresetsGLSLP.xml";
#else
  xmlFilename = "ShaderPresetsHLSLP.xml";
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

    const TiXmlNode* pathNode{GetFirstChildOfNode(*child, "path")};
    if (pathNode)
      videoFilter.path =
          URIUtils::AddFileToFolder(URIUtils::GetBasePath(xmlPath), pathNode->Value());

    const TiXmlNode* nameNode{GetFirstChildOfNode(*child, "name")};
    if (nameNode)
      videoFilter.name = nameNode->Value();

    const TiXmlNode* folderNode{GetFirstChildOfNode(*child, "folder")};
    if (folderNode)
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

    auto item{std::make_shared<CFileItem>(videoFilter.name)};
    item->SetLabel2(videoFilter.folder);
    item->SetProperty("game.videofilter", CVariant{videoFilter.path});
    item->SetArt("icon", ICON_VIDEO);

    m_items.Add(std::move(item));
  }
}

void CDialogGameVideoFilter::InitGetMoreButton()
{
  using namespace ADDON;

  CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();

  // Show the "Get more" icon if add-on is disabled or missing
  bool showGetMore = false;

  if (addonManager.IsAddonInstalled(PRESETS_ADDON_NAME))
  {
    if (addonManager.IsAddonDisabled(PRESETS_ADDON_NAME))
      showGetMore = true;
  }
  else
  {
    // Check if the add-on is in a remote repo
    VECADDONS addons;
    if (addonManager.GetInstallableAddons(addons, AddonType::SHADERDLL) && !addons.empty())
      showGetMore = true;
  }

  if (showGetMore)
  {
    auto item = std::make_shared<CFileItem>(g_localizeStrings.Get(21452)); // "Get more..."
    item->SetArt("icon", ICON_GET_MORE);
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
    m_focusedItemIndex = index;

    CFileItemPtr item = m_items[index];

    std::string videoFilter;
    GetProperties(*item, videoFilter);
    if (videoFilter.empty())
      return;

    ::CGameSettings& gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

    if (gameSettings.VideoFilter() != videoFilter)
    {
      gameSettings.SetVideoFilter(videoFilter);
      gameSettings.NotifyObservers(ObservableMessageSettingsChanged);
    }
  }
}

unsigned int CDialogGameVideoFilter::GetFocusedItem() const
{
  const ::CGameSettings& gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  for (int i = 0; i < m_items.Size(); i++)
  {
    std::string videoFilter;
    GetProperties(*m_items[i], videoFilter);

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
  if (static_cast<int>(m_focusedItemIndex) < m_items.Size())
  {
    const auto focusedItem = std::const_pointer_cast<const CFileItem>(m_items[m_focusedItemIndex]);

    // "Get more..." button has an empty string for the video filter
    if (focusedItem && focusedItem->GetProperty("game.videofilter").empty())
    {
      OnGetMore();
      return true;
    }
  }

  Close();
  return true;
}

void CDialogGameVideoFilter::RefreshList()
{
  // Re-generate the items in case the shader preset add-on was installed
  if (m_regenerateList)
  {
    m_regenerateList = false;
    PreInit();
  }

  // Call ancestor
  CDialogGameVideoSelect::RefreshList();
}

std::string CDialogGameVideoFilter::GetLocalizedString(uint32_t code)
{
  return g_localizeStrings.GetAddonString(PRESETS_ADDON_NAME, code);
}

void CDialogGameVideoFilter::OnGetMore()
{
  const std::shared_ptr<CJobManager> jobManager = CServiceBroker::GetJobManager();
  if (!jobManager)
    return;

  jobManager->Submit(
      [this]()
      {
        using namespace ADDON;

        CAddonMgr& addonManager = CServiceBroker::GetAddonMgr();

        bool success = false;
        if (addonManager.IsAddonDisabled(PRESETS_ADDON_NAME))
        {
          success = addonManager.EnableAddon(PRESETS_ADDON_NAME);
        }
        else if (!addonManager.IsAddonInstalled(PRESETS_ADDON_NAME))
        {
          AddonPtr addon;
          success = CAddonInstaller::GetInstance().InstallModal(PRESETS_ADDON_NAME, addon,
                                                                InstallModalPrompt::CHOICE_NO);
        }

        if (success)
          OnGetMoreComplete();
      });
}

void CDialogGameVideoFilter::OnGetMoreComplete()
{
  CGUIComponent* gui = CServiceBroker::GetGUI();
  if (gui == nullptr)
    return;

  // Shader preset add-ons are loaded async, so wait until the new
  // add-on is fully loaded. Bail after a reasonable timeout to avoid
  // waiting indefinitely.
  XbmcThreads::EndTime<> timeout(5s);
  while (!CServiceBroker::GetGameServices().VideoShaders().HasAddons() && !timeout.IsTimePast())
  {
    // Should take on the order of 100-200ms
    KODI::TIME::Sleep(50ms);
  }

  if (timeout.IsTimePast())
    CLog::Log(LOGERROR, "Timed out waiting for shader presets to load");

  if (CServiceBroker::GetGameServices().VideoShaders().HasAddons())
  {
    // Indicate that the new presets should be loaded
    m_regenerateList = true;

    // Send the GUI message to reload the list
    CGUIMessage msg(GUI_MSG_REFRESH_LIST, GetID(), CONTROL_VIDEO_THUMBS);
    gui->GetWindowManager().SendThreadMessage(msg, GetID());
  }
}
