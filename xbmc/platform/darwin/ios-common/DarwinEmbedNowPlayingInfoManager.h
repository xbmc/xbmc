/*
 *  Copyright (C) 2010-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import <Foundation/Foundation.h>

typedef NS_ENUM(unsigned int, DarwinEmbedPlaybackState) {
  DARWINEMBED_PLAYBACK_STOPPED,
  DARWINEMBED_PLAYBACK_PAUSED,
  DARWINEMBED_PLAYBACK_PLAYING
};

@interface DarwinEmbedNowPlayingInfoManager : NSObject

@property(nonatomic, copy) NSDictionary* nowPlayingInfo;
@property(nonatomic) DarwinEmbedPlaybackState playbackState;

- (void)onPlay:(NSDictionary*)item;
- (void)OnSpeedChanged:(NSDictionary*)item;
- (void)onPause:(NSDictionary*)item;
- (void)onStop:(NSDictionary*)item;
- (void)observeValueForKeyPath:(NSString*)keyPath
                      ofObject:(id)object
                        change:(NSDictionary*)change
                       context:(void*)context;

@end
