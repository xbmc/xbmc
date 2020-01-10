/*
 *  Copyright (C) 2010-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "DarwinEmbedNowPlayingInfoManager.h"

#if defined(TARGET_DARWIN_IOS)
#include "platform/darwin/ios/XBMCController.h"
#elif defined(TARGET_DARWIN_TVOS)
#include "platform/darwin/tvos/XBMCController.h"
#endif

#import <MediaPlayer/MediaPlayer.h>

@implementation DarwinEmbedNowPlayingInfoManager

@synthesize nowPlayingInfo = m_nowPlayingInfo;
@synthesize playbackState;

#pragma mark - Now Playing routines

- (void)onPlay:(NSDictionary*)item
{
  auto dict = [NSMutableDictionary new];

  NSString* title = item[@"title"];
  if (title && title.length > 0)
    dict[MPMediaItemPropertyTitle] = title;
  NSString* album = item[@"album"];
  if (album && album.length > 0)
    dict[MPMediaItemPropertyAlbumTitle] = album;
  NSArray* artists = item[@"artist"];
  if (artists && artists.count > 0)
    dict[MPMediaItemPropertyArtist] = [artists componentsJoinedByString:@" "];
  if (id track = item[@"track"])
    dict[MPMediaItemPropertyAlbumTrackNumber] = track;
  if (id duration = item[@"duration"])
    dict[MPMediaItemPropertyPlaybackDuration] = duration;
  NSArray* genres = item[@"genre"];
  if (genres && genres.count > 0)
    dict[MPMediaItemPropertyGenre] = [genres componentsJoinedByString:@" "];

  NSString* thumb = item[@"thumb"];
  if (thumb && thumb.length > 0)
  {
    if (auto image = [UIImage imageWithContentsOfFile:thumb])
    {
      if (auto mArt =
          [[MPMediaItemArtwork alloc] initWithBoundsSize:image.size
                                          requestHandler:^UIImage* _Nonnull(CGSize aSize) {
                                            return image;}])
      {
        dict[MPMediaItemPropertyArtwork] = mArt;
      }
    }
  }

  if (id elapsed = item[@"elapsed"])
    dict[MPNowPlayingInfoPropertyElapsedPlaybackTime] = elapsed;
  if (id speed = item[@"speed"])
    dict[MPNowPlayingInfoPropertyPlaybackRate] = speed;
  if (id current = item[@"current"])
    dict[MPNowPlayingInfoPropertyPlaybackQueueIndex] = current;
  if (id total = item[@"total"])
    dict[MPNowPlayingInfoPropertyPlaybackQueueCount] = total;

  /*! @Todo additional properties?
   other properities can be set:
   MPMediaItemPropertyAlbumTrackCount
   MPMediaItemPropertyComposer
   MPMediaItemPropertyDiscCount
   MPMediaItemPropertyDiscNumber
   MPMediaItemPropertyPersistentID
   Additional metadata properties:
   MPNowPlayingInfoPropertyChapterNumber;
   MPNowPlayingInfoPropertyChapterCount;
   */

  [self setNowPlayingInfo:dict];

  self.playbackState = DARWINEMBED_PLAYBACK_PLAYING;

#if defined(TARGET_DARWIN_IOS)
  [g_xbmcController disableNetworkAutoSuspend];
#endif
}

- (void)OnSpeedChanged:(NSDictionary*)item
{
  NSMutableDictionary* dict = [self.nowPlayingInfo mutableCopy];
  if (id elapsed = item[@"elapsed"])
    dict[MPNowPlayingInfoPropertyElapsedPlaybackTime] = elapsed;
  if (id speed = item[@"speed"])
    dict[MPNowPlayingInfoPropertyPlaybackRate] = speed;

  [self setNowPlayingInfo:dict];
}

- (void)onPause:(NSDictionary*)item
{
  self.playbackState = DARWINEMBED_PLAYBACK_PAUSED;

#if defined(TARGET_DARWIN_IOS)
  // schedule set network auto suspend state for save power if idle.
  [g_xbmcController rescheduleNetworkAutoSuspend];
#endif
}

- (void)onStop:(NSDictionary*)item
{
  [self setNowPlayingInfo:nil];

  self.playbackState = DARWINEMBED_PLAYBACK_STOPPED;

#if defined(TARGET_DARWIN_IOS)
  // delay set network auto suspend state in case we are switching playing item.
  [g_xbmcController rescheduleNetworkAutoSuspend];
#endif
}

- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context
{
  if ([keyPath isEqualToString:NSStringFromSelector(@selector(nowPlayingInfo))])
    [MPNowPlayingInfoCenter defaultCenter].nowPlayingInfo = m_nowPlayingInfo;
}

- (instancetype)init
{
  self = [super init];
  if (!self)
    return nil;

  playbackState = DARWINEMBED_PLAYBACK_STOPPED;
  [self addObserver:self
         forKeyPath:NSStringFromSelector(@selector(nowPlayingInfo))
            options:kNilOptions
            context:nullptr];

  return self;
}

- (void)dealloc
{
  [self removeObserver:self forKeyPath:NSStringFromSelector(@selector(nowPlayingInfo))];
}

@end
