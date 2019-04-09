/*
 *      Copyright (C) 2012-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#define BOOL XBMC_BOOL
#import "Application.h"
#import "guilib/GUIKeyboardFactory.h"
#import "threads/Event.h"
#undef BOOL

#import "platform/darwin/tvos/MainKeyboardView.h"
#import "platform/darwin/DarwinUtils.h"
#import "platform/darwin/tvos/MainController.h"
#import "platform/darwin/tvos/MainKeyboard.h"
#import "platform/darwin/NSLogDebugHelpers.h"


static CEvent keyboardFinishedEvent;

#define INPUT_BOX_HEIGHT 30
#define SPACE_BETWEEN_INPUT_AND_KEYBOARD 0

@implementation KeyboardView
@synthesize _text;
@synthesize _confirmed;
@synthesize _tvosKeyboard;
@synthesize _frame;

- (id)initWithFrameInternal
{
  CGRect frame = _frame;
  self = [super initWithFrame:frame];
  if (self)
  {
    _tvosKeyboard = nil;
    _confirmed = NO;
    _canceled = NULL;
    _deactivated = NO;
    
    self._text = [NSMutableString stringWithString:@""];
    
    // default input box position above the half screen.
    CGRect textFieldFrame = CGRectMake(frame.size.width/2,
                                       frame.size.height/2-INPUT_BOX_HEIGHT-SPACE_BETWEEN_INPUT_AND_KEYBOARD,
                                       frame.size.width/2,
                                       INPUT_BOX_HEIGHT);
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
    
    [[NSNotificationCenter defaultCenter] addObserver:self
                                             selector:@selector(textChanged:) name:UITextFieldTextDidChangeNotification object:_textField];
  }

  return self;
}

- (id)initWithFrame:(CGRect)frame
{
  _frame = frame;
  if([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(initWithFrameInternal) withObject:nil  waitUntilDone:YES];
  }
  else
  {
    [self initWithFrameInternal:nil];
  }
  /*
  self = [super initWithFrame:frame];
  if (self)
  {
    _tvosKeyboard = nil;
    _confirmed = NO;
    _canceled = NULL;
    _deactivated = NO;
    
    self._text = [NSMutableString stringWithString:@""];

   // default input box position above the half screen.
    CGRect textFieldFrame = CGRectMake(frame.size.width/2, 
                                       frame.size.height/2-INPUT_BOX_HEIGHT-SPACE_BETWEEN_INPUT_AND_KEYBOARD, 
                                       frame.size.width/2, 
                                       INPUT_BOX_HEIGHT);
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

    [[NSNotificationCenter defaultCenter] addObserver:self
      selector:@selector(textChanged:) name:UITextFieldTextDidChangeNotification object:_textField];
  }*/
  return self;
}

- (void)layoutSubviews
{
  //PRINT_SIGNATURE();
  CGFloat headingW = 0;
  if (_heading.text and _heading.text.length > 0)
  {
    //CGSize headingSize = [_heading.text sizeWithFont:[UIFont systemFontOfSize:[UIFont systemFontSize]]];

    CGSize headingSize;
    headingSize.width = 25;
    headingSize.height = 25;
    headingW = MIN(self.bounds.size.width/2, headingSize.width+30);
  }
  CGFloat kbHeight = _kbRect.size.width;

  CGFloat y = kbHeight <= 0 ?
    _textField.frame.origin.y : 
    MIN(self.bounds.size.height - kbHeight, self.bounds.size.height/5*3) - INPUT_BOX_HEIGHT - SPACE_BETWEEN_INPUT_AND_KEYBOARD;

  if (CDarwinUtils::GetIOSVersion() >= 8.0)
    y = _kbRect.origin.y - INPUT_BOX_HEIGHT - SPACE_BETWEEN_INPUT_AND_KEYBOARD;

  _heading.frame = CGRectMake(0, y, headingW, INPUT_BOX_HEIGHT);
  _textField.frame = CGRectMake(headingW, y, self.bounds.size.width-headingW, INPUT_BOX_HEIGHT);
}

