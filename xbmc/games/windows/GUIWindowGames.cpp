/*
 *  Copyright (C) 2012-2020 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GUIWindowGames.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "GUIPassword.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "Util.h"
#include "addons/gui/GUIDialogAddonInfo.h"
#include "dialogs/GUIDialogContextMenu.h"
#include "dialogs/GUIDialogMediaSource.h"
#include "dialogs/GUIDialogProgress.h"
#include "dialogs/GUIDialogSelect.h"
#include "filesystem/FileDirectoryFactory.h"
#include "games/GameServices.h"
#include "games/GameSettings.h"
#include "games/GameUtils.h"
#include "games/library/GameNameInfo.h"
#include "games/library/GameNameParser.h"
#include "games/library/GameVersions.h"
#include "games/library/ReleasePolicy.h"
#include "games/tags/GameInfoTag.h"
#include "guilib/GUIComponent.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/WindowIDs.h"
#include "input/actions/ActionIDs.h"
#include "media/MediaLockState.h"
#include "messaging/ApplicationMessenger.h"
#include "playlists/PlayListFileItemClassify.h"
#include "playlists/PlayListTypes.h"
#include "settings/MediaSourceSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Variant.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

using namespace KODI;
using namespace GAME;
using KODI::MEDIA::MediaSection;

#define CONTROL_BTNVIEWASICONS 2
#define CONTROL_BTNSORTBY 3
#define CONTROL_BTNSORTASC 4

namespace
{
//! The paths of a game's versions, the listed one first, on the item that lists it
const std::string PROPERTY_VERSIONS = "gameversions";

// Lists each game once, as the version the policy picks, and keeps the paths
// of all its versions for "Choose version"
void ShowOneVersionPerGame(CFileItemList& items,
                           const std::vector<CFileItemPtr>& files,
                           const std::vector<ParsedGameName>& names,
                           const CReleasePolicy& policy)
{
  for (const std::vector<size_t>& versions : CGameVersions::Group(names, policy))
  {
    CVariant paths(CVariant::VariantTypeArray);
    for (size_t version : versions)
      paths.push_back(files[version]->GetPath());
    files[versions.front()]->SetProperty(PROPERTY_VERSIONS, paths);

    for (size_t i = 1; i < versions.size(); ++i)
      items.Remove(files[versions[i]].get());
  }
}
} // namespace

CGUIWindowGames::CGUIWindowGames() : CGUIMediaWindow(WINDOW_GAMES, "MyGames.xml")
{
}

bool CGUIWindowGames::OnMessage(CGUIMessage& message)
{
  switch (message.GetMessage())
  {
    case GUI_MSG_WINDOW_INIT:
    {
      m_rootDir.AllowNonLocalSources(true);

      // Is this the first time the window is opened?
      if (m_vecItems->GetPath() == "?" && message.GetStringParam().empty())
        message.SetStringParam(
            CMediaSourceSettings::GetInstance().GetDefaultSource(MediaSection::GAMES));

      m_dlgProgress = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogProgress>(
          WINDOW_DIALOG_PROGRESS);

      break;
    }
    case GUI_MSG_CLICKED:
    {
      if (OnClickMsg(message.GetSenderId(), message.GetParam1()))
        return true;
      break;
    }
    default:
      break;
  }
  return CGUIMediaWindow::OnMessage(message);
}

bool CGUIWindowGames::OnClickMsg(int controlId, int actionId)
{
  if (!m_viewControl.HasControl(controlId)) // list/thumb control
    return false;

  const int iItem = m_viewControl.GetSelectedItem();

  CFileItemPtr pItem = m_vecItems->Get(iItem);
  if (!pItem)
    return false;

  switch (actionId)
  {
    case ACTION_DELETE_ITEM:
    {
      // Is delete allowed?
      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
              CSettings::SETTING_FILELISTS_ALLOWFILEDELETION))
      {
        OnDeleteItem(iItem);
        return true;
      }
      break;
    }
    case ACTION_PLAYER_PLAY:
    {
      if (CanPlay(*pItem))
      {
        PlayGame(*pItem);
        return true;
      }
      break;
    }
    case ACTION_SHOW_INFO:
    {
      if (!m_vecItems->IsPlugin())
      {
        if (pItem->HasAddonInfo())
        {
          CGUIDialogAddonInfo::ShowForItem(pItem);
          return true;
        }
      }
      break;
    }
    default:
      break;
  }

  return false;
}

void CGUIWindowGames::SetupShares()
{
  CGUIMediaWindow::SetupShares();

  // Don't convert zip files to directories. Otherwise, the files will be
  // opened and scanned for games with a valid extension. If none are found,
  // the .zip won't be shown.
  //
  // This is a problem for MAME roms, because the files inside the .zip don't
  // have standard extensions.
  //
  m_rootDir.SetFlags(XFILE::DIR_FLAG_NO_FILE_DIRS);
}

bool CGUIWindowGames::OnClick(int iItem, const std::string& player /* = "" */)
{
  CFileItemPtr item = m_vecItems->Get(iItem);
  if (item)
  {
    if (!item->IsFolder())
    {
      PlayGame(*item);
      return true;
    }
  }

  return CGUIMediaWindow::OnClick(iItem, player);
}

