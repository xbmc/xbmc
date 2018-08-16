/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import <UIKit/UIKit.h>

#import "Application.h"
#import "FileItem.h"
#import "music/tags/MusicInfoTag.h"
#import "music/MusicDatabase.h"
#import "TextureCache.h"
#import "filesystem/SpecialProtocol.h"
#include "PlayListPlayer.h"
#import "playlists/PlayList.h"

#import "platform/darwin/ios-common/AnnounceReceiver.h"
#if defined(TARGET_DARWIN_TVOS)
#import "platform/darwin/tvos/MainController.h"
#else
#import "platform/darwin/ios/XBMCController.h"
#endif
#import "utils/Variant.h"
#include "ServiceBroker.h"

id objectFromVariant(const CVariant &data);

NSArray *arrayFromVariantArray(const CVariant &data)
{
  if (!data.isArray())
    return nil;
  NSMutableArray *array = [[[NSMutableArray alloc] initWithCapacity:data.size()] autorelease];
  for (CVariant::const_iterator_array itr = data.begin_array(); itr != data.end_array(); ++itr)
    [array addObject:objectFromVariant(*itr)];

  return array;
}

NSDictionary *dictionaryFromVariantMap(const CVariant &data)
{
  if (!data.isObject())
    return nil;
  NSMutableDictionary *dict = [[[NSMutableDictionary alloc] initWithCapacity:data.size()] autorelease];
  for (CVariant::const_iterator_map itr = data.begin_map(); itr != data.end_map(); ++itr)
    [dict setValue:objectFromVariant(itr->second) forKey:[NSString stringWithUTF8String:itr->first.c_str()]];

  return dict;
}

id objectFromVariant(const CVariant &data)
{
  if (data.isNull())
    return nil;
  if (data.isString())
    return [NSString stringWithUTF8String:data.asString().c_str()];
  if (data.isWideString())
    return [NSString stringWithCString:(const char *)data.asWideString().c_str() encoding:NSUnicodeStringEncoding];
  if (data.isInteger())
    return [NSNumber numberWithLongLong:data.asInteger()];
  if (data.isUnsignedInteger())
    return [NSNumber numberWithUnsignedLongLong:data.asUnsignedInteger()];
  if (data.isBoolean())
    return [NSNumber numberWithInt:data.asBoolean()?1:0];
  if (data.isDouble())
    return [NSNumber numberWithDouble:data.asDouble()];
  if (data.isArray())
    return arrayFromVariantArray(data);
  if (data.isObject())
    return dictionaryFromVariantMap(data);

  return nil;
}

void AnnounceBridge(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
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
        item_id = (int)nonConstData["item"]["id"].asInteger();
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

  //LOG(@"AnnounceBridge: [%s], [%s], [%s]", ANNOUNCEMENT::AnnouncementFlagToString(flag), sender, message);
  NSDictionary *dict = dictionaryFromVariantMap(nonConstData);
  //LOG(@"data: %@", dict.description);
  if (msg == "OnPlay" || msg == "OnResume")
  {
    NSDictionary *item = [dict valueForKey:@"item"];
    NSDictionary *player = [dict valueForKey:@"player"];
    [item setValue:[player valueForKey:@"speed"] forKey:@"speed"];
    std::string thumb = g_application.CurrentFileItem().GetArt("thumb");
    if (!thumb.empty())
    {
      bool needsRecaching;
      std::string cachedThumb(CTextureCache::GetInstance().CheckCachedImage(thumb, needsRecaching));
      //LOG("thumb: %s, %s", thumb.c_str(), cachedThumb.c_str());
      if (!cachedThumb.empty())
      {
        std::string thumbRealPath = CSpecialProtocol::TranslatePath(cachedThumb);
        [item setValue:[NSString stringWithUTF8String:thumbRealPath.c_str()] forKey:@"thumb"];
      }
    }
    double duration = g_application.GetTotalTime();
    if (duration > 0)
      [item setValue:[NSNumber numberWithDouble:duration] forKey:@"duration"];
    [item setValue:[NSNumber numberWithDouble:g_application.GetTime()] forKey:@"elapsed"];
    int current = CServiceBroker::GetPlaylistPlayer().GetCurrentSong();
    if (current >= 0)
    {
      [item setValue:[NSNumber numberWithInt:current] forKey:@"current"];
      [item setValue:[NSNumber numberWithInt:CServiceBroker::GetPlaylistPlayer().GetPlaylist(CServiceBroker::GetPlaylistPlayer().GetCurrentPlaylist()).size()] forKey:@"total"];
    }
    if (g_application.CurrentFileItem().HasMusicInfoTag())
    {
      const std::vector<std::string> &genre = g_application.CurrentFileItem().GetMusicInfoTag()->GetGenre();
      if (!genre.empty())
      {
        NSMutableArray *genreArray = [[NSMutableArray alloc] initWithCapacity:genre.size()];
        for(std::vector<std::string>::const_iterator it = genre.begin(); it != genre.end(); ++it)
        {
          [genreArray addObject:[NSString stringWithUTF8String:it->c_str()]];
        }
        [item setValue:genreArray forKey:@"genre"];
      }
    }
    //LOG(@"item: %@", item.description);
    [g_xbmcController performSelectorOnMainThread:@selector(onPlay:) withObject:item  waitUntilDone:NO];
  }
  else if (msg == "OnSpeedChanged" || msg == "OnPause")
  {
    NSDictionary *item = [dict valueForKey:@"item"];
    NSDictionary *player = [dict valueForKey:@"player"];
    [item setValue:[player valueForKey:@"speed"] forKey:@"speed"];
    [item setValue:[NSNumber numberWithDouble:g_application.GetTime()] forKey:@"elapsed"];
    //LOG(@"item: %@", item.description);
    [g_xbmcController performSelectorOnMainThread:@selector(OnSpeedChanged:) withObject:item  waitUntilDone:NO];
    if (msg == "OnPause")
      [g_xbmcController performSelectorOnMainThread:@selector(onPause:) withObject:[dict valueForKey:@"item"]  waitUntilDone:NO];
  }
  else if (msg == "OnStop")
  {
    [g_xbmcController performSelectorOnMainThread:@selector(onStop:) withObject:[dict valueForKey:@"item"]  waitUntilDone:NO];
  }
}

CAnnounceReceiver *CAnnounceReceiver::GetInstance()
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

void CAnnounceReceiver::Announce(ANNOUNCEMENT::AnnouncementFlag flag, const char *sender, const char *message, const CVariant &data)
{
  // can be called from c++, we need an auto poll here.
  NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];
  AnnounceBridge(flag, sender, message, data);
  [pool release];
}
