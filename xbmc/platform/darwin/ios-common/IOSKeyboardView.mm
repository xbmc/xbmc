/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "platform/darwin/ios-common/IOSKeyboardView.h"

#include "Application.h"
#include "guilib/GUIKeyboardFactory.h"
#include "threads/Event.h"

#import "platform/darwin/NSLogDebugHelpers.h"
#import "platform/darwin/ios-common/IOSKeyboard.h"
#import "platform/darwin/ios/IOSScreenManager.h"
#import "platform/darwin/ios/XBMCController.h"

static CEvent keyboardFinishedEvent;

#define INPUT_BOX_HEIGHT 30
#define SPACE_BETWEEN_INPUT_AND_KEYBOARD 0

@implementation KeyboardView
@synthesize text;
@synthesize _confirmed;
@synthesize _iosKeyboard;
@synthesize _frame;

- (id)initWithFrame:(CGRect)frame
{
  _frame = frame;
  if ([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(initWithFrameInternal)
                           withObject:nil
                        waitUntilDone:YES];
  }
  else
  {
    self = [self initWithFrameInternal];
  }
  return self;
}

- (id)initWithFrameInternal
{
  CGRect frame = _frame;
  self = [super initWithFrame:frame];
  if (self)
  {
    _iosKeyboard = nil;
    _keyboardIsShowing = 0;
    _confirmed = NO;
    _canceled = NULL;
    _deactivated = NO;

    self.text = [NSMutableString stringWithString:@""];

    // default input box position above the half screen.
    CGRect textFieldFrame =
        CGRectMake(frame.size.width / 2,
                   frame.size.height / 2 - INPUT_BOX_HEIGHT - SPACE_BETWEEN_INPUT_AND_KEYBOARD,
                   frame.size.width / 2, INPUT_BOX_HEIGHT);
    _textField = [[UITextField alloc] initWithFrame:textFieldFrame];
    _textField.clearButtonMode = UITextFieldViewModeAlways;
    // UITextBorderStyleRoundedRect; - with round rect we can't control backgroundcolor
    _textField.borderStyle = UITextBorderStyleNone;
    _textField.returnKeyType = UIReturnKeyDone;
    _textField.autocapitalizationType = UITextAutocapitalizationTypeNone;
    _textField.backgroundColor = [UIColor whiteColor];
    _textField.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
    _textField.delegate = self;

    CGRect labelFrame = textFieldFrame;
    labelFrame.origin.x = 0;
    _heading = [[UITextField alloc] initWithFrame:labelFrame];
    _heading.borderStyle = UITextBorderStyleNone;
    _heading.backgroundColor = [UIColor whiteColor];
    _heading.adjustsFontSizeToFitWidth = YES;
    _heading.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
    _heading.enabled = NO;

    [self addSubview:_heading];
    [self addSubview:_textField];

    self.userInteractionEnabled = YES;

    [self setAlpha:0.9];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(textChanged:)
                                                 name:UITextFieldTextDidChangeNotification
                                               object:_textField];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardDidHide:)
                                                 name:UIKeyboardDidHideNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardDidChangeFrame:)
                                                 name:UIKeyboardDidChangeFrameNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardWillShow:)
                                                 name:UIKeyboardWillShowNotification
                                               object:nil];
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(keyboardDidShow:)
                                                 name:UIKeyboardDidShowNotification
                                               object:nil];
  }
  return self;
}

- (void)layoutSubviews
{
  CGFloat headingW = 0;
  if (_heading.text and _heading.text.length > 0)
  {
    CGSize headingSize = [_heading.text sizeWithAttributes:@{
      NSFontAttributeName : [UIFont systemFontOfSize:[UIFont systemFontSize]]
    }];
    headingW = MIN(self.bounds.size.width / 2, headingSize.width + 30);
  }

  CGFloat y = _kbRect.origin.y - INPUT_BOX_HEIGHT - SPACE_BETWEEN_INPUT_AND_KEYBOARD;

  _heading.frame = CGRectMake(0, y, headingW, INPUT_BOX_HEIGHT);
  _textField.frame = CGRectMake(headingW, y, self.bounds.size.width - headingW, INPUT_BOX_HEIGHT);
}

- (void)keyboardWillShow:(NSNotification*)notification
{
  NSDictionary* info = [notification userInfo];
  CGRect kbRect = [[info objectForKey:UIKeyboardFrameEndUserInfoKey] CGRectValue];
  LOG(@"keyboardWillShow: keyboard frame: %@", NSStringFromCGRect(kbRect));
  _kbRect = kbRect;
  [self setNeedsLayout];
  _keyboardIsShowing = 1;
}

- (void)keyboardDidShow:(NSNotification*)notification
{
  LOG(@"keyboardDidShow: deactivated: %d", _deactivated);
  _keyboardIsShowing = 2;
  if (_deactivated)
    [self doDeactivate:nil];
}

- (void)touchesBegan:(NSSet*)touches withEvent:(UIEvent*)event
{
  PRINT_SIGNATURE();
  [_textField resignFirstResponder];
}

