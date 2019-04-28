/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */


#import "TVOSTopShelf.h"
#import "tvosShared.h"

#include "Application.h"
#include "messaging/ApplicationMessenger.h"
#include "FileItem.h"
#include "DatabaseManager.h"
#include "guilib/GUIWindowManager.h"
#include "video/VideoDatabase.h"
#include "video/VideoThumbLoader.h"
#include "video/VideoInfoTag.h"
#include "video/dialogs/GUIDialogVideoInfo.h"
#include "video/windows/GUIWindowVideoNav.h"
#include "video/windows/GUIWindowVideoBase.h"
#include "filesystem/File.h"

#import "system.h"
#include "utils/log.h"
#include "guilib/LocalizeStrings.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/Base64.h"

#import <Foundation/Foundation.h>
#import <UIKit/UIKit.h>
#import <mach/mach_host.h>
#import <sys/sysctl.h>

std::string CTVOSTopShelf::m_url;
bool CTVOSTopShelf::m_handleUrl;

CTVOSTopShelf::CTVOSTopShelf()
{
}

CTVOSTopShelf::~CTVOSTopShelf()
{
}

CTVOSTopShelf &CTVOSTopShelf::GetInstance()
{
  static CTVOSTopShelf sTopShelf;
  return sTopShelf;
}

