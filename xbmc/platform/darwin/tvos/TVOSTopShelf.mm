/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "TVOSTopShelf.h"

#include "DatabaseManager.h"
#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "application/Application.h"
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
    static const auto isSandboxed = CDarwinEmbedUtils::IsIosSandboxed();
    static const auto storeUrl = [[tvosShared getSharedURL] URLByAppendingPathComponent:@"RA" isDirectory:YES];
    CLog::Log(LOGDEBUG, "[TopShelf] (sandboxed: {}) Use storeUrl: {}", isSandboxed ? "yes" : "no",
              storeUrl.path.UTF8String);

    auto fileManager = NSFileManager.defaultManager;

    // Retrieve TopShelf current categories
    auto sharedSandboxDict = [[NSUserDefaults alloc] initWithSuiteName:[tvosShared getSharedID]];
    static const auto topshelfKey = @"topshelfCategories";
    static const auto sharedJailbreakUrl = [storeUrl URLByAppendingPathComponent:@"topshelf.dict"
                                                                     isDirectory:NO];
    NSMutableDictionary* topshelfCategories;
    if (isSandboxed)
    {
      if ([sharedSandboxDict objectForKey:topshelfKey])
        topshelfCategories = [[sharedSandboxDict objectForKey:topshelfKey] mutableCopy];
      else
        topshelfCategories = [NSMutableDictionary dictionaryWithCapacity:2];
    }
    else
    {
      if ([[NSFileManager defaultManager] fileExistsAtPath:sharedJailbreakUrl.path])
        topshelfCategories = [NSMutableDictionary dictionaryWithContentsOfURL:sharedJailbreakUrl];
      else
        topshelfCategories = [NSMutableDictionary dictionaryWithCapacity:2];
    }

    // Function used to add category items in TopShelf dict
    CVideoThumbLoader thumbLoader;
    auto fillSharedDicts =
        [&](CFileItemList& items, NSString* categoryKey, NSString* categoryTitle,
            const std::function<std::string(CFileItemPtr videoItem)>& getThumbnailForItem,
            const std::function<std::string(CFileItemPtr videoItem)>& getTitleForItem) {
          // Store all old thumbs names of this category in array
          const auto thumbsPath = [storeUrl URLByAppendingPathComponent:categoryKey isDirectory:YES];
          auto thumbsToRemove = [NSMutableSet setWithArray:[fileManager contentsOfDirectoryAtPath:thumbsPath.path error:nil]];
          
          if (items.Size() <= 0)
          {
            // If there is no item in this category, remove it from topshelfCategories
            [topshelfCategories removeObjectForKey:categoryKey];
          }
          else
          {
            // Create dict for this category
            auto categoryDict = [NSMutableDictionary dictionaryWithCapacity:2];

            // Save category title in dict
            categoryDict[@"categoryTitle"] = categoryTitle;

            // Create an array to store each category item
            const int categorySize = std::min(items.Size(), MaxItems);
            auto categoryItems = [NSMutableArray arrayWithCapacity:categorySize];
            for (int i = 0; i < categorySize; ++i)
            {
              @autoreleasepool
              {
                // Get item thumb and title
                auto item = items.Get(i);
                if (!item->HasArt("thumb"))
                  thumbLoader.LoadItem(item.get());

                auto thumbPathSrc = getThumbnailForItem(item);
                auto itemThumb = std::to_string(item->GetVideoInfoTag()->m_iDbId) +
                                URIUtils::GetFileName(thumbPathSrc);
                auto thumbPathDst = URIUtils::AddFileToFolder(thumbsPath.path.UTF8String, itemThumb);
                if (!XFILE::CFile::Exists(thumbPathDst))
                  XFILE::CFile::Copy(thumbPathSrc, thumbPathDst);
                else
                {
                  // Remove from thumbsToRemove array so it doesn't get deleted at the end
                  [thumbsToRemove removeObject:@(itemThumb.c_str())];
                }

                auto itemTitle = getTitleForItem(item);
                
                // Add item object in categoryItems
                CLog::Log(LOGDEBUG, "[TopShelf] Adding item '{}' in category '{}'", itemTitle,
                          categoryKey.UTF8String);
                [categoryItems addObject:@{
                  @"title" : @(itemTitle.c_str()),
                  @"thumb" : @(itemThumb.c_str()),
                  @"url" : @(Base64::Encode(item->GetVideoInfoTag()->GetPath()).c_str())
                }];
              }
            }

            // Store category items array in category dict
            categoryDict[@"categoryItems"] = categoryItems;

            // Store category dict in topshelfCategories
            topshelfCategories[categoryKey] = categoryDict;
          }
          
          // Remove unused thumbs of this TopShelf category from thumbsPath folder
          for (NSString* thumbFileName in thumbsToRemove)
            [fileManager removeItemAtURL:[thumbsPath URLByAppendingPathComponent:thumbFileName isDirectory:NO] error:nil];
        };


    // Based on category type, add items in TopShelf shared dict
    switch (category)
    {
      case TVOSTopShelfItemsCategory::MOVIES:
        fillSharedDicts(
            items, @"movies", @(g_localizeStrings.Get(20386).c_str()),
            [](const CFileItemPtr& videoItem) {
              if (videoItem->HasArt("poster"))
                return videoItem->GetArt("poster");
              else
                return videoItem->GetArt("thumb");
            },
            [](const CFileItemPtr& videoItem) { return videoItem->GetLabel(); });
        break;
      case TVOSTopShelfItemsCategory::TV_SHOWS:
        CVideoDatabase videoDb;
        videoDb.Open();
        fillSharedDicts(
            items, @"tvshows", @(g_localizeStrings.Get(20387).c_str()),
            [&videoDb](const CFileItemPtr& videoItem)
            {
              int season = videoItem->GetVideoInfoTag()->m_iIdSeason;
              return season > 0 ? videoDb.GetArtForItem(season, MediaTypeSeason, "poster")
                                : std::string{};
            },
            [](const CFileItemPtr& videoItem)
            {
              return StringUtils::Format("{} s{:02}e{:02}",
                                         videoItem->GetVideoInfoTag()->m_strShowTitle,
                                         videoItem->GetVideoInfoTag()->m_iSeason,
                                         videoItem->GetVideoInfoTag()->m_iEpisode);
            });
        videoDb.Close();
        break;
    }

    // Set TopShelf new categories
    if (isSandboxed)
    {
      [sharedSandboxDict setObject:topshelfCategories forKey:topshelfKey];
      [sharedSandboxDict synchronize];
    }
    else
      [topshelfCategories writeToURL:sharedJailbreakUrl atomically:YES];
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
  std::string cmd = StringUtils::Format("PlayMedia({})", StringUtils::Paramify(url));
  CServiceBroker::GetAppMessenger()->PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, cmd);
}

void CTVOSTopShelf::HandleTopShelfUrl(const std::string& url, const bool run)
{
  m_url = url;
  m_handleUrl = run;
}
