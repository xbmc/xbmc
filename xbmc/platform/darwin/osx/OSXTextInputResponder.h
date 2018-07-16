/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import <Cocoa/Cocoa.h>

@interface OSXTextInputResponder : NSView <NSTextInputClient>
{
  NSString *_markedText;
  NSRange   _markedRange;
  NSRange   _selectedRange;
  NSRect    _inputRect;
}
- (void) setInputRect:(NSRect) rect;
@end
