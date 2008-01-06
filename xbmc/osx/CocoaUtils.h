//
//  Utils.h
//  XBMC
//
//  Created by Elan Feingold on 1/5/2008.
//  Copyright 2008 __MyCompanyName__. All rights reserved.
//

#ifndef _OSX_UTILS_H_
#define _OSX_UTILS_H_

extern "C" {

void* InitializeAutoReleasePool();
void DestroyAutoReleasePool(void* pool);

}

#endif
