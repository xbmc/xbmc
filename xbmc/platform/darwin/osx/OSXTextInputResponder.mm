/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#import "OSXTextInputResponder.h"

#include "GUIUserMessages.h"
#include "guilib/GUIWindowManager.h"
#include "input/Key.h"
#include "messaging/ApplicationMessenger.h"
#include "utils/log.h"

using namespace KODI::MESSAGING;

void SendKeyboardText(const char *text)
{
//  CLog::Log(LOGDEBUG, "SendKeyboardText(%s)", text);

  /* Don't post text events for unprintable characters */
  if ((unsigned char)*text < ' ' || *text == 127)
    return;

  CAction *action = new CAction(ACTION_INPUT_TEXT);
  action->SetText(text);
  CApplicationMessenger::GetInstance().PostMsg(TMSG_GUI_ACTION, WINDOW_INVALID, -1, static_cast<void*>(action));
}

void SendEditingText(const char *text, unsigned int location, unsigned int length)
{
  //@todo fix this hack
//  CLog::Log(LOGDEBUG, "SendEditingText(%s, %u, %u)", text, location, length);
//  CGUIMessage msg(GUI_MSG_INPUT_TEXT_EDIT, 0, 0, location, length);
//  msg.SetLabel(text);
//  CServiceBroker::GetGUI()->GetWindowManager().SendThreadMessage(msg, CServiceBroker::GetGUI()->GetWindowManager().GetActiveWindowOrDialog());
}

@implementation OSXTextInputResponder

- (id)initWithFrame:(NSRect)frame
{
  self = [super initWithFrame:frame];
  if (self) {
    // Initialization code here.
    _selectedRange = NSMakeRange(0, 0);
    _markedRange = NSMakeRange(NSNotFound, 0);
    _inputRect = NSZeroRect;
  }

  return self;
}

- (void) setInputRect:(NSRect) rect
{
  _inputRect = rect;
}

- (void) insertText:(id) aString replacementRange:(NSRange)replacementRange
{
  const char *str;

  if (replacementRange.location == NSNotFound) {
    if (_markedRange.location != NSNotFound) {
      replacementRange = _markedRange;
    } else {
      replacementRange = _selectedRange;
    }
  }

  /* Could be NSString or NSAttributedString, so we have
   * to test and convert it */
  if ([aString isKindOfClass: [NSAttributedString class]])
    str = [[aString string] UTF8String];
  else
    str = [aString UTF8String];

  SendKeyboardText(str);

  [self unmarkText];
}

- (void) doCommandBySelector:(SEL) myselector
{
  // No need to do anything since we are not using Cocoa
  // selectors to handle special keys, instead we use SDL
  // key events to do the same job.
}

- (BOOL) hasMarkedText
{
  return _markedText != nil;
}

- (NSRange) markedRange
{
  return _markedRange;
}

- (NSRange) selectedRange
{
  return _selectedRange;
}

- (void) setMarkedText:(id) aString
         selectedRange:(NSRange) selRange
      replacementRange:(NSRange) replacementRange
{
  if (replacementRange.location == NSNotFound) {
    if (_markedRange.location != NSNotFound)
      replacementRange = _markedRange;
    else
      replacementRange = _selectedRange;
  }

  if ([aString isKindOfClass: [NSAttributedString class]])
    aString = [aString string];

  if ([aString length] == 0)
  {
    [self unmarkText];
    return;
  }

  _markedText = aString;
  _selectedRange = selRange;
//  _markedRange = NSMakeRange(0, [aString length]);
  _markedRange = NSMakeRange(replacementRange.location, [aString length]);

  SendEditingText([aString UTF8String], selRange.location, selRange.length);
}

- (void) unmarkText
{
  _markedText = nil;
  _markedRange = NSMakeRange(NSNotFound, 0);

  SendEditingText("", 0, 0);
}

- (NSRect) firstRectForCharacterRange: (NSRange) theRange
                          actualRange:(NSRangePointer)actualRange
{
  if (actualRange)
    *actualRange = theRange;

  NSWindow *window = [self window];
  NSRect contentRect = [window contentRectForFrameRect: [window frame]];
  float windowHeight = contentRect.size.height;
  NSRect rect = NSMakeRect(_inputRect.origin.x, windowHeight - _inputRect.origin.y - _inputRect.size.height,
                           _inputRect.size.width, _inputRect.size.height);

//  CLog::Log(LOGDEBUG, "firstRectForCharacterRange: (%lu, %lu): windowHeight = %g, rect = %s",
//            theRange.location, theRange.length, windowHeight,
//            [NSStringFromRect(rect) UTF8String]);
  rect.origin = [[self window] convertRectToScreen:rect].origin;

  return rect;
}

- (NSAttributedString*)attributedSubstringForProposedRange:(NSRange)theRange
                                               actualRange:(NSRangePointer)actualRange
{
  return nil;
}

// This method returns the index for character that is
// nearest to thePoint.  thPoint is in screen coordinate system.
- (NSUInteger) characterIndexForPoint:(NSPoint) thePoint
{
  return 0;
}

// This method is the key to attribute extension.
// We could add new attributes through this method.
// NSInputServer examines the return value of this
// method & constructs appropriate attributed string.
- (NSArray *) validAttributesForMarkedText
{
  return [NSArray array];
}

@end
