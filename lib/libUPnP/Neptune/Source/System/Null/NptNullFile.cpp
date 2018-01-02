/*****************************************************************
|
|      Neptune - File :: Null/Stub Implementation
|
|      (c) 2001-2016 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptFile.h"
#include "NptUtils.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const char* const NPT_FilePath::Separator = "/";

/*----------------------------------------------------------------------
|   NPT_File::GetRoots
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetRoots(NPT_List<NPT_String>& roots)
{
    roots.Clear();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_File::CreateDir
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::CreateDir(const char* /* path */)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_File::RemoveFile
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::RemoveFile(const char* /* path */)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_File::RemoveDir
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::RemoveDir(const char* /* path */)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_File::Rename
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Rename(const char* /* from_path */, const char* /* to_path */)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_File::ListDir
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::ListDir(const char*           path,
                  NPT_List<NPT_String>& entries,
                  NPT_Ordinal           start /* = 0 */,
                  NPT_Cardinal          max   /* = 0 */)
{
    // default return value
    entries.Clear();

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_File::GetWorkingDir
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetWorkingDir(NPT_String& path)
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_File::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetInfo(const char* path, NPT_FileInfo* info)
{
    // default value
    if (info) NPT_SetMemory(info, 0, sizeof(*info));

    return NPT_SUCCESS;
}
