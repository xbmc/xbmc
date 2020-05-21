/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "platform/darwin/ios-common/DarwinEmbedKeyboard.h"

#import <UIKit/UIKit.h>

@interface KeyboardView : UIView <UITextFieldDelegate>
{
  bool* m_canceled;
  BOOL m_deactivated;
  UITextField* __weak m_inputTextField;
}

@property(getter=isConfirmed) BOOL confirmed;
@property(assign) CDarwinEmbedKeyboard* darwinEmbedKeyboard;
@property(nonatomic, readonly) NSString* text;
@property(nonatomic, assign, getter=isKeyboardVisible) bool keyboardVisible;

- (void)setHeading:(NSString*)heading;
- (void)setHidden:(BOOL)hidden;
- (void)activate;
- (void)deactivate;
- (void)setKeyboardText:(NSString*)aText closeKeyboard:(BOOL)closeKeyboard;
- (void)textChanged:(NSNotification*)aNotification;
- (void)setCancelFlag:(bool*)cancelFlag;

@end
