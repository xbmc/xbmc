/*****************************************************************
|
|      BufferedStreams Test Program 1
|
|      (c) 2001-2005 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include <stdlib.h>

#if defined(_DEBUG) && defined(WIN32)
#include <crtdbg.h>
#endif

/*----------------------------------------------------------------------
|       main
+---------------------------------------------------------------------*/
int
main(int /*argc*/, char** /*argv*/)
{
    // setup debugging
#if defined(_DEBUG) && defined(WIN32)
    int flags = _crtDbgFlag       | 
        _CRTDBG_ALLOC_MEM_DF      |
        _CRTDBG_DELAY_FREE_MEM_DF |
        _CRTDBG_CHECK_ALWAYS_DF;

    _CrtSetDbgFlag(flags);
    //AllocConsole();
    //freopen("CONOUT$", "w", stdout);
#endif 

#if 0
    const char* b0 = "";
    const char* b1 = "\n";
    const char* b2 = "\r";
    const char* b3 = "\r\n";
    const char* b4 = "0\r1\r\r2\r\r\r"       // only \r, up to 3
                     "3\n4\n\n5\n\n\n"       // only \n, up to 3
                     "6\r\n7\n\r"            // one \r and one \n
                     "8\r\n\r9\r\r\na\n\r\r" // two \r and one \n
                     "b\n\r\nc\n\n\rd\r\n\n" // two \n and one \r
                     ;
    const char* b5 = "aaa\r";
    const char* b6 = "aaa\n";
    const char* b7 = "aaa\r\n";

    printf("BufferedInputStream test1 passed\n");
#endif

    return 0;
}