void CGUIWindowGames::GetContextButtons(int itemNumber, CContextButtons& buttons)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);

  if (item && !item->GetProperty("pluginreplacecontextitems").asBoolean())
  {
    if (m_vecItems->IsVirtualDirectoryRoot() || m_vecItems->IsSourcesPath())
    {
      // Context buttons for a sources path, like "Add Source", "Remove Source", etc.
      CGUIDialogContextMenu::GetContextButtons(MediaSection::GAMES, item, buttons);
    }
    else
    {
      if (CanPlay(*item))
      {
        buttons.Add(CONTEXT_BUTTON_PLAY_ITEM, 208); // Play
      }

      if (item->GetProperty(PROPERTY_VERSIONS).size() > 1)
        buttons.Add(CONTEXT_BUTTON_CHOOSE_GAME_VERSION, 35724); // "Choose version"

      // Offered on folders as well as games: setting one on a folder is the
      // point, and a game only overrides the folder it sits in
      if (item->IsFolder() || CanPlay(*item))
      {
        buttons.Add(CONTEXT_BUTTON_SET_DEFAULT_EMULATOR, 35510); // "Default emulator"
      }

      if (CServiceBroker::GetSettingsComponent()->GetSettings()->GetBool(
              CSettings::SETTING_FILELISTS_ALLOWFILEDELETION) &&
          !item->IsReadOnly())
      {
        buttons.Add(CONTEXT_BUTTON_DELETE, 117);
        buttons.Add(CONTEXT_BUTTON_RENAME, 118);
      }
    }
  }

  CGUIMediaWindow::GetContextButtons(itemNumber, buttons);
}

bool CGUIWindowGames::OnContextButton(int itemNumber, CONTEXT_BUTTON button)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (item)
  {
    if (m_vecItems->IsVirtualDirectoryRoot() || m_vecItems->IsSourcesPath())
    {
      if (CGUIDialogContextMenu::OnContextButton(MediaSection::GAMES, item, button))
      {
        Update(m_vecItems->GetPath());
        return true;
      }
    }
    switch (button)
    {
      case CONTEXT_BUTTON_PLAY_ITEM:
        PlayGame(*item);
        return true;
      case CONTEXT_BUTTON_SET_DEFAULT_EMULATOR:
        CGameUtils::ChooseAndSetDefaultGameClient(*item);
        return true;
      case CONTEXT_BUTTON_CHOOSE_GAME_VERSION:
        ChooseVersionAndPlay(*item);
        return true;
      case CONTEXT_BUTTON_INFO:
        CGUIDialogAddonInfo::ShowForItem(item);
        return true;
      case CONTEXT_BUTTON_DELETE:
        OnDeleteItem(itemNumber);
        return true;
      case CONTEXT_BUTTON_RENAME:
        OnRenameItem(itemNumber);
        return true;
      default:
        break;
    }
  }
  return CGUIMediaWindow::OnContextButton(itemNumber, button);
}

