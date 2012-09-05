/*****************************************************************
|
|      Neptune - Console Support: Cocoa Implementation
|
|      (c) 2002-2006 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#import <Foundation/Foundation.h>
#include "NptConfig.h"
#include "NptConsole.h"

/*----------------------------------------------------------------------
|       NPT_Console::Output
+---------------------------------------------------------------------*/
void
NPT_Console::Output(const char* message)
{
    NSLog(@"%s", message);
}

