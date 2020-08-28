/*
 *  Copyright (C) 2019- Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "IOSKeyboardView.h"

#import "utils/log.h"

static const CGFloat INPUT_BOX_HEIGHT = 30;

@interface IOSKeyboardView ()
@property(nonatomic, weak) UIView* textFieldContainer;
@property(nonatomic, weak) NSLayoutConstraint* containerBottomConstraint;
@end

@implementation IOSKeyboardView

@synthesize textFieldContainer = m_textFieldContainer;
@synthesize containerBottomConstraint = m_containerBottomConstraint;

- (instancetype)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  if (!self)
    return nil;

  self.backgroundColor = [UIColor colorWithWhite:0 alpha:0.1];

  auto notificationCenter = NSNotificationCenter.defaultCenter;
  [notificationCenter addObserver:self
                         selector:@selector(keyboardDidHide:)
                             name:UIKeyboardDidHideNotification
                           object:nil];
  [notificationCenter addObserver:self
                         selector:@selector(keyboardDidChangeFrame:)
                             name:UIKeyboardDidChangeFrameNotification
                           object:nil];
  [notificationCenter addObserver:self
                         selector:@selector(keyboardWillShow:)
                             name:UIKeyboardWillShowNotification
                           object:nil];
  [notificationCenter addObserver:self
                         selector:@selector(keyboardDidShow:)
                             name:UIKeyboardDidShowNotification
                           object:nil];

  [self addGestureRecognizer:[[UITapGestureRecognizer alloc]
                                 initWithTarget:m_inputTextField
                                         action:@selector(resignFirstResponder)]];

  auto textBackgroundColor = UIColor.whiteColor;
  if (@available(iOS 13.0, *))
    textBackgroundColor = UIColor.systemBackgroundColor;

  auto textFieldContainer = [UIView new];
  textFieldContainer.translatesAutoresizingMaskIntoConstraints = NO;
  textFieldContainer.backgroundColor = textBackgroundColor;
  [textFieldContainer addSubview:m_inputTextField];
  [self addSubview:textFieldContainer];
  m_textFieldContainer = textFieldContainer;

  auto bottomConstraint = [textFieldContainer.bottomAnchor constraintEqualToAnchor:self.topAnchor];
  m_containerBottomConstraint = bottomConstraint;

  [NSLayoutConstraint activateConstraints:@[
    bottomConstraint,
    [textFieldContainer.leadingAnchor constraintEqualToAnchor:self.leadingAnchor],
    [textFieldContainer.trailingAnchor constraintEqualToAnchor:self.trailingAnchor],
    [textFieldContainer.heightAnchor constraintEqualToConstant:INPUT_BOX_HEIGHT],

    [m_inputTextField.widthAnchor constraintEqualToAnchor:textFieldContainer.widthAnchor
                                               multiplier:0.5],
    [m_inputTextField.centerXAnchor constraintEqualToAnchor:textFieldContainer.centerXAnchor],
    [m_inputTextField.topAnchor constraintEqualToAnchor:textFieldContainer.topAnchor],
    [m_inputTextField.bottomAnchor constraintEqualToAnchor:textFieldContainer.bottomAnchor],
  ]];

  return self;
}

- (void)keyboardWillShow:(NSNotification*)notification
{
  CLog::Log(LOGDEBUG, "{} {}", __PRETTY_FUNCTION__, notification.userInfo.description.UTF8String);
  if (!self.isKeyboardVisible)
    self.textFieldContainer.hidden = YES;
}

- (void)keyboardDidShow:(NSNotification*)notification
{
  CLog::Log(LOGDEBUG, "{} deactivated: {}, {}", __PRETTY_FUNCTION__, m_deactivated,
            notification.userInfo.description.UTF8String);
  self.keyboardVisible = true;
  self.textFieldContainer.hidden = NO;

  if (m_deactivated)
    [self deactivate];
}

- (void)keyboardDidChangeFrame:(NSNotification*)notification
{
  auto keyboardFrame = [notification.userInfo[UIKeyboardFrameEndUserInfoKey] CGRectValue];
  // although converting isn't really necessary in our case, as the holder view occupies
  // the whole screen, technically it's more correct
  auto convertedFrame = [self convertRect:keyboardFrame
                      fromCoordinateSpace:UIScreen.mainScreen.coordinateSpace];
  self.containerBottomConstraint.constant = CGRectGetMinY(convertedFrame);
  [self layoutIfNeeded];
}

- (void)keyboardDidHide:(id)sender
{
  if (m_inputTextField.editing)
  {
    CLog::Log(LOGDEBUG, "kb hide when editing, it could be a language switch");
    return;
  }

  self.keyboardVisible = false;
  [self deactivate];
}

- (void)setKeyboardText:(NSString*)aText closeKeyboard:(BOOL)closeKeyboard
{

  [super setKeyboardText:aText closeKeyboard:closeKeyboard];

  if (closeKeyboard)
  {
    self.confirmed = YES;
    [self deactivate];
  }
}

@end