- (BOOL)textFieldShouldEndEditing:(UITextField*)textField
{
  LOG(@"%s: keyboard IsShowing %d", __PRETTY_FUNCTION__, _keyboardIsShowing);
  // Do not break the keyboard show up process, else we will lost
  // keyboard did hide notification.
  return _keyboardIsShowing != 1;
}

- (void)textFieldDidEndEditing:(UITextField*)textField
{
  PRINT_SIGNATURE();
  [self deactivate];
}

- (BOOL)textFieldShouldReturn:(UITextField*)textField
{
  PRINT_SIGNATURE();
  _confirmed = YES;
  [_textField resignFirstResponder];
  return YES;
}

- (void)keyboardDidChangeFrame:(id)sender
{
}

- (void)keyboardDidHide:(id)sender
{
  PRINT_SIGNATURE();

  _keyboardIsShowing = 0;

  if (_textField.editing)
  {
    LOG(@"kb hide when editing, it could be a language switch");
    return;
  }

  [self deactivate];
}

- (void)doActivate:(NSDictionary*)dict
{
  PRINT_SIGNATURE();
  [g_xbmcController activateKeyboard:self];
  [_textField becomeFirstResponder];
  [self setNeedsLayout];
  keyboardFinishedEvent.Reset();
}

- (void)activate
{
  PRINT_SIGNATURE();
  if ([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(doActivate:) withObject:nil waitUntilDone:YES];
  }
  else
  {
    // this would be fatal! We never should be called from the ios mainthread
    return;
  }

  // we are waiting on the user finishing the keyboard
  while (!keyboardFinishedEvent.WaitMSec(500))
  {
    if (NULL != _canceled && *_canceled)
    {
      [self deactivate];
      _canceled = NULL;
    }
  }
}

- (void)doDeactivate:(NSDictionary*)dict
{
  LOG(@"%s: keyboard IsShowing %d", __PRETTY_FUNCTION__, _keyboardIsShowing);
  _deactivated = YES;

  // Do not break keyboard show up process, if so there's a bug of ios4 will not
  // notify us keyboard hide.
  if (_keyboardIsShowing == 1)
    return;

  // invalidate our callback object
  if (_iosKeyboard)
  {
    _iosKeyboard->invalidateCallback();
    _iosKeyboard = nil;
  }
  // give back the control to whoever
  [_textField resignFirstResponder];

  // delay closing view until text field finishes resigning first responder
  dispatch_async(dispatch_get_main_queue(), ^{
    // always called in the mainloop context
    // detach the keyboard view from our main controller
    [g_xbmcController deactivateKeyboard:self];

    // no more notification we want to receive.
    [[NSNotificationCenter defaultCenter] removeObserver:self];

    keyboardFinishedEvent.Set();
  });
}

- (void)deactivate
{
  PRINT_SIGNATURE();
  if ([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(doDeactivate:) withObject:nil waitUntilDone:YES];
  }
  else
  {
    [self doDeactivate:nil];
  }
}

- (void)setKeyboardText:(NSString*)aText closeKeyboard:(BOOL)closeKeyboard
{
  LOG(@"%s: %@, %d", __PRETTY_FUNCTION__, aText, closeKeyboard);
  if ([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(setDefault:) withObject:aText waitUntilDone:YES];
  }
  else
  {
    [self setDefault:aText];
  }
  if (closeKeyboard)
  {
    _confirmed = YES;
    [self deactivate];
  }
}

- (void)setHeading:(NSString*)heading
{
  if ([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(setHeadingInternal:)
                           withObject:heading
                        waitUntilDone:YES];
  }
  else
  {
    [self setHeadingInternal:heading];
  }
}

- (void)setHeadingInternal:(NSString*)heading
{
  if (heading && heading.length > 0)
  {
    _heading.text = [NSString stringWithFormat:@" %@:", heading];
  }
  else
  {
    _heading.text = nil;
  }
}

- (void)setDefault:(NSString*)defaultText
{
  [_textField setText:defaultText];
  [self textChanged:nil];
}

- (void)setHiddenInternal:(NSNumber*)hidden
{
  BOOL hiddenBool = [hidden boolValue];
  [_textField setSecureTextEntry:hiddenBool];
}

- (void)setHidden:(BOOL)hidden
{
  NSNumber* passedValue = [NSNumber numberWithBool:hidden];

  if ([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(setHiddenInternal:)
                           withObject:passedValue
                        waitUntilDone:YES];
  }
  else
  {
    [self setHiddenInternal:passedValue];
  }
}

- (void)textChanged:(NSNotification*)aNotification
{
  if (![self.text isEqualToString:_textField.text])
  {
    [self.text setString:_textField.text];
    if (_iosKeyboard)
    {
      _iosKeyboard->fireCallback([self.text UTF8String]);
    }
  }
}

- (void)setCancelFlag:(bool*)cancelFlag
{
  _canceled = cancelFlag;
}

@end
