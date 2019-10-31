/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/darwin/ios-common/IOSKeyboard.h"

#import <UIKit/UIKit.h>

@interface KeyboardView : UIView <UITextFieldDelegate>
{
  NSMutableString* text;
  BOOL _confirmed;
  CIOSKeyboard* _iosKeyboard;
  bool* _canceled;
  BOOL _deactivated;
  UITextField* _textField;
  UITextField* _heading;
  int _keyboardIsShowing; // 0: not, 1: will show, 2: showing
  CGRect _kbRect;
}

@property(nonatomic, strong) NSMutableString* text;
@property(getter=isConfirmed) BOOL _confirmed;
@property(assign, setter=registerKeyboard:) CIOSKeyboard* _iosKeyboard;
@property CGRect _frame;

- (void)setHeading:(NSString*)heading;
- (void)setHidden:(BOOL)hidden;
- (void)activate;
- (void)deactivate;
- (void)setKeyboardText:(NSString*)aText closeKeyboard:(BOOL)closeKeyboard;
- (void)textChanged:(NSNotification*)aNotification;
- (void)setCancelFlag:(bool*)cancelFlag;
- (void)doDeactivate:(NSDictionary*)dict;
- (id)initWithFrameInternal;
@end
