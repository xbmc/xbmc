/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "filesystem/Directory.h"
#include "filesystem/File.h"
#include "filesystem/MultiPathDirectory.h"
#include "guilib/test/TestGUIStubs.h"
#include "pictures/GUIWindowPictures.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"

#include <string>
#include <vector>

#include <gtest/gtest.h>

namespace
{
class CTestGUIWindowPictures : public CGUIWindowPictures
{
public:
  using CGUIWindowPictures::OnPrepareFileItems;
};

// Lists a multipath source of the given number of folders, each holding a picture and a folder.jpg
void ExpectNoFolderThumbs(int folderCount)
{
  const std::string root = "special://temp/multipathpictures/";
  std::vector<std::string> folders;
  for (int i = 1; i <= folderCount; ++i)
  {
    const std::string folder = root + std::to_string(i) + "/";
    ASSERT_TRUE(XFILE::CDirectory::Create(folder));
    for (const std::string& name :
         {std::string("folder.jpg"), "picture" + std::to_string(i) + ".jpg"})
    {
      XFILE::CFile file;
      ASSERT_TRUE(file.OpenForWrite(folder + name, true));
    }
    folders.push_back(folder);
  }

  CFileItemList items;
  ASSERT_TRUE(
      XFILE::CDirectory::GetDirectory(XFILE::CMultiPathDirectory::ConstructMultiPath(folders),
                                      items, "", XFILE::DIR_FLAG_DEFAULTS));
  XFILE::CDirectory::RemoveRecursive(root);
  ASSERT_EQ(items.Size(), 2 * folderCount);

  // Stops before the tag loading that follows the filtering
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const bool useTags = settings->GetBool(CSettings::SETTING_PICTURES_USETAGS);
  settings->SetBool(CSettings::SETTING_PICTURES_USETAGS, false);
  // The window's view state asks the window manager for the active window
  KODI::GUILIB::TEST::CTestGUIComponent gui;
  {
    CTestGUIWindowPictures window;
    window.OnPrepareFileItems(items);
  }
  CServiceBroker::UnregisterGUI();
  settings->SetBool(CSettings::SETTING_PICTURES_USETAGS, useTags);

  ASSERT_EQ(items.Size(), folderCount);
  for (const auto& item : items)
  {
    EXPECT_FALSE(StringUtils::EqualsNoCase(item->GetLabel(), "folder.jpg"));
  }
}
} // namespace

TEST(TestGUIWindowPictures, FolderThumbsLeaveAMultiPathListing)
{
  ExpectNoFolderThumbs(2);
  ExpectNoFolderThumbs(3);
}
