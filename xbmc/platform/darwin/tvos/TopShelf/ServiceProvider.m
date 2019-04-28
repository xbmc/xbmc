/*
 *      Copyright (C) 2015 Team MrMC
 *      https://github.com/MrMC
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with MrMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#import "ServiceProvider.h"
#import "../tvosShared.h"

@implementation ServiceProvider

#pragma mark - TVTopShelfProvider protocol

- (TVTopShelfContentStyle)topShelfStyle
{
  return TVTopShelfContentStyleSectioned;
}

- (NSArray<TVContentItem*>*)topShelfItems
{
  NSURL* storeUrl = [tvosShared getSharedURL];
  if (!storeUrl)
    return @[];

  NSString* sharedID = [tvosShared getSharedID];
  NSUserDefaults* shared = [[NSUserDefaults alloc] initWithSuiteName:sharedID];
  NSMutableArray* topShelfItems = [[NSMutableArray alloc] init];
  TVContentIdentifier* wrapperIdentifier = [[TVContentIdentifier alloc] initWithIdentifier:@"shelf-wrapper" container:nil];

  NSArray* movieArray = nil;
  NSArray* tvArray = nil;
  NSDictionary* sharedDict = nil;
  
  if ([tvosShared isJailbroken])
  {
    NSURL* sharedDictUrl = [storeUrl URLByAppendingPathComponent:@"shared.dict" isDirectory:FALSE];
    sharedDict = [NSDictionary dictionaryWithContentsOfFile:[sharedDictUrl path]];

    movieArray = [sharedDict valueForKey:@"movies"];
    tvArray = [sharedDict valueForKey:@"tv"];
  }
  else
  {
    movieArray = [shared objectForKey:@"movies"];
    tvArray = [shared valueForKey:@"tv"];
  }

  storeUrl = [storeUrl URLByAppendingPathComponent:@"RA" isDirectory:TRUE];
  __auto_type contentItemsFrom = ^NSArray<TVContentItem*>* (NSArray* videosArray){
    NSMutableArray<TVContentItem*>* contentItems = [[NSMutableArray alloc] initWithCapacity:videosArray.count];
    for (NSDictionary* videoDict in videosArray)
    {
      TVContentIdentifier* identifier = [[TVContentIdentifier alloc] initWithIdentifier:@"VOD" container:wrapperIdentifier];
      TVContentItem* contentItem = [[TVContentItem alloc] initWithContentIdentifier:identifier];
      [identifier release];

      [contentItem setImageURL:[storeUrl URLByAppendingPathComponent:[videoDict valueForKey:@"thumb"] isDirectory:FALSE] forTraits:TVContentItemImageTraitScreenScale1x];
      contentItem.imageShape = TVContentItemImageShapePoster;
      contentItem.title = [videoDict valueForKey:@"title"];
      NSString* url = [videoDict valueForKey:@"url"];
      contentItem.displayURL = [NSURL URLWithString:[NSString stringWithFormat:@"kodi://display/movie/%@", url]];
      contentItem.playURL = [NSURL URLWithString:[NSString stringWithFormat:@"kodi://play/movie/%@", url]];
      [contentItems addObject:contentItem];
      [contentItem release];
    }
    return [contentItems autorelease];
  };
  
  if ([movieArray count] > 0)
  {
    TVContentItem* itemMovie = [[TVContentItem alloc] initWithContentIdentifier:wrapperIdentifier];
    itemMovie.title = [(sharedDict ?: shared) valueForKey:@"moviesTitle"];
    itemMovie.topShelfItems = contentItemsFrom(movieArray);
    [topShelfItems addObject:itemMovie];
    [itemMovie release];
  }

  if ([tvArray count] > 0)
  {
    TVContentItem* itemTv = [[TVContentItem alloc] initWithContentIdentifier:wrapperIdentifier];
    itemTv.title = [(sharedDict ?: shared) valueForKey:@"tvTitle"];
    itemTv.topShelfItems = contentItemsFrom(tvArray);
    [topShelfItems addObject:itemTv];
    [itemTv release];
  }

  [wrapperIdentifier release];
  [shared release];
  
  return [topShelfItems autorelease];
}

@end
