/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "DarwinEmbedKeyboardView.h"

#include "Application.h"
#include "guilib/GUIKeyboardFactory.h"
#include "threads/Event.h"
#include "utils/log.h"

#import "platform/darwin/ios/XBMCController.h"

static CEvent keyboardFinishedEvent;

static const int INPUT_BOX_HEIGHT = 30;
static const int SPACE_BETWEEN_INPUT_AND_KEYBOARD = 0;

@implementation KeyboardView

@synthesize confirmed = m_confirmed;
@synthesize darwinEmbedKeyboard = m_darwinEmbedKeyboard;
@synthesize text = m_text;

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  if (!self)
    return nil;

  m_confirmed = NO;
  m_canceled = NULL;
  m_deactivated = NO;

  m_text = [NSMutableString string];

  // default input box position above the half screen.
  CGRect textFieldFrame =
      CGRectMake(frame.size.width / 2,
                 frame.size.height / 2 - INPUT_BOX_HEIGHT - SPACE_BETWEEN_INPUT_AND_KEYBOARD,
                 frame.size.width / 2, INPUT_BOX_HEIGHT);
  m_inputTextField = [[UITextField alloc] initWithFrame:textFieldFrame];
  m_inputTextField.clearButtonMode = UITextFieldViewModeAlways;
  m_inputTextField.borderStyle = UITextBorderStyleNone;
  m_inputTextField.returnKeyType = UIReturnKeyDone;
  m_inputTextField.autocapitalizationType = UITextAutocapitalizationTypeNone;
  m_inputTextField.backgroundColor = [UIColor whiteColor];
  m_inputTextField.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
  m_inputTextField.delegate = self;

  CGRect labelFrame = textFieldFrame;
  labelFrame.origin.x = 0;
  m_inputTextHeading = [[UITextField alloc] initWithFrame:labelFrame];
  m_inputTextHeading.borderStyle = UITextBorderStyleNone;
  m_inputTextHeading.backgroundColor = [UIColor whiteColor];
  m_inputTextHeading.adjustsFontSizeToFitWidth = YES;
  m_inputTextHeading.contentVerticalAlignment = UIControlContentVerticalAlignmentCenter;
  m_inputTextHeading.enabled = NO;

  [self addSubview:m_inputTextHeading];
  [self addSubview:m_inputTextField];

  self.userInteractionEnabled = YES;

  [[NSNotificationCenter defaultCenter] addObserver:self
                                           selector:@selector(textChanged:)
                                               name:UITextFieldTextDidChangeNotification
                                             object:m_inputTextField];
  return self;
}

- (void)layoutSubviews
{
  CGFloat headingW = 0;
  if (m_inputTextHeading.text && m_inputTextHeading.text.length > 0)
  {
    CGSize headingSize = [m_inputTextHeading.text sizeWithAttributes:@{
      NSFontAttributeName : [UIFont systemFontOfSize:[UIFont systemFontSize]]
    }];

    headingW = MIN(self.bounds.size.width / 2, headingSize.width + 30);
  }

  CGFloat y = m_kbRect.origin.y - INPUT_BOX_HEIGHT - SPACE_BETWEEN_INPUT_AND_KEYBOARD;

  m_inputTextHeading.frame = CGRectMake(0, y, headingW, INPUT_BOX_HEIGHT);
  m_inputTextField.frame =
      CGRectMake(headingW, y, self.bounds.size.width - headingW, INPUT_BOX_HEIGHT);
}

- (void)textFieldDidEndEditing:(UITextField*)textField
{
  [self deactivate];
}

- (BOOL)textFieldShouldReturn:(UITextField*)textField
{
  m_confirmed = YES;

  return YES;
}

- (void)activate
{
  dispatch_sync(dispatch_get_main_queue(), ^{
    [g_xbmcController activateKeyboard:self];
    [m_inputTextField becomeFirstResponder];
    [self setNeedsLayout];
    keyboardFinishedEvent.Reset();
  });

  // we are waiting on the user finishing the keyboard
  while (!keyboardFinishedEvent.WaitMSec(500))
  {
    if (nullptr != m_canceled && *m_canceled)
    {
      [self deactivate];
      m_canceled = nullptr;
    }
  }
}

- (void)deactivate
{
  m_deactivated = YES;

  // invalidate our callback object
  if (m_darwinEmbedKeyboard)
  {
    m_darwinEmbedKeyboard->invalidateCallback();
    m_darwinEmbedKeyboard = nil;
  }
  // give back the control to whoever
  [m_inputTextField resignFirstResponder];

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

- (void)setKeyboardText:(NSString*)aText closeKeyboard:(BOOL)closeKeyboard
{
  CLog::Log(LOGDEBUG, "{}: {}, {}", __PRETTY_FUNCTION__, aText.UTF8String, closeKeyboard);

  dispatch_sync(dispatch_get_main_queue(), ^{
    m_inputTextField.text = aText;
    [self textChanged:nil];
  });

  if (closeKeyboard)
  {
    m_confirmed = YES;
    [self deactivate];
  }
}

- (void)setHeading:(NSString*)heading
{
  dispatch_sync(dispatch_get_main_queue(), ^{
    if (heading && heading.length > 0)
      m_inputTextHeading.text = [NSString stringWithFormat:@" %@:", heading];
    else
      m_inputTextHeading.text = nil;
  });
}

- (void)setHidden:(BOOL)hidden
{
  dispatch_sync(dispatch_get_main_queue(), ^{
    m_inputTextField.secureTextEntry = hidden;
  });
}

- (void)textChanged:(NSNotification*)aNotification
{
  if (![self.text isEqualToString:m_inputTextField.text])
  {
    [self.text setString:m_inputTextField.text];
    if (m_darwinEmbedKeyboard)
      m_darwinEmbedKeyboard->fireCallback([self text].UTF8String);
  }
}

- (void)setCancelFlag:(bool*)cancelFlag
{
  m_canceled = cancelFlag;
}

@end