bool CGUIWindowGames::OnAddMediaSource()
{
  return CGUIDialogMediaSource::ShowAndAddMediaSource(MediaSection::GAMES);
}

bool CGUIWindowGames::GetDirectory(const std::string& strDirectory, CFileItemList& items)
{
  if (!CGUIMediaWindow::GetDirectory(strDirectory, items))
    return false;

  // Now we must account for file folders not handled by DIR_FLAG_NO_FILE_DIRS
  for (int i = 0; i < items.Size(); ++i)
  {
    CFileItemPtr item = items[i];
    if (item->IsFolder() || !item->IsFileFolder(FileFolderType::ALWAYS))
      continue;

    const std::string originalPath = item->GetPath();

    // This will turn the item into a file folder as a side effect
    std::unique_ptr<XFILE::IFileDirectory> pDirectory{
        XFILE::CFileDirectoryFactory::Create(CURL{originalPath}, item.get())};
    if (pDirectory)
    {
      // Check for empty file folders
      CFileItemList fileFolderItems;
      if (!pDirectory->GetDirectory(item->GetURL(), fileFolderItems) || fileFolderItems.IsEmpty())
      {
        items.Remove(i);
        --i;
        continue;
      }

      // Check if file folder contains games or subfolders
      if (std::ranges::any_of(fileFolderItems,
                              [](const CFileItemPtr& fileFolderItem) {
                                return fileFolderItem->IsFolder() ||
                                       CGameUtils::HasGameExtension(fileFolderItem->GetPath());
                              }))
      {
        continue;
      }

      // If the file folder contains no games, turn it back into a regular file
      item->SetFolder(false);
      item->SetPath(originalPath);
    }
    else
    {
      // File folder contains a single file and was collapsed down. Remove it
      // if not a game.
      if (!CGameUtils::HasGameExtension(item->GetPath()))
      {
        items.Remove(i);
        --i;
      }
    }
  }

  // Set label
  std::string label;
  if (items.GetLabel().empty())
  {
    std::string source;
    if (m_rootDir.IsSource(items.GetPath(),
                           &CMediaSourceSettings::GetInstance().GetSources(MediaSection::GAMES),
                           &source))
      label = std::move(source);
  }

  if (!label.empty())
    items.SetLabel(label);

  // Set content
  std::string content;
  if (items.GetContent().empty())
  {
    if (!items.IsVirtualDirectoryRoot() && // Don't set content for root directory
        !items.IsPlugin()) // Don't set content for plugins
    {
      content = "games";
    }
  }

  if (!content.empty())
    items.SetContent(content);

  // Plugins and add-ons give their own titles
  const bool namedByFile = !items.IsPlugin() && !items.IsAddonsPath();
  CGameSettings& gameSettings = CServiceBroker::GetGameServices().GameSettings();
  const bool replaceLabels = namedByFile && gameSettings.ReplaceLabels();

  std::vector<CFileItemPtr> files;
  std::vector<ParsedGameName> names;

  for (const CFileItemPtr& item : items)
  {
    if (item->IsFolder())
      continue;

    // Ensure a game info tag is created so that files are recognized as games
    CGameInfoTag& tag = *item->GetGameInfoTag();
    if (namedByFile && tag.GetTitle().empty())
    {
      ParsedGameName name = CGameNameParser::ParsePath(item->GetPath());
      CGameNameInfo::SetInfoTag(tag, name);
      if (replaceLabels && !tag.GetTitle().empty())
        item->SetLabel(tag.GetTitle());
      files.emplace_back(item);
      names.emplace_back(std::move(name));
    }
  }

  // Listed by file name, every version stays: one name can't stand for the others
  if (replaceLabels && gameSettings.OneVersionPerGame())
  {
    const CReleasePolicy policy(CReleasePolicy::RegionPriority(
        {GameRegion::Make(gameSettings.Region()), GameRegion::Make(gameSettings.SecondRegion())}));
    ShowOneVersionPerGame(items, files, names, policy);
  }

  return true;
}

