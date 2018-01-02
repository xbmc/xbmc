/*****************************************************************
|
|      Neptune - Dynamic Libraries :: Null/Stub Implementation
|
|      (c) 2001-2016 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptLogging.h"
#include "NptDynamicLibraries.h"

/*----------------------------------------------------------------------
|   NPT_DynamicLibrary::Load
+---------------------------------------------------------------------*/
NPT_Result
NPT_DynamicLibrary::Load(const char* /* name */, NPT_Flags /* flags */, NPT_DynamicLibrary*& /* library */)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}
