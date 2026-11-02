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
#include "guilib/GUIBaseContainer.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIMessage.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/Action.h"
#include "input/actions/ActionIDs.h"
#include "jobs/JobManager.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/GameSettings.h"
#include "settings/MediaSettings.h"
#include "threads/SystemClock.h"
#include "utils/ArtTypes.h"
#include "utils/DefaultArt.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"
#include "utils/XBMCTinyXML2.h"
#include "utils/log.h"

#include <algorithm>
#include <stdlib.h>

using namespace KODI;
using namespace GAME;
using namespace std::chrono_literals;

namespace
{

constexpr const char* PRESETS_ADDON_NAME = "game.shader.presets";
constexpr const char* ICON_VIDEO = "";

// Window property telling the skin which way the filters moved to another
// category: "next" or "previous"
constexpr const char* PROPERTY_CATEGORY_MOVE = "categorymove";

// Long enough for a skin to slide the new filters in, as Kodi resets an
// animation whose condition ends before it does
constexpr auto CATEGORY_MOVE_TIME = 400ms;

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

std::string GetCategory(const CFileItem& item)
{
  RETRO::CRenderVideoSettings videoSettings;
  videoSettings.SetVideoFilter(item.GetProperty("game.videofilter").asString());

  // Shader presets are grouped by their category. The scaling methods, and the
  // "Get more..." button, share a category of their own.
  if (!videoSettings.GetShaderPreset().empty())
    return item.GetLabel2();

  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(16298); // "Built-in"
}

} // namespace

CDialogGameVideoFilter::CDialogGameVideoFilter()
  : CDialogGameVideoSelect(WINDOW_DIALOG_GAME_VIDEO_FILTER)
{
}

std::string CDialogGameVideoFilter::GetHeading()
{
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(35225); // "Video filter"
}

bool CDialogGameVideoFilter::OnMessage(CGUIMessage& message)
{
  if (message.GetMessage() == GUI_MSG_CLICKED &&
      message.GetSenderId() == static_cast<int>(CONTROL_VIDEO_CATEGORIES))
  {
    const int actionId = message.GetParam1();
    if (actionId == ACTION_SELECT_ITEM || actionId == ACTION_MOUSE_LEFT_CLICK)
    {
      // A tap moves focus on before FrameMove() sees the category it chose
      UpdateCategory();
      m_choosingCategory = true;
      SET_CONTROL_FOCUS(CONTROL_VIDEO_THUMBS, 0);
      return true;
    }
  }

  return CDialogGameVideoSelect::OnMessage(message);
}

bool CDialogGameVideoFilter::OnAction(const CAction& action)
{
  if ((action.GetID() == ACTION_MOVE_RIGHT && MoveAcrossCategories(true)) ||
      (action.GetID() == ACTION_MOVE_LEFT && MoveAcrossCategories(false)))
    return true;

  return CDialogGameVideoSelect::OnAction(action);
}

void CDialogGameVideoFilter::FrameMove()
{
  if (m_categoryMoveDelay > 0)
  {
    if (--m_categoryMoveDelay == 0)
    {
      SetProperty(PROPERTY_CATEGORY_MOVE, m_categoryMove);
      m_categoryMoveStart = std::chrono::steady_clock::now();
    }
  }
  else if (!m_categoryMove.empty() &&
           std::chrono::steady_clock::now() - m_categoryMoveStart >= CATEGORY_MOVE_TIME)
  {
    m_categoryMove.clear();
    SetProperty(PROPERTY_CATEGORY_MOVE, "");
  }

  const CGUIControl* categories = GetControl(CONTROL_VIDEO_CATEGORIES);
  const CGUIControl* filters = GetControl(CONTROL_VIDEO_THUMBS);
  if (categories != nullptr && categories->HasFocus())
  {
    UpdateCategory();
    m_choosingCategory = true;
  }
  else if (m_choosingCategory && filters != nullptr && filters->HasFocus())
  {
    // Moving down from the categories applies the highlighted filter, which a
    // refreshed list leaves unapplied
    m_choosingCategory = false;
    m_focusedItemIndex = static_cast<unsigned int>(m_shownItems.size());
  }

  CDialogGameVideoSelect::FrameMove();
}

void CDialogGameVideoFilter::UpdateCategory()
{
  const auto* categories =
      dynamic_cast<const CGUIBaseContainer*>(GetControl(CONTROL_VIDEO_CATEGORIES));
  if (categories == nullptr)
    return;

  const int categoryIndex = categories->GetSelectedItem();
  if (categoryIndex >= 0 && static_cast<unsigned int>(categoryIndex) != m_categoryIndex &&
      static_cast<unsigned int>(categoryIndex) < m_categories.size())
  {
    m_categoryIndex = categoryIndex;
    RefreshList();
  }
}

