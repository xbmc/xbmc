//
//  XBMCMain.h
//  XBMC
//
//  Created by Elan Feingold on 2/27/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import <Cocoa/Cocoa.h>

@interface XBMCMain : NSObject 
{
  void*               pApplication;
}

+ (XBMCMain *)sharedInstance;
- (void)setApplication:(void*) application;

@end
