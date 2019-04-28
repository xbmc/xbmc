/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/darwin/tvos/MainKeyboard.h"

#import <UIKit/UIKit.h>

@interface KeyboardView : UIView <UITextFieldDelegate>
{
  NSMutableString* _text;
  BOOL _confirmed;
  CMainKeyboard* _tvosKeyboard;
  bool* _canceled;
  BOOL _deactivated;
  UITextField* _textField;
  UITextField* _heading;
  CGRect _kbRect;
  CGRect _frame;
}

@property (nonatomic, retain) NSMutableString* _text;
@property (getter = isConfirmed) BOOL _confirmed;
@property (assign, setter = registerKeyboard:) CMainKeyboard* _tvosKeyboard;
@property CGRect _frame;

- (void) setHeading:(NSString*)heading;
- (void) setHidden:(BOOL)hidden;
- (void) activate;
- (void) deactivate;
- (void) setKeyboardText:(NSString*)aText closeKeyboard:(BOOL)closeKeyboard;
- (void) textChanged:(NSNotification*)aNotification;
- (void) setCancelFlag:(bool*)cancelFlag;
- (void) doDeactivate:(NSDictionary*)dict;
- (id)initWithFrameInternal;

@end
