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

#import <UIKit/UIKit.h>
#include "platform/darwin/tvos/MainKeyboard.h"

@interface KeyboardView : UIView <UITextFieldDelegate>
{
  NSMutableString *_text;
  BOOL _confirmed;
  CMainKeyboard *_tvosKeyboard;
  bool *_canceled;
  BOOL _deactivated;
  UITextField *_textField;
  UITextField *_heading;
  CGRect _kbRect;
  CGRect _frame;
}

@property (nonatomic, retain) NSMutableString *_text;
@property (getter = isConfirmed) BOOL _confirmed;
@property (assign, setter = registerKeyboard:) CMainKeyboard *_tvosKeyboard;
@property CGRect _frame;

- (void) setHeading:(NSString *)heading;
- (void) setHidden:(BOOL)hidden;
- (void) activate;
- (void) deactivate;
- (void) setKeyboardText:(NSString*)aText closeKeyboard:(BOOL)closeKeyboard;
- (void) textChanged:(NSNotification*)aNotification;
- (void) setCancelFlag:(bool *)cancelFlag;
- (void) doDeactivate:(NSDictionary *)dict;
- (id)initWithFrameInternal;

@end
