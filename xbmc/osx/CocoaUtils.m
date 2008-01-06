//
//  CocoaUtils.m
//  XBMC
//
//  Created by Elan Feingold on 1/5/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//
#import <Cocoa/Cocoa.h>

void* InitializeAutoReleasePool()
{
	NSAutoreleasePool* pool = [[NSAutoreleasePool alloc] init];
	return pool;
}

void DestroyAutoReleasePool(void* aPool)
{
	NSAutoreleasePool* pool = (NSAutoreleasePool* )aPool;
	[pool release];
}