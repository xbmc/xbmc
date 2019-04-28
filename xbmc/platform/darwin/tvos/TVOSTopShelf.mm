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

void CTVOSTopShelf::SetTopShelfItems(CFileItemList& movies, CFileItemList& tv)
{
  @autoreleasepool
  {
    auto storeUrl = [tvosShared getSharedURL];
    if (!storeUrl)
      return;

    storeUrl = [storeUrl URLByAppendingPathComponent:@"RA" isDirectory:YES];
    const BOOL isJailbroken = [tvosShared isJailbroken];
    CLog::Log(LOGDEBUG, "TopShelf: using shared path {} (jailbroken: {})\n",
              storeUrl.path.UTF8String, isJailbroken ? "yes" : "no");

    auto sharedDefaults = [[NSUserDefaults alloc] initWithSuiteName:[tvosShared getSharedID]];
    auto sharedDictJailbreak = isJailbroken ? [[NSMutableDictionary alloc] initWithCapacity:2 + 2]
                                            : nil; // for jailbroken devices

    // store all old thumbs in array
    auto fileManager = NSFileManager.defaultManager;
    auto filePaths =
        [NSMutableSet setWithArray:[fileManager contentsOfDirectoryAtPath:storeUrl.path error:nil]];
    std::string raPath = storeUrl.path.UTF8String;
    CVideoThumbLoader thumbLoader;

    auto fillSharedDicts =
        [&](CFileItemList& videoItems, NSString* videosKey, NSString* videosTitleKey,
            uint32_t titleStringCode,
            std::function<std::string(CFileItemPtr videoItem)> getThumbnailForItem,
            std::function<std::string(CFileItemPtr videoItem)> getTitleForItem) {
          if (videoItems.Size() <= 0)
          {
            // cleanup if there is no RA
            [sharedDefaults removeObjectForKey:videosKey];
            [sharedDefaults removeObjectForKey:videosTitleKey];
            return;
          }

          const int topShelfSize = std::min(videoItems.Size(), MaxItems);
          auto videosArray = [NSMutableArray arrayWithCapacity:topShelfSize];
          for (int i = 0; i < topShelfSize; ++i)
          {
            @autoreleasepool
            {
              auto videoItem = videoItems.Get(i);
              if (!videoItem->HasArt("thumb"))
                thumbLoader.LoadItem(videoItem.get());

              auto thumbnailPath = getThumbnailForItem(videoItem);
              auto fileName = std::to_string(videoItem->GetVideoInfoTag()->m_iDbId) +
                              URIUtils::GetFileName(thumbnailPath);
              auto destPath = URIUtils::AddFileToFolder(raPath, fileName);
              if (!XFILE::CFile::Exists(destPath))
                XFILE::CFile::Copy(thumbnailPath, destPath);
              else
              {
                // remove from array so it doesnt get deleted at the end
                [filePaths removeObject:@(fileName.c_str())];
              }

              auto title = getTitleForItem(videoItem);
              CLog::Log(LOGDEBUG, "TopShelf: - adding video to '{}' array: {}\n",
                        videosKey.UTF8String, title.c_str());
              [videosArray addObject:@{
                @"title" : @(title.c_str()),
                @"thumb" : @(fileName.c_str()),
                @"url" : @(Base64::Encode(videoItem->GetVideoInfoTag()->GetPath()).c_str())
              }];
            }
          }
          [sharedDefaults setObject:videosArray forKey:videosKey];
          sharedDictJailbreak[videosKey] = videosArray;

          auto tvTitle = @(g_localizeStrings.Get(titleStringCode).c_str());
          [sharedDefaults setObject:tvTitle forKey:videosTitleKey];
          sharedDictJailbreak[videosTitleKey] = tvTitle;
        };

    fillSharedDicts(movies, @"movies", @"moviesTitle", 20386,
                    [](CFileItemPtr videoItem) { return videoItem->GetArt("thumb"); },
                    [](CFileItemPtr videoItem) { return videoItem->GetLabel(); });

    CVideoDatabase videoDb;
    videoDb.Open();
    fillSharedDicts(tv, @"tv", @"tvTitle", 20387,
                    [&videoDb](CFileItemPtr videoItem) {
                      int season = videoItem->GetVideoInfoTag()->m_iIdSeason;
                      return season > 0 ? videoDb.GetArtForItem(season, MediaTypeSeason, "poster")
                                        : std::string{};
                    },
                    [](CFileItemPtr videoItem) {
                      return StringUtils::Format(
                          "%s s%02de%02d", videoItem->GetVideoInfoTag()->m_strShowTitle.c_str(),
                          videoItem->GetVideoInfoTag()->m_iSeason,
                          videoItem->GetVideoInfoTag()->m_iEpisode);
                    });
    videoDb.Close();

    // remove unused thumbs from cache folder
    NSString* strFiles;
    for (strFiles in filePaths)
      [fileManager removeItemAtURL:[storeUrl URLByAppendingPathComponent:strFiles isDirectory:NO]
                             error:nil];

    [sharedDictJailbreak writeToURL:[storeUrl URLByAppendingPathComponent:@"shared.dict"]
                         atomically:YES];
    [sharedDefaults synchronize];
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
