/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


#import "TVOSTopShelf.h"

#include "Application.h"
#include "DatabaseManager.h"
#include "FileItem.h"
#include "filesystem/File.h"
#include "guilib/GUIWindowManager.h"
#include "guilib/LocalizeStrings.h"
#include "messaging/ApplicationMessenger.h"
#include "tvosShared.h"
#include "utils/Base64.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoInfoTag.h"
#include "video/VideoThumbLoader.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoBase.h"
#include "video/windows/GUIWindowVideoNav.h"

#include "platform/darwin/ios-common/DarwinEmbedUtils.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <mach/mach_host.h>
#import <sys/sysctl.h>

#import "system.h"

static const int MaxItems = 5;

std::string CTVOSTopShelf::m_url;
bool CTVOSTopShelf::m_handleUrl;

CTVOSTopShelf& CTVOSTopShelf::GetInstance()
{
  static CTVOSTopShelf sTopShelf;
  return sTopShelf;
}

void CTVOSTopShelf::SetTopShelfItems(CFileItemList& items, TVOSTopShelfItemsCategory category)
{
  @autoreleasepool
  {
    // Retrieve store URL
    auto storeUrl = [tvosShared getSharedURL];
    if (!storeUrl)
      return;
    storeUrl = [storeUrl URLByAppendingPathComponent:@"RA" isDirectory:YES];

    const auto isSandboxed = CDarwinEmbedUtils::IsIosSandboxed();
    CLog::Log(LOGDEBUG, "[TopShelf] (sandboxed: {}) Use storeUrl: {}", isSandboxed ? "yes" : "no",
              storeUrl.path.UTF8String);

    // store all old thumbs in array
    auto fileManager = NSFileManager.defaultManager;
    auto filePaths =
        [NSMutableSet setWithArray:[fileManager contentsOfDirectoryAtPath:storeUrl.path error:nil]];
    std::string raPath = storeUrl.path.UTF8String;

    // Shared dicts  (if we are sandboxed we use sharedDefaults, else we use sharedJailbreak)
    auto sharedDefaults =
        isSandboxed ? [[NSUserDefaults alloc] initWithSuiteName:[tvosShared getSharedID]] : nil;
    auto sharedJailbreak = isSandboxed ? nil : [[NSMutableDictionary alloc] initWithCapacity:2];

    // Function used to add category items in TopShelf shared dict
    CVideoThumbLoader thumbLoader;
    auto fillSharedDicts =
        [&](CFileItemList& items, NSString* categoryKey, uint32_t categoryTitleCode,
            std::function<std::string(CFileItemPtr videoItem)> getThumbnailForItem,
            std::function<std::string(CFileItemPtr videoItem)> getTitleForItem) {
          if (items.Size() <= 0)
          {
            // If there is no item in this category, remove this dict from sharedDefaults
            [sharedDefaults removeObjectForKey:categoryKey];
            return;
          }

          // Create dict for this category
          auto categoryDict = [NSMutableDictionary dictionaryWithCapacity:2];

          // Save category title in dict
          categoryDict[@"categoryTitle"] = @(g_localizeStrings.Get(categoryTitleCode).c_str());

          // Create an array to store each category item
          const int categorySize = std::min(items.Size(), MaxItems);
          auto categoryItems = [NSMutableArray arrayWithCapacity:categorySize];
          for (int i = 0; i < categorySize; ++i)
          {
            @autoreleasepool
            {
              auto item = items.Get(i);
              if (!item->HasArt("thumb"))
                thumbLoader.LoadItem(item.get());

              auto thumbnailPath = getThumbnailForItem(item);
              auto fileName = std::to_string(item->GetVideoInfoTag()->m_iDbId) +
                              URIUtils::GetFileName(thumbnailPath);
              auto destPath = URIUtils::AddFileToFolder(raPath, fileName);
              if (!XFILE::CFile::Exists(destPath))
                XFILE::CFile::Copy(thumbnailPath, destPath);
              else
              {
                // Remove from array so it doesn't get deleted at the end
                [filePaths removeObject:@(fileName.c_str())];
              }

              auto itemTitle = getTitleForItem(item);
              CLog::Log(LOGDEBUG, "[TopShelf] Adding item '{}' in category '{}'", itemTitle.c_str(),
                        categoryKey.UTF8String);
              [categoryItems addObject:@{
                @"title" : @(itemTitle.c_str()),
                @"thumb" : @(fileName.c_str()),
                @"url" : @(Base64::Encode(item->GetVideoInfoTag()->GetPath()).c_str())
              }];
            }
          }

          // Store category items array in category dict
          categoryDict[@"categoryItems"] = categoryItems;

          // Store category dict in shared dict
          [sharedDefaults setObject:categoryDict forKey:categoryKey];
          sharedJailbreak[categoryKey] = categoryDict;
        };


    // Based on category type, add items in TopShelf shared dict
    switch (category)
    {
      case TVOSTopShelfItemsCategory::MOVIES:
        fillSharedDicts(
            items, @"movies", 20386,
            [](CFileItemPtr videoItem) {
              if (videoItem->HasArt("poster"))
                return videoItem->GetArt("poster");
              else
                return videoItem->GetArt("thumb");
            },
            [](CFileItemPtr videoItem) { return videoItem->GetLabel(); });
        break;
      case TVOSTopShelfItemsCategory::TV_SHOWS:
        CVideoDatabase videoDb;
        videoDb.Open();
        fillSharedDicts(
            items, @"tvshows", 20387,
            [&videoDb](CFileItemPtr videoItem) {
              int season = videoItem->GetVideoInfoTag()->m_iIdSeason;
              return season > 0 ? videoDb.GetArtForItem(season, MediaTypeSeason, "poster")
                                : std::string{};
            },
            [](CFileItemPtr videoItem) {
              return StringUtils::Format("%s s%02de%02d",
                                         videoItem->GetVideoInfoTag()->m_strShowTitle.c_str(),
                                         videoItem->GetVideoInfoTag()->m_iSeason,
                                         videoItem->GetVideoInfoTag()->m_iEpisode);
            });
        videoDb.Close();
        break;
    }

    // Remove unused thumbs from cache folder
    NSString* strFiles;
    for (strFiles in filePaths)
      [fileManager removeItemAtURL:[storeUrl URLByAppendingPathComponent:strFiles isDirectory:NO]
                             error:nil];

    // Synchronize shared dict
    [sharedDefaults synchronize];
    [sharedJailbreak writeToURL:[storeUrl URLByAppendingPathComponent:@"shared.dict"]
                     atomically:YES];
  }
}

void CTVOSTopShelf::RunTopShelf()
{
  if (!m_handleUrl)
    return;

  m_handleUrl = false;
  std::vector<std::string> split = StringUtils::Split(m_url, "/");
  std::string url = Base64::Decode(split[4]);

  //!@Todo handle tvcontentitem.displayurl. Should show topshelf item video info
  //  check split[2] for url type (display or play)

  // its a bit ugly, but only way to get resume window to show
  std::string cmd =
      StringUtils::Format("PlayMedia(%s)", StringUtils::Paramify(url.c_str()).c_str());
  KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1,
                                                                nullptr, cmd);
}

void CTVOSTopShelf::HandleTopShelfUrl(const std::string& url, const bool run)
{
  m_url = url;
  m_handleUrl = run;
}