std::string CGUIWindowGames::GetStartFolder(const std::string& dir)
{
  // From CGUIWindowPictures::GetStartFolder()

  if (StringUtils::EqualsNoCase(dir, "plugins") || StringUtils::EqualsNoCase(dir, "addons"))
  {
    return "addons://sources/game/";
  }

  SetupShares();
  std::vector<CMediaSource> shares;
  m_rootDir.GetSources(shares);
  bool bIsSourceName = false;
  int iIndex = CUtil::GetMatchingSource(dir, shares, bIsSourceName);
  if (iIndex >= 0)
  {
    if (iIndex < static_cast<int>(shares.size()) && shares[iIndex].GetLockInfo().IsLocked())
    {
      CFileItem item(shares[iIndex]);
      if (!g_passwordManager.IsItemUnlocked(&item, MediaSection::GAMES))
        return "";
    }
    if (bIsSourceName)
      return shares[iIndex].strPath;
    return dir;
  }
  return CGUIMediaWindow::GetStartFolder(dir);
}

void CGUIWindowGames::OnItemInfo(int itemNumber)
{
  CFileItemPtr item = m_vecItems->Get(itemNumber);
  if (!item)
    return;

  if (!m_vecItems->IsPlugin())
  {
    if (item->IsPlugin() || item->IsScript())
      CGUIDialogAddonInfo::ShowForItem(item);
  }

}

void CGUIWindowGames::ChooseVersionAndPlay(const CFileItem& item)
{
  const CVariant paths = item.GetProperty(PROPERTY_VERSIONS);

  CFileItemList versions;
  for (auto it = paths.begin_array(); it != paths.end_array(); ++it)
  {
    const std::string path = it->asString();
    const ParsedGameName name = CGameNameParser::ParsePath(path);
    auto version = std::make_shared<CFileItem>(path, false);
    CGameNameInfo::SetInfoTag(*version->GetGameInfoTag(), name);
    version->SetLabel(CGameNameInfo::VersionLabel(name));
    version->SetLabel2(URIUtils::GetFileName(path));
    versions.Add(version);
  }
  if (versions.IsEmpty())
    return;

  auto* dialog = CServiceBroker::GetGUI()->GetWindowManager().GetWindow<CGUIDialogSelect>(
      WINDOW_DIALOG_SELECT);
  if (dialog == nullptr)
    return;

  dialog->Reset();
  dialog->SetHeading(CVariant{35724}); // "Choose version"
  dialog->SetItems(versions);
  dialog->SetUseDetails(true);
  dialog->SetSelected(0);
  dialog->Open();

  if (dialog->IsConfirmed() && dialog->GetSelectedFileItem())
    PlayGame(*dialog->GetSelectedFileItem());
}

bool CGUIWindowGames::PlayGame(const CFileItem& item)
{
  CFileItem itemCopy(item);

  // Dereference file folders. The check here assumes all "is folder" items
  // passed CanPlay(), e.g. are definitely file folders.
  if (itemCopy.IsFolder())
  {
    itemCopy.SetFolder(false);
    itemCopy.SetPath(itemCopy.GetURL().GetHostName());
    itemCopy.GetGameInfoTag();
  }

  PLAYLIST::Id playlistId = PLAYLIST::Id::TYPE_NONE;
  if (PLAYLIST::IsPlayList(item))
    playlistId = PLAYLIST::Id::TYPE_GAME;

  CServiceBroker::GetAppMessenger()->PostMsg(
      TMSG_APPLICATION_PLAY_MEDIA, static_cast<int>(playlistId), -1, new CFileItem(itemCopy), "");
  return true;
}

bool CGUIWindowGames::CanPlay(const CFileItem& item) const
{
  if (item.IsGame())
    return true;

  if (item.IsFolder())
  {
    // Check for file folders
    CURL url{item.GetPath()};
    if (url.GetFileName().empty() && URIUtils::IsZIP(url.GetHostName()))
      return true;
  }

  return false;
}
