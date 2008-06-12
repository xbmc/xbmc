/*
 *  CocoaToCppThunk.h
 *  XBMC
 *
 *  Created by Elan Feingold on 2/27/2008.
 *  Copyright 2008 __MyCompanyName__. All rights reserved.
 *
 */
#ifdef __cplusplus
extern "C" 
{
#endif

void Cocoa_OnAppleRemoteKey(void* application, AppleRemoteEventIdentifier event, bool pressedDown, unsigned int count);

#ifdef __cplusplus
}
#endif