bool CDialogGameVideoFilter::MoveAcrossCategories(bool forward)
{
  if (!HasCategories() || m_categories.size() < 2 || m_shownItems.empty())
    return false;

  const auto* filters = dynamic_cast<const CGUIBaseContainer*>(GetControl(CONTROL_VIDEO_THUMBS));
  if (filters == nullptr || !filters->HasFocus())
    return false;

  const int lastIndex = static_cast<int>(m_shownItems.size()) - 1;
  if (filters->GetSelectedItem() != (forward ? lastIndex : 0))
    return false;

  // The last category leads back round to the first
  const auto categoryCount = static_cast<unsigned int>(m_categories.size());
  m_categoryIndex = (m_categoryIndex + (forward ? 1 : categoryCount - 1)) % categoryCount;

  CGUIMessage selectCategory(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_VIDEO_CATEGORIES,
                             m_categoryIndex);
  OnMessage(selectCategory);

  RefreshList();

  // Carry on from the side the player came in from, applying that filter
  const unsigned int index = forward ? 0 : static_cast<unsigned int>(m_shownItems.size()) - 1;
  CGUIMessage selectFilter(GUI_MSG_ITEM_SELECT, GetID(), CONTROL_VIDEO_THUMBS, index);
  OnMessage(selectFilter);
  m_focusedItemIndex = static_cast<unsigned int>(m_shownItems.size());
  OnItemFocus(index);

  AnnounceCategoryMove(forward);

  return true;
}

void CDialogGameVideoFilter::AnnounceCategoryMove(bool forward)
{
  m_categoryMove = forward ? "next" : "previous";

  // Already waiting to tell the skin
  if (m_categoryMoveDelay > 0)
    return;

  // During the last move's slide, the property is cleared first, for long
  // enough that the skin sees it change and starts another
  if (!GetProperty(PROPERTY_CATEGORY_MOVE).asString().empty())
  {
    SetProperty(PROPERTY_CATEGORY_MOVE, "");
    m_categoryMoveDelay = 2;
    return;
  }

  SetProperty(PROPERTY_CATEGORY_MOVE, m_categoryMove);
  m_categoryMoveStart = std::chrono::steady_clock::now();
}

void CDialogGameVideoFilter::OnInitWindow()
{
  CDialogGameVideoSelect::OnInitWindow();

  BindCategories();

  // The highlighted filter is the one in use, so when the configured filter
  // is gone the one highlighted in its place is applied
  m_focusedItemIndex = static_cast<unsigned int>(m_shownItems.size());
}

void CDialogGameVideoFilter::PreInit()
{
  m_items.Clear();

  InitScalingMethods();
  InitVideoFilters();
  InitGetMoreButton();

  if (m_items.Size() == 0)
  {
    CFileItemPtr item = std::make_shared<CFileItem>(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(231)); // "None"
    m_items.Add(std::move(item));
  }

  InitCategories();
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

        CFileItemPtr item = std::make_shared<CFileItem>(
            CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
                scalingMethodProps.nameIndex));
        item->SetLabel2(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
            scalingMethodProps.categoryIndex));
        item->SetProperty("game.videofilter", CVariant{videoSettings.GetVideoFilter()});
        item->SetArt(ART::TYPE::ICON, ICON_VIDEO);
        m_items.Add(std::move(item));
      }
    }
  }
}

namespace
{
const tinyxml2::XMLNode* GetFirstChildOfNode(const tinyxml2::XMLNode& node, const char* childName)
{
  const tinyxml2::XMLNode* ret{node.FirstChildElement(childName)};
  if (ret)
    return ret->FirstChild();

  return ret;
}
} // unnamed namespace

void CDialogGameVideoFilter::InitVideoFilters()
{

  std::vector<VideoFilterProperties> videoFilters;

  std::string xmlPath;
  std::unique_ptr<CXBMCTinyXML2> xml;

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
      xml = std::make_unique<CXBMCTinyXML2>();
      if (xml->LoadFile(xmlPath))
        break;

      CLog::LogF(LOGERROR, "Couldn't load shader presets from XML, {}", CURL::GetRedacted(xmlPath));
      xml.reset();
    }
  }

  if (!xml)
    return;

  const auto* root = xml->RootElement();

  for (const tinyxml2::XMLNode* child = root->FirstChild(); child; child = child->NextSibling())
  {
    VideoFilterProperties videoFilter;

    if (child->FirstChild() == nullptr)
      continue;

    const tinyxml2::XMLNode* pathNode{GetFirstChildOfNode(*child, "path")};
    if (pathNode)
      videoFilter.path =
          URIUtils::AddFileToFolder(URIUtils::GetBasePath(xmlPath), pathNode->Value());

    const tinyxml2::XMLNode* nameNode{GetFirstChildOfNode(*child, "name")};
    if (nameNode)
      videoFilter.name = nameNode->Value();

    // Presets named their category "folder" before
    const tinyxml2::XMLNode* categoryNode{GetFirstChildOfNode(*child, "category")};
    if (!categoryNode)
      categoryNode = GetFirstChildOfNode(*child, "folder");
    if (categoryNode)
      videoFilter.category = categoryNode->Value();

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
    item->SetLabel2(videoFilter.category);
    item->SetProperty("game.videofilter", CVariant{videoFilter.path});
    item->SetArt(ART::TYPE::ICON, ICON_VIDEO);

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
    auto item = std::make_shared<CFileItem>(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21452)); // "Get more..."
    item->SetArt(ART::TYPE::ICON, ART::DEFAULT::ADD_SOURCE);
    m_items.Add(std::move(item));
  }
}

