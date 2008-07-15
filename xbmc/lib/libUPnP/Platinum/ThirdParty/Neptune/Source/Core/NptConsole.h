/*****************************************************************
|
|   Neptune - Console
|
|   (c) 2002-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/
/** @file
* Header file for console support
*/

#ifndef _NPT_CONSOLE_H_
#define _NPT_CONSOLE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptResults.h"

/*----------------------------------------------------------------------
|   prototypes
+---------------------------------------------------------------------*/
class NPT_Console {
public:
    // class methods
    static void Output(const char* message);
    static void OutputF(const char* format, ...);
};


#endif /* _NPT_CONSOLE_H_ */
