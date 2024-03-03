/*
 *  Copyright (C) 2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#import <Foundation/Foundation.h>

@interface CMediaKeyTap : NSObject

- (void)enableMediaKeyTap;
- (void)disableMediaKeyTap;
- (bool)HandleMediaKey:(int)keyCode;

@end