- (void)textFieldDidEndEditing:(UITextField *)textField
{
  //PRINT_SIGNATURE();
  // when user hits 'done' button or has canceled by menuing out
  [self deactivate];
}

-(BOOL)textFieldShouldReturn:(UITextField *)textField{
  //PRINT_SIGNATURE();
  // when user hits 'done' button
  _confirmed = YES;
  return YES;
}

- (void) doActivate:(NSDictionary *)dict
{
  //PRINT_SIGNATURE();
  [g_xbmcController activateKeyboard:self];
  [_textField becomeFirstResponder];
  [self setNeedsLayout];
  keyboardFinishedEvent.Reset();
}

- (void)activate
{
  //PRINT_SIGNATURE();
  if([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(doActivate:) withObject:nil  waitUntilDone:YES];  
  }
  else
  {
    // this would be fatal! We never should be called from the ios mainthread
    return;
  }

  // we are waiting on the user finishing the keyboard
  while(!keyboardFinishedEvent.WaitMSec(500))
  {
    if (NULL != _canceled && *_canceled)
    {
      [self deactivate];
      _canceled = NULL;
    }
  }
}

- (void) doDeactivate:(NSDictionary *)dict
{
  //PRINT_SIGNATURE();
  _deactivated = YES;
  
  // invalidate our callback object
  if(_tvosKeyboard)
  {
    _tvosKeyboard->invalidateCallback();
    _tvosKeyboard = nil;
  }
  // give back the control to whoever
  [_textField resignFirstResponder];

  // allways calld in the mainloop context
  // detach the keyboard view from our main controller
  [g_xbmcController deactivateKeyboard:self];
  
  // no more notification we want to receive.
  [[NSNotificationCenter defaultCenter] removeObserver: self];

  keyboardFinishedEvent.Set();
}

- (void) deactivate
{
  //PRINT_SIGNATURE();
  if([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(doDeactivate:) withObject:nil  waitUntilDone:YES];  
  }
  else
  {
    [self doDeactivate:nil];
  }
}

- (void) setKeyboardText:(NSString*)aText closeKeyboard:(BOOL)closeKeyboard
{
  //LOG(@"%s: %@, %d", __PRETTY_FUNCTION__, aText, closeKeyboard);
  if([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(setDefault:) withObject:aText  waitUntilDone:YES];
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

- (void) setHeading:(NSString *)heading
{
  if([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(setHeadingInternal:) withObject:heading  waitUntilDone:YES];
  }
  else
  {
    [self setHeadingInternal:heading];
  }
}

- (void) setHeadingInternal:(NSString *)heading
{
  if (heading && heading.length > 0) {
    _heading.text = [NSString stringWithFormat:@" %@:", heading];
  }
  else {
    _heading.text = nil;
  }
}

- (void) setDefault:(NSString *)defaultText
{
  [_textField setText:defaultText];
  [self textChanged:nil];
}

- (void) setHidden:(BOOL)hidden
{
  NSNumber *passedValue = [NSNumber numberWithBool:hidden];
  if([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(setHiddenInternal:) withObject:passedValue  waitUntilDone:YES];
  }
  else
  {
    [self setHiddenInternal:passedValue];
  }
}

- (void) setHiddenInternal:(NSNumber *)hidden
{
  BOOL hiddenBool = [hidden boolValue];
  [_textField setSecureTextEntry:hiddenBool];
}

- (void) textChanged:(NSNotification*)aNotification; {
  if (![self._text isEqualToString:_textField.text])
  {
    [self._text setString:_textField.text];
    if (_tvosKeyboard)
    {
      _tvosKeyboard->fireCallback([self._text UTF8String]);
    }
  }
}

- (void) setCancelFlag:(bool *)cancelFlag
{
  _canceled = cancelFlag;
}

- (void) dealloc
{
  //PRINT_SIGNATURE();
  self._text = nil;
  [super dealloc];
}
@end