void CTVOSTopShelf::SetTopShelfItems(CFileItemList& movies, CFileItemList& tv)
{
  CVideoThumbLoader loader;
  NSString* sharedID = [tvosShared getSharedID];
  NSMutableArray* movieArray = [[NSMutableArray alloc] init];
  NSMutableArray* tvArray = [[NSMutableArray alloc] init];
  NSUserDefaults* shared = [[NSUserDefaults alloc] initWithSuiteName:sharedID];
  NSMutableDictionary* sharedDict = [[NSMutableDictionary alloc] init];

  NSFileManager* fileManager = [NSFileManager defaultManager];
  NSURL* storeUrl = [tvosShared getSharedURL];
  NSURL* sharedDictUrl = [storeUrl URLByAppendingPathComponent:@"shared.dict"];
  storeUrl = [storeUrl URLByAppendingPathComponent:@"RA" isDirectory:TRUE];

  CLog::Log(LOGDEBUG, "TopShelf: using shared path %s (jailbroken: %s)\n", [[storeUrl path] cStringUsingEncoding:NSUTF8StringEncoding], [tvosShared isJailbroken] ? "yes" : "no");

  // store all old thumbs in array
  NSMutableArray* filePaths = (NSMutableArray*)[fileManager contentsOfDirectoryAtPath:storeUrl.path error:nil];
  if (storeUrl == nil)
    return;
  std::string raPath = [storeUrl.path UTF8String];

  if (movies.Size() > 0)
  {
    for (int i = 0; i < movies.Size() && i < 5; ++i)
    {
      CFileItemPtr item          = movies.Get(i);
      NSMutableDictionary* movieDict = [[NSMutableDictionary alloc] init];
      if (!item->HasArt("thumb"))
        loader.LoadItem(item.get());

      // srcPath == full path to the thumb
      std::string srcPath = item->GetArt("thumb");
      // make the destfilename different for distinguish files with the same name
      std::string fileName = std::to_string(item->GetVideoInfoTag()->m_iDbId) + URIUtils::GetFileName(srcPath);
      std::string destPath = URIUtils::AddFileToFolder(raPath, fileName);
      if (!XFILE::CFile::Exists(destPath))
        XFILE::CFile::Copy(srcPath,destPath);
      else
        // remove from array so it doesnt get deleted at the end
        if ([filePaths containsObject:[NSString stringWithUTF8String:fileName.c_str()]])
          [filePaths removeObject:[NSString stringWithUTF8String:fileName.c_str()]];

      CLog::Log(LOGDEBUG, "TopShelf: - adding movie to array %s\n",item->GetLabel().c_str());
      [movieDict setValue:[NSString stringWithUTF8String:item->GetLabel().c_str()] forKey:@"title"];
      [movieDict setValue:[NSString stringWithUTF8String:fileName.c_str()] forKey:@"thumb"];
      std::string fullPath = item->GetVideoInfoTag()->GetPath();
      [movieDict setValue:[NSString stringWithUTF8String:Base64::Encode(fullPath).c_str()] forKey:@"url"];

      [movieArray addObject:movieDict];
    }
    [shared setObject:movieArray forKey:@"movies"];
    [sharedDict setObject:movieArray forKey:@"movies"];
    NSString *tvTitle = [NSString stringWithUTF8String:g_localizeStrings.Get(20386).c_str()];
    [shared setObject:tvTitle forKey:@"moviesTitle"];
    [sharedDict setObject:tvTitle forKey:@"moviesTitle"];// for jailbroken devices
  }
  else
  {
    // cleanup if there is no RA
    [shared removeObjectForKey:@"movies"];
    [shared removeObjectForKey:@"moviesTitle"];
  }

  if (tv.Size() > 0)
  {
    for (int i = 0; i < tv.Size() && i < 5; ++i)
    {
      CFileItemPtr item = tv.Get(i);
      NSMutableDictionary* tvDict = [[NSMutableDictionary alloc] init];
      if (!item->HasArt("thumb"))
        loader.LoadItem(item.get());

      std::string title = StringUtils::Format("%s s%02de%02d",
                                              item->GetVideoInfoTag()->m_strShowTitle.c_str(),
                                              item->GetVideoInfoTag()->m_iSeason,
                                              item->GetVideoInfoTag()->m_iEpisode);

      std::string seasonThumb;

      if (item->GetVideoInfoTag()->m_iIdSeason > 0)
      {
        CVideoDatabase videodatabase;
        videodatabase.Open();
        seasonThumb = videodatabase.GetArtForItem(item->GetVideoInfoTag()->m_iIdSeason, MediaTypeSeason, "poster");

        videodatabase.Close();
      }

      std::string fileName = std::to_string(item->GetVideoInfoTag()->m_iDbId) + URIUtils::GetFileName(seasonThumb);
      std::string destPath = URIUtils::AddFileToFolder(raPath, fileName);
      if (!XFILE::CFile::Exists(destPath))
        XFILE::CFile::Copy(seasonThumb ,destPath);
      else
        // remove from array so it doesnt get deleted at the end
        if ([filePaths containsObject:[NSString stringWithUTF8String:fileName.c_str()]])
          [filePaths removeObject:[NSString stringWithUTF8String:fileName.c_str()]];

      CLog::Log(LOGDEBUG, "TopShelf: - adding tvshow to array %s\n",title.c_str());
      [tvDict setValue:[NSString stringWithUTF8String:title.c_str()] forKey:@"title"];
      [tvDict setValue:[NSString stringWithUTF8String:fileName.c_str()] forKey:@"thumb"];

      std::string fullPath = item->GetVideoInfoTag()->GetPath();
      [tvDict setValue:[NSString stringWithUTF8String:Base64::Encode(fullPath).c_str()] forKey:@"url"];
      [tvArray addObject:tvDict];
    }
    [shared setObject:tvArray forKey:@"tv"];
    [sharedDict setObject:tvArray forKey:@"tv"];// for jailbroken devices
    NSString* tvTitle = [NSString stringWithUTF8String:g_localizeStrings.Get(20387).c_str()];
    [shared setObject:tvTitle forKey:@"tvTitle"];
    [sharedDict setObject:tvTitle forKey:@"tvTitle"];// for jailbroken devices
  }
  else
  {
    // cleanup if there is no RA
    [shared removeObjectForKey:@"tv"];
    [shared removeObjectForKey:@"tvTitle"];
  }

  // remove unused thumbs from cache folder
  for (NSString* strFiles in filePaths)
    [fileManager removeItemAtURL:[storeUrl URLByAppendingPathComponent:strFiles isDirectory:FALSE] error:nil];

  if ([tvosShared isJailbroken])
  {
    [sharedDict writeToFile:[sharedDictUrl path] atomically:TRUE];
  }

  [shared synchronize];
}

void CTVOSTopShelf::RunTopShelf()
{
  if (m_handleUrl)
  {
    m_handleUrl = false;

    std::vector<std::string> split = StringUtils::Split(m_url, "/");

    std::string url = Base64::Decode(split[4]);

    if (split[2] == "display")
    {
      KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_MASK_WINDOWMANAGER + 9, -1, -1, static_cast<void*>(new CFileItem(url.c_str(), false)));
    }
    else //play
    {
      // its a bit ugly, but only way to get resume window to show
      std::string cmd = StringUtils::Format("PlayMedia(%s)", StringUtils::Paramify(url.c_str()).c_str());
      KODI::MESSAGING::CApplicationMessenger::GetInstance().PostMsg(TMSG_EXECUTE_BUILT_IN, -1, -1, nullptr, cmd);
    }
  }
}

void CTVOSTopShelf::HandleTopShelfUrl(const std::string& url, const bool run)
{
  m_url = url;
  m_handleUrl = run;
}
