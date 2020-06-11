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

#include "platform/darwin/ios-common/DarwinEmbedUtils.h"

@implementation ServiceProvider

#pragma mark - TVTopShelfProvider protocol

- (TVTopShelfContentStyle)topShelfStyle
{
  return TVTopShelfContentStyleSectioned;
}

- (NSArray<TVContentItem*>*)topShelfItems
{
  // Retrieve store URL
  auto storeUrl = [tvosShared getSharedURL];
  if (!storeUrl)
    return @[];
  storeUrl = [storeUrl URLByAppendingPathComponent:@"RA" isDirectory:YES];


  // Retrieve shared dict
  NSDictionary* sharedDict;
  if (CDarwinEmbedUtils::IsIosSandboxed())
  {
    auto const sharedID = [tvosShared getSharedID];
    auto const shared = [[NSUserDefaults alloc] initWithSuiteName:sharedID];
    sharedDict = shared.dictionaryRepresentation;
  }
  else
  {
    auto sharedDictUrl = [storeUrl URLByAppendingPathComponent:@"shared.dict"
                                                          isDirectory:NO];
    sharedDict = [NSDictionary dictionaryWithContentsOfFile:[sharedDictUrl path]];
  }


  // Retrieve Kodi URL Scheme
  auto const mainAppBundle = [tvosShared mainAppBundle];
  auto kodiUrlScheme = @"kodi"; // fallback value
  for (NSDictionary* dic in [mainAppBundle objectForInfoDictionaryKey:@"CFBundleURLTypes"])
  {
    if ([dic[@"CFBundleURLName"] isEqualToString:mainAppBundle.bundleIdentifier])
    {
      kodiUrlScheme = dic[@"CFBundleURLSchemes"][0];
      break;
    }
  }

  auto wrapperIdentifier = [[TVContentIdentifier alloc] initWithIdentifier:@"shelf-wrapper"
                                                                        container:nil];


  // Function to create a TVContentItem array from an array of items (a category)
  auto contentItemsFrom = ^NSArray<TVContentItem*>*(NSArray* categoryItems)
  {
    NSMutableArray<TVContentItem*>* contentItems =
        [[NSMutableArray alloc] initWithCapacity:categoryItems.count];
    for (NSDictionary* item in categoryItems)
    {
      auto identifier = [[TVContentIdentifier alloc] initWithIdentifier:@"VOD"
                                                                     container:wrapperIdentifier];
      auto contentItem = [[TVContentItem alloc] initWithContentIdentifier:identifier];

      [contentItem setImageURL:[storeUrl URLByAppendingPathComponent:item[@"thumb"] isDirectory:NO]
                     forTraits:TVContentItemImageTraitScreenScale1x];
      contentItem.imageShape = TVContentItemImageShapePoster;
      contentItem.title = item[@"title"];
      NSString* url = item[@"url"];
      contentItem.displayURL = [NSURL
          URLWithString:[NSString stringWithFormat:@"%@://display/movie/%@", kodiUrlScheme, url]];
      contentItem.playURL = [NSURL
          URLWithString:[NSString stringWithFormat:@"%@://play/movie/%@", kodiUrlScheme, url]];
      [contentItems addObject:contentItem];
    }
    return contentItems;
  };

  // Add each category to TopShelf
  auto topShelfItems = [[NSMutableArray alloc] init];

  [sharedDict enumerateKeysAndObjectsUsingBlock:^(NSString* categoryKey, id categoryDict, BOOL *stop) {
    if (![categoryDict isKindOfClass:[NSDictionary class]])
      return;
    NSArray* categoryItems = categoryDict[@"categoryItems"];
    if (!categoryItems)
      return;
    if (categoryItems.count == 0)
      return;

    auto categoryContent = [[TVContentItem alloc] initWithContentIdentifier:wrapperIdentifier];
    categoryContent.title = categoryDict[@"categoryTitle"];
    categoryContent.topShelfItems = contentItemsFrom(categoryItems);
    [topShelfItems addObject:categoryContent];
  }];

  return topShelfItems;
}

@end
