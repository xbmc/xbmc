/*****************************************************************
|
|   Platinum - Leaks
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltLeaks.h"

#if defined(WIN32)
#include <crtdbg.h>
#include <stdio.h>
#include "string.h"

/*----------------------------------------------------------------------
|   PLT_Leak_AllocHook
+---------------------------------------------------------------------*/
int PLT_Leak_AllocHook(int                  alloc_type, 
                       void*                user_data, 
                       size_t               size, 
                       int                  block_type, 
                       long                 request_number, 
                       const unsigned char* filename, 
                       int                  line_number)
{
    (void)alloc_type;
    (void)user_data;
    (void)size;
    (void)block_type;
    (void)request_number;
    (void)line_number;
    (void)filename;
   /*
    * if (request_number == 34556)
    *   return 2;
    *
    */
    return 1;
}

/*----------------------------------------------------------------------
|   PLT_Leak_Enable
+---------------------------------------------------------------------*/
void
PLT_Leak_Enable(void) 
{
#if defined(_DEBUG)
    /*
    * If you want VC to dump file name and line number of leaking resource
    * use #define _CRTDBG_MAP_ALLOC in suspected file (project)
    * and #include "crtdbg.h" in suspected file
    */
_CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF    |
               _CRTDBG_CHECK_ALWAYS_DF |
               _CRTDBG_LEAK_CHECK_DF);

_CrtSetAllocHook(PLT_Leak_AllocHook );

#endif
}
#else
/*----------------------------------------------------------------------
|   PLT_Leak_Enable
+---------------------------------------------------------------------*/
void
PLT_Leak_Enable(void) 
{
}
#endif
