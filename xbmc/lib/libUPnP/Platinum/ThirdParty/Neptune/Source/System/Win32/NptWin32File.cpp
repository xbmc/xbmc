/*****************************************************************
|
|      Neptune - File :: Win32 Implementation
|
|      (c) 2001-2008 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptLogging.h"
#include "NptFile.h"
#include "NptUtils.h"
#include "NptWin32Utils.h"

#if defined(_XBOX)
#include <xtl.h>
#else
#include <windows.h>
#endif

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
//NPT_SET_LOCAL_LOGGER("neptune.win32.file")

/*----------------------------------------------------------------------
|   MapError
+---------------------------------------------------------------------*/
static NPT_Result
MapError(DWORD err) {
    switch (err) {
      case ERROR_ALREADY_EXISTS:    return NPT_ERROR_FILE_ALREADY_EXISTS;
      case ERROR_PATH_NOT_FOUND:    return NPT_ERROR_NO_SUCH_FILE;
      case ERROR_FILE_NOT_FOUND:    return NPT_ERROR_NO_SUCH_FILE;
      case ERROR_ACCESS_DENIED:     return NPT_ERROR_PERMISSION_DENIED;
      case ERROR_SHARING_VIOLATION: return NPT_ERROR_FILE_BUSY;
      default:                      return NPT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   NPT_FilePath::Separator
+---------------------------------------------------------------------*/
const NPT_String NPT_FilePath::Separator("\\");

/*----------------------------------------------------------------------
|   NPT_File::GetRoots
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetRoots(NPT_List<NPT_String>& roots)
{
    roots.Clear();
#if defined(_WIN32_WCE) || defined(_XBOX)
    return NPT_ERROR_NOT_IMPLEMENTED;
#else
    DWORD drives = GetLogicalDrives();
    for (unsigned int i=0; i<26; i++) {
        if (drives & (1<<i)) {
            char drive_name[4] = {'A'+i, ':', '\\', 0};
            roots.Add(drive_name);
        }
    }
    return NPT_SUCCESS;
#endif
}

#if defined(_WIN32_WCE)
/*----------------------------------------------------------------------
|   NPT_File::CreateDir
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::CreateDir(const char* path)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_File::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetInfo(const char* path, NPT_FileInfo* info)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_File::Delete
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Delete(const char* path)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_File::DeleteDirectory
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::DeleteDirectory(const char* path)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_File::Rename
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Rename(const char* from_path, const char* to_path)
{
    return NPT_ERROR_NOT_IMPLEMENTED;
}
#endif

#if defined(_WIN32_WCE) || defined(_XBOX)
/*----------------------------------------------------------------------
|   NPT_File::GetWorkingDirectory
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetWorkingDirectory(NPT_String& path)
{
    path.SetLength(0);
    return NPT_ERROR_NOT_IMPLEMENTED;
}
#endif

/*----------------------------------------------------------------------
|   NPT_File_ProcessFindData
+---------------------------------------------------------------------*/
static void
NPT_File_ProcessFindData(NPT_WIN32_FIND_DATA* find_data, NPT_List<NPT_String>& entries)
{
    NPT_WIN32_USE_CHAR_CONVERSION;
    entries.Add(NPT_WIN32_W2A(find_data->cFileName));
}

/*----------------------------------------------------------------------
|   NPT_File::ListDirectory
+---------------------------------------------------------------------*/
NPT_Result 
NPT_File::ListDirectory(const char* path, NPT_List<NPT_String>& entries)
{
    NPT_WIN32_USE_CHAR_CONVERSION;

    // default return value
    entries.Clear();

    // check the arguments
    if (path == NULL) return NPT_ERROR_INVALID_PARAMETERS;

    // construct a path name with a \* wildcard at the end
    NPT_String path_pattern = path;
    if (path_pattern.EndsWith("\\") || path_pattern.EndsWith("/")) {
        path_pattern += "*";
    } else {
        path_pattern += "\\*";
    }

    // list the entries
    NPT_WIN32_FIND_DATA find_data;
    HANDLE find_handle = NPT_FindFirstFile(NPT_WIN32_A2W(path_pattern.GetChars()), &find_data);
    if (find_handle == INVALID_HANDLE_VALUE) return MapError(GetLastError());
    NPT_File_ProcessFindData(&find_data, entries);
    while (NPT_FindNextFile(find_handle, &find_data)) {
        NPT_File_ProcessFindData(&find_data, entries);
    }
    DWORD last_error = GetLastError();
    NPT_FindClose(find_handle);
    if (last_error != ERROR_NO_MORE_FILES) return MapError(last_error);

    return NPT_SUCCESS;
}
