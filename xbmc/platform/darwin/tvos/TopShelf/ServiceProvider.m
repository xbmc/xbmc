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

@interface ServiceProvider ()

@end

@implementation ServiceProvider


- (instancetype)init {
    self = [super init];
    if (self) {
    }
    return self;
}

#pragma mark - TVTopShelfProvider protocol

- (TVTopShelfContentStyle)topShelfStyle
{
    // Return desired Top Shelf style.
    return TVTopShelfContentStyleSectioned;
}

- (NSArray *)topShelfItems
{
    NSString *sharedID = [tvosShared getSharedID];
    NSMutableArray *topShelfItems = [[NSMutableArray alloc] init];
    NSUserDefaults *shared = [[NSUserDefaults alloc] initWithSuiteName:sharedID];
  
    TVContentIdentifier *wrapperIdentifier = [[TVContentIdentifier alloc] initWithIdentifier:@"shelf-wrapper" container:nil];

    NSURL* storeUrl = [tvosShared getSharedURL];
    NSArray *movieArray = nil;
    NSArray * tvArray = nil;
    NSMutableDictionary *sharedDict = nil;
  
    if ([tvosShared isJailbroken])
    {
      NSURL* sharedDictUrl = [storeUrl URLByAppendingPathComponent:@"shared.dict" isDirectory:FALSE];
      sharedDict = [NSMutableDictionary dictionaryWithContentsOfFile:[sharedDictUrl path]];
      movieArray = [sharedDict valueForKey:@"movies"];
      tvArray = [sharedDict valueForKey:@"tv"];
    }
    else
    {
      movieArray = [shared objectForKey:@"movies"];
      tvArray = [shared valueForKey:@"tv"];
    }

    storeUrl = [storeUrl URLByAppendingPathComponent:@"RA" isDirectory:TRUE];
  
    if ([movieArray count] > 0)
    {
      TVContentItem *itemMovie = [[TVContentItem alloc] initWithContentIdentifier:wrapperIdentifier];
      NSMutableArray *ContentItems = [[NSMutableArray alloc] init];
      for (NSUInteger i = 0; i < [movieArray count]; i++)
      {
        NSMutableDictionary * movieDict = [[NSMutableDictionary alloc] init];
        movieDict = [movieArray objectAtIndex:i];
        
        TVContentIdentifier *identifier = [[TVContentIdentifier alloc] initWithIdentifier:@"VOD" container:wrapperIdentifier];
        TVContentItem *contentItem = [[TVContentItem alloc] initWithContentIdentifier:identifier];
        
        contentItem.imageURL = [storeUrl URLByAppendingPathComponent:[movieDict valueForKey:@"thumb"] isDirectory:FALSE];
        contentItem.imageShape = TVContentItemImageShapePoster;
        contentItem.title = [movieDict valueForKey:@"title"];
        NSString *url = [movieDict valueForKey:@"url"];
        contentItem.displayURL = [NSURL URLWithString:[NSString stringWithFormat:@"kodi://display/movie/%@",url]];
        contentItem.playURL = [NSURL URLWithString:[NSString stringWithFormat:@"kodi://play/movie/%@",url]];
        [ContentItems addObject:contentItem];
      }
      
      if ([tvosShared isJailbroken])
      {
        itemMovie.title = [sharedDict valueForKey:@"moviesTitle"];
      }
      else
      {
        itemMovie.title = [shared stringForKey:@"moviesTitle"];
      }

      itemMovie.topShelfItems = ContentItems;
      [topShelfItems addObject:itemMovie];
    }

    if ([tvArray count] > 0)
    {
      TVContentItem *itemTv = [[TVContentItem alloc] initWithContentIdentifier:wrapperIdentifier];
      NSMutableArray *ContentItemsTv = [[NSMutableArray alloc] init];
      for (NSUInteger i = 0; i < [tvArray count]; i++)
      {
        NSMutableDictionary * tvDict = [[NSMutableDictionary alloc] init];
        tvDict = [tvArray objectAtIndex:i];
        
        TVContentIdentifier *identifier = [[TVContentIdentifier alloc] initWithIdentifier:@"VOD" container:wrapperIdentifier];
        TVContentItem *contentItem = [[TVContentItem alloc] initWithContentIdentifier:identifier];
        
        contentItem.imageURL = [storeUrl URLByAppendingPathComponent:[tvDict valueForKey:@"thumb"] isDirectory:FALSE];
        contentItem.imageShape = TVContentItemImageShapePoster;
        contentItem.title = [tvDict valueForKey:@"title"];
        NSString *url = [tvDict valueForKey:@"url"];
        contentItem.displayURL = [NSURL URLWithString:[NSString stringWithFormat:@"kodi://display/tv/%@",url]];
        contentItem.playURL = [NSURL URLWithString:[NSString stringWithFormat:@"kodi://play/tv/%@",url]];
        [ContentItemsTv addObject:contentItem];
      }
      
      if ([tvosShared isJailbroken])
      {
        itemTv.title = [sharedDict valueForKey:@"tvTitle"];
      }
      else
      {
        itemTv.title = [shared stringForKey:@"tvTitle"];
      }

      itemTv.topShelfItems = ContentItemsTv;
      [topShelfItems addObject:itemTv];
    }
    return (NSArray *)topShelfItems;
}

@end
