//
//  XBMCMain.m
//  XBMC
//
//  Created by Elan Feingold on 2/27/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#import "XBMCMain.h"

@implementation XBMCMain

static XBMCMain *_o_sharedMainInstance = nil;

+ (XBMCMain *)sharedInstance
{
  return _o_sharedMainInstance ? _o_sharedMainInstance : [[self alloc] init];
}

- (id)init
{
  if( _o_sharedMainInstance)
      [self dealloc];
  else
      _o_sharedMainInstance = [super init];
    
  return _o_sharedMainInstance;
}

- (void)setApplication: (void*) application;
{
  pApplication = application;
}

@end
