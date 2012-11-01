/*
 *      Copyright (C) 2012 Team XBMC
 *      http://www.xbmc.org
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
#include "guilib/GUIWindowManager.h"
#include "guilib/GUIKeyboardFactory.h"
#include "threads/Event.h"
#include "Application.h"
#undef BOOL

#import "IOSKeyboardView.h"
#import "IOSScreenManager.h"
#import "XBMCController.h"
#include "IOSKeyboard.h"

static CEvent keyboardFinishedEvent;

@implementation KeyboardView
@synthesize _text;
@synthesize _heading;
@synthesize _result;
@synthesize _hiddenInput;
@synthesize _iosKeyboard;

- (id)initWithFrame:(CGRect)frame
{
  self = [super initWithFrame:frame];
  if (self) 
  {
    _iosKeyboard = nil;
    _text = [[NSMutableString alloc] initWithString:@""];
    _heading = [[NSMutableString alloc] initWithString:@""];    

    [[NSNotificationCenter defaultCenter] addObserver:self 
                                          selector:@selector(keyboardDidHide:) 
                                          name:UIKeyboardDidHideNotification
                                          object:nil];
    [self setBackgroundColor:[UIColor whiteColor]];
  }
  return self;
}

- (void)drawRect:(CGRect)rect
{
  CGRect rectForText = CGRectInset(rect, 10.0, 5.0);
  // init with heading
  NSMutableString *drawText = [[NSMutableString alloc] initWithString:self._heading];;
  [drawText appendString:@": "];
  
  if(_hiddenInput)//hidden input requested
  {
    NSMutableString *hiddenText = [[NSMutableString alloc] initWithString:@""];
    // hide chars with *
    for(unsigned int i = 0; i < [self._text length]; i++)
    {
      [hiddenText appendString:@"*"];
    }
    //append to heading
    [drawText appendString:hiddenText];
    [hiddenText release];
  }
  else
  {
    //append to heading
    [drawText appendString:self._text];
  }
  //finally draw the text
  [drawText drawInRect:rectForText withFont:[UIFont systemFontOfSize:14.0]];  
  [drawText release];
}

- (void)keyboardDidHide:(id)sender
{
  // user left the keyboard by hiding it
  // we treat this as a cancel
  _result = false;
  [self deactivate];
}

- (void) doActivate:(NSDictionary *)dict
{
  [g_xbmcController activateKeyboard:self];
  [self becomeFirstResponder];
}

- (void)activate
{
  if([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(doActivate:) withObject:nil  waitUntilDone:YES];  
  }
  else
  {
    // this would be fatal! We never should be called from the ios mainthread
    return;
  }

  keyboardFinishedEvent.Reset();

  // emulate a modale dialog here
  // we are waiting on the user finishing the keyboard
  // and have to process our app while doing that
  // basicall what our GUIDialog does if called modal
  while(!keyboardFinishedEvent.WaitMSec(500) && !g_application.m_bStop)
  {
    g_windowManager.ProcessRenderLoop();
  }
}

- (void) doDeactivate:(NSDictionary *)dict
{
  // allways calld in the mainloop context
  // detach the keyboard view from our main controller
  [g_xbmcController deactivateKeyboard:self];

  // invalidate our callback object
  if(_iosKeyboard)
  {
    _iosKeyboard->invalidateCallback();
    _iosKeyboard = nil;
  }
  // give back the control to whoever
  [self resignFirstResponder];
}

- (void) deactivate
{
  if([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(doDeactivate:) withObject:nil  waitUntilDone:YES];  
  }
  else
  {
    [self doDeactivate:nil];
  }

  keyboardFinishedEvent.Set();
}

// yes we can!
- (BOOL)canBecomeFirstResponder
{
    return YES; 
}

- (BOOL)hasText 
{
    return YES;
}

- (void) setText:(NSMutableString *)text
{
  [_text setString:@""];
  if([NSThread currentThread] != [NSThread mainThread])
  {
    [self performSelectorOnMainThread:@selector(insertText:) withObject:text  waitUntilDone:YES];  
  }
  else
  {
    [self insertText:text];
  }
}

- (void)insertText:(NSString *)newText 
{
  for(int i=0; i < [newText length];i++)
  {
    // we leave the keyboard ui if enter was hit
    // and treat it as an user confirmation
    if([newText characterAtIndex:i] == '\n')
    {
      _result = true;
      [self deactivate];
      return;
    }
  }
  
  // new typed text gets appended to the
  // whole string
  [self._text appendString:newText];
  [self setNeedsDisplay]; // update the UI
  
  // fire the string callback to our
  // handle realtime filtering and so on
  if(_iosKeyboard)
  {
    _iosKeyboard->fireCallback([_text UTF8String]);
  }
}

// handle backspace
- (void)deleteBackward 
{
  if([self._text length] == 0)
    return;

  NSRange rangeToDelete = NSMakeRange(self._text.length-1, 1);
  [self._text deleteCharactersInRange:rangeToDelete];
  [self setNeedsDisplay];

  // fire the string callback to our
  // handle realtime filtering and so on
  if(_iosKeyboard)
  {
    _iosKeyboard->fireCallback([_text UTF8String]);
  }  
}

- (void) dealloc
{
    [[NSNotificationCenter defaultCenter] removeObserver: self];
    [_text release];
    [_heading release];
    [super dealloc];
}
@end
