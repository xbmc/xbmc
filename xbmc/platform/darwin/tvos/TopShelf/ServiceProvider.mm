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
  static const auto storeUrl = [[tvosShared getSharedURL] URLByAppendingPathComponent:@"RA" isDirectory:YES];

  // Retrieve TopShelf categories
  static const auto isSandboxed = CDarwinEmbedUtils::IsIosSandboxed();
  NSDictionary* topshelfCategories;
  if (isSandboxed)
  {
    auto sharedSandboxDict = [[NSUserDefaults alloc] initWithSuiteName:[tvosShared getSharedID]];
    topshelfCategories = [sharedSandboxDict objectForKey:@"topshelfCategories"];
  }
  else
  {
    static const auto sharedJailbreakUrl = [storeUrl URLByAppendingPathComponent:@"topshelf.dict"
                                                                     isDirectory:NO];
    topshelfCategories = [NSDictionary dictionaryWithContentsOfURL:sharedJailbreakUrl];
  }

  // Retrieve Kodi URL Scheme
  static auto const mainAppBundle = [tvosShared mainAppBundle];
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
  auto contentItemsFrom = ^NSArray<TVContentItem*>*(NSString* categoryKey, NSArray* categoryItems)
  {
    NSMutableArray<TVContentItem*>* contentItems =
        [[NSMutableArray alloc] initWithCapacity:categoryItems.count];
    const auto thumbsPath = [storeUrl URLByAppendingPathComponent:categoryKey isDirectory:YES];
    for (NSDictionary* item in categoryItems)
    {
      auto identifier = [[TVContentIdentifier alloc] initWithIdentifier:@"VOD"
                                                                     container:wrapperIdentifier];
      auto contentItem = [[TVContentItem alloc] initWithContentIdentifier:identifier];

      [contentItem setImageURL:[thumbsPath URLByAppendingPathComponent:item[@"thumb"] isDirectory:NO]
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

  [topshelfCategories enumerateKeysAndObjectsUsingBlock:^(NSString* categoryKey, NSDictionary* categoryDict, BOOL *stop) {
    auto categoryContent = [[TVContentItem alloc] initWithContentIdentifier:wrapperIdentifier];
    categoryContent.title = categoryDict[@"categoryTitle"];
    categoryContent.topShelfItems = contentItemsFrom(categoryKey, categoryDict[@"categoryItems"]);
    [topShelfItems addObject:categoryContent];
  }];

  return topShelfItems;
}

@end
