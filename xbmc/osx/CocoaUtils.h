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

//
// Pools.
//
void* InitializeAutoReleasePool();
void DestroyAutoReleasePool(void* pool);

//
// Open GL.
//
void  Cocoa_GL_MakeCurrentContext(void* theContext);
void  Cocoa_GL_ReleaseContext(void* context);
void  Cocoa_GL_SwapBuffers(void* theContext);

}

#endif