void CDialogGameVideoFilter::InitCategories()
{
  m_categories.clear();
  m_categoryIndex = 0;

  const ::CGameSettings& gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  for (const auto& item : m_items)
  {
    std::string category = GetCategory(*item);

    auto it = std::ranges::find(m_categories, category);
    if (it == m_categories.end())
      it = m_categories.insert(m_categories.end(), std::move(category));

    std::string videoFilter;
    GetProperties(*item, videoFilter);
    if (videoFilter == gameSettings.VideoFilter())
      m_categoryIndex = static_cast<unsigned int>(std::distance(m_categories.begin(), it));
  }
}

void CDialogGameVideoFilter::BindCategories()
{
  if (!HasCategories())
    return;

  CFileItemList items;
  for (const auto& category : m_categories)
    items.Add(std::make_shared<CFileItem>(category));

  CGUIMessage msg(GUI_MSG_LABEL_BIND, GetID(), CONTROL_VIDEO_CATEGORIES, m_categoryIndex, 0,
                  &items);
  OnMessage(msg);
}

bool CDialogGameVideoFilter::HasCategories()
{
  return GetControl(CONTROL_VIDEO_CATEGORIES) != nullptr;
}

void CDialogGameVideoFilter::GetItems(CFileItemList& items)
{
  m_shownItems.clear();

  // Skins without a category list show every filter in one row
  const bool showAll = !HasCategories();

  for (const auto& item : m_items)
  {
    if (showAll || GetCategory(*item) == m_categories[m_categoryIndex])
    {
      items.Add(item);
      m_shownItems.push_back(item);
    }
  }
}

void CDialogGameVideoFilter::OnItemFocus(unsigned int index)
{
  // A filter is applied when the player moves to it. A refreshed list, such as
  // another category's, can leave one highlighted that isn't in use.
  if (index >= m_shownItems.size() || index == m_focusedItemIndex)
    return;

  m_focusedItemIndex = index;
  if (!m_refreshingList)
    ApplyVideoFilter(*m_shownItems[index]);
}

void CDialogGameVideoFilter::ApplyVideoFilter(const CFileItem& item)
{
  std::string videoFilter;
  GetProperties(item, videoFilter);
  if (videoFilter.empty())
    return;

  ::CGameSettings& gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  if (gameSettings.VideoFilter() != videoFilter)
  {
    gameSettings.SetVideoFilter(videoFilter);
    gameSettings.NotifyObservers(ObservableMessageSettingsChanged);
  }
}

unsigned int CDialogGameVideoFilter::GetFocusedItem() const
{
  const ::CGameSettings& gameSettings = CMediaSettings::GetInstance().GetCurrentGameSettings();

  for (unsigned int i = 0; i < m_shownItems.size(); i++)
  {
    std::string videoFilter;
    GetProperties(*m_shownItems[i], videoFilter);

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
  m_shownItems.clear();
  m_categories.clear();
  m_choosingCategory = false;
  m_categoryMove.clear();
  m_categoryMoveDelay = 0;
  SetProperty(PROPERTY_CATEGORY_MOVE, "");
}

bool CDialogGameVideoFilter::OnClickAction()
{
  if (m_focusedItemIndex < m_shownItems.size())
  {
    const auto focusedItem =
        std::const_pointer_cast<const CFileItem>(m_shownItems[m_focusedItemIndex]);

    // "Get more..." button has an empty string for the video filter
    if (focusedItem && focusedItem->GetProperty("game.videofilter").empty())
    {
      OnGetMore();
      return true;
    }

    if (focusedItem)
      ApplyVideoFilter(*focusedItem);
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
    BindCategories();
  }

  // Call ancestor
  m_refreshingList = true;
  CDialogGameVideoSelect::RefreshList();
  m_refreshingList = false;
}

std::string CDialogGameVideoFilter::GetLocalizedString(uint32_t code)
{
  return CServiceBroker::GetResourcesComponent().GetLocalizeStrings().GetAddonString(
      PRESETS_ADDON_NAME, code);
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
