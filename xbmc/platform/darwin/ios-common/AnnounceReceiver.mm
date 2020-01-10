/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "platform/darwin/ios-common/AnnounceReceiver.h"

#include "Application.h"
#include "FileItem.h"
#include "PlayListPlayer.h"
#include "ServiceBroker.h"
#include "TextureCache.h"
#include "filesystem/SpecialProtocol.h"
#include "music/MusicDatabase.h"
#include "music/tags/MusicInfoTag.h"
#include "playlists/PlayList.h"
#include "utils/Variant.h"

#import "platform/darwin/ios-common/DarwinEmbedNowPlayingInfoManager.h"
#if defined(TARGET_DARWIN_IOS)
#import "platform/darwin/ios/XBMCController.h"
#elif defined(TARGET_DARWIN_TVOS)
#import "platform/darwin/tvos/XBMCController.h"
#endif

#import <UIKit/UIKit.h>

id objectFromVariant(const CVariant& data);

NSArray* arrayFromVariantArray(const CVariant& data)
{
  if (!data.isArray())
    return nil;
  NSMutableArray* array = [[NSMutableArray alloc] initWithCapacity:data.size()];
  for (CVariant::const_iterator_array itr = data.begin_array(); itr != data.end_array(); ++itr)
    [array addObject:objectFromVariant(*itr)];

  return array;
}

NSDictionary* dictionaryFromVariantMap(const CVariant& data)
{
  if (!data.isObject())
    return nil;
  NSMutableDictionary* dict = [[NSMutableDictionary alloc] initWithCapacity:data.size()];
  for (CVariant::const_iterator_map itr = data.begin_map(); itr != data.end_map(); ++itr)
    dict[@(itr->first.c_str())] = objectFromVariant(itr->second);

  return dict;
}

id objectFromVariant(const CVariant& data)
{
  if (data.isNull())
    return nil;
  if (data.isString())
    return @(data.asString().c_str());
  if (data.isWideString())
    return [NSString stringWithCString:reinterpret_cast<const char*>(data.asWideString().c_str())
                              encoding:NSUnicodeStringEncoding];
  if (data.isInteger())
    return @(data.asInteger());
  if (data.isUnsignedInteger())
    return @(data.asUnsignedInteger());
  if (data.isBoolean())
    return @(data.asBoolean() ? 1 : 0);
  if (data.isDouble())
    return @(data.asDouble());
  if (data.isArray())
    return arrayFromVariantArray(data);
  if (data.isObject())
    return dictionaryFromVariantMap(data);

  return nil;
}

void AnnounceBridge(ANNOUNCEMENT::AnnouncementFlag flag,
                    const char* sender,
                    const char* message,
                    const CVariant& data)
{
  int item_id = -1;
  std::string item_type = "";
  CVariant nonConstData = data;
  const std::string msg(message);

  // handle data which only has a database id and not the metadata inside
  if (msg == "OnPlay" || msg == "OnResume")
  {
    if (!nonConstData["item"].isNull())
    {
      if (!nonConstData["item"]["id"].isNull())
      {
        item_id = static_cast<int>(nonConstData["item"]["id"].asInteger());
      }

      if (!nonConstData["item"]["type"].isNull())
      {
        item_type = nonConstData["item"]["type"].asString();
      }
    }

    // if we got an id from the passed data
    // we need to get title, track, album and artist from the db
    if (item_id >= 0)
    {
      if (item_type == MediaTypeSong)
      {
        CMusicDatabase db;
        if (db.Open())
        {
          CSong song;
          if (db.GetSong(item_id, song))
          {
            nonConstData["item"]["title"] = song.strTitle;
            nonConstData["item"]["track"] = song.iTrack;
            nonConstData["item"]["album"] = song.strAlbum;
            nonConstData["item"]["artist"] = song.GetArtist();
          }
        }
      }
    }
  }

  auto dict = dictionaryFromVariantMap(nonConstData);

  if (msg == "OnPlay" || msg == "OnResume")
  {
    NSMutableDictionary* item = dict[@"item"];
    NSDictionary* player = dict[@"player"];
    item[@"speed"] = player[@"speed"];
    std::string thumb = g_application.CurrentFileItem().GetArt("thumb");
    if (!thumb.empty())
    {
      bool needsRecaching;
      std::string cachedThumb(CTextureCache::GetInstance().CheckCachedImage(thumb, needsRecaching));
      if (!cachedThumb.empty())
      {
        std::string thumbRealPath = CSpecialProtocol::TranslatePath(cachedThumb);
        item[@"thumb"] = @(thumbRealPath.c_str());
      }
    }
    double duration = g_application.GetTotalTime();
    if (duration > 0)
      item[@"duration"] = @(duration);
    item[@"elapsed"] = @(g_application.GetTime());
    int current = CServiceBroker::GetPlaylistPlayer().GetCurrentSong();
    if (current >= 0)
    {
      item[@"current"] = @(current);
      item[@"total"] = @(CServiceBroker::GetPlaylistPlayer()
                             .GetPlaylist(CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist())
                             .size());
    }
    if (g_application.CurrentFileItem().HasMusicInfoTag())
    {
      const auto& genre = g_application.CurrentFileItem().GetMusicInfoTag()->GetGenre();
      if (!genre.empty())
      {
        NSMutableArray* genreArray = [[NSMutableArray alloc] initWithCapacity:genre.size()];
        for (auto genreItem : genre)
        {
          [genreArray addObject:@(genreItem.c_str())];
        }
        item[@"genre"] = genreArray;
      }
    }

    dispatch_async(dispatch_get_main_queue(), ^{
      [g_xbmcController.MPNPInfoManager onPlay:item];
    });
  }
  else if (msg == "OnSpeedChanged" || msg == "OnPause")
  {
    NSMutableDictionary* item = dict[@"item"];
    NSDictionary* player = dict[@"player"];
    item[@"speed"] = player[@"speed"];
    item[@"elapsed"] = @(g_application.GetTime());

    dispatch_async(dispatch_get_main_queue(), ^{
      [g_xbmcController.MPNPInfoManager OnSpeedChanged:item];
    });
    if (msg == "OnPause")
    {
      dispatch_async(dispatch_get_main_queue(), ^{
        [g_xbmcController.MPNPInfoManager onPause:item];
      });
    }
  }
  else if (msg == "OnStop")
  {
    dispatch_async(dispatch_get_main_queue(), ^{
      [g_xbmcController.MPNPInfoManager onStop:dict[@"item"]];
    });
  }
}

CAnnounceReceiver* CAnnounceReceiver::GetInstance()
{
  static CAnnounceReceiver announceReceiverInstance;
  return &announceReceiverInstance;
}

void CAnnounceReceiver::Initialize()
{
  CServiceBroker::GetAnnouncementManager()->AddAnnouncer(GetInstance());
}

void CAnnounceReceiver::DeInitialize()
{
  CServiceBroker::GetAnnouncementManager()->RemoveAnnouncer(GetInstance());
}

void CAnnounceReceiver::Announce(ANNOUNCEMENT::AnnouncementFlag flag,
                                 const char* sender,
                                 const char* message,
                                 const CVariant& data)
{
  // can be called from c++, we need an auto poll here.
  @autoreleasepool
  {
    AnnounceBridge(flag, sender, message, data);
  }
}
