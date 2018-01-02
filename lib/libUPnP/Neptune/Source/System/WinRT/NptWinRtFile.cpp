/*****************************************************************
|
|   Neptune - Files :: WinRT Implementation
|
|   (c) 2001-2013 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptWinRtPch.h"

#include "NptConfig.h"
#include "NptUtils.h"
#include "NptFile.h"
#include "NptStrings.h"
#include "NptDebug.h"
#include "NptWinRtUtils.h"

/*----------------------------------------------------------------------
|   MapError
+---------------------------------------------------------------------*/
static NPT_Result
MapError(DWORD err) {
    switch (err) {
      case ERROR_ALREADY_EXISTS:      return NPT_ERROR_FILE_ALREADY_EXISTS;
      case ERROR_PATH_NOT_FOUND:    
      case ERROR_FILE_NOT_FOUND:    
      case ERROR_INVALID_DRIVE:
      case ERROR_BAD_PATHNAME:
      case ERROR_BAD_NET_NAME:
      case ERROR_FILENAME_EXCED_RANGE:
      case ERROR_NO_MORE_FILES:
      case ERROR_BAD_NETPATH:         return NPT_ERROR_NO_SUCH_FILE;
      case ERROR_LOCK_VIOLATION:
      case ERROR_SEEK_ON_DEVICE:
      case ERROR_CURRENT_DIRECTORY:
      case ERROR_CANNOT_MAKE:
      case ERROR_FAIL_I24:
      case ERROR_NETWORK_ACCESS_DENIED:
      case ERROR_DRIVE_LOCKED:
      case ERROR_ACCESS_DENIED:       return NPT_ERROR_PERMISSION_DENIED;
      case ERROR_NOT_LOCKED:
      case ERROR_LOCK_FAILED:
      case ERROR_SHARING_VIOLATION:   return NPT_ERROR_FILE_BUSY;
      case ERROR_INVALID_FUNCTION:    return NPT_ERROR_INTERNAL;
      case ERROR_NOT_ENOUGH_QUOTA:    return NPT_ERROR_OUT_OF_MEMORY;
      case ERROR_ARENA_TRASHED:
      case ERROR_NOT_ENOUGH_MEMORY:
      case ERROR_INVALID_BLOCK:       return NPT_ERROR_OUT_OF_MEMORY;
      case ERROR_DISK_FULL:           return NPT_ERROR_FILE_NOT_ENOUGH_SPACE;
      case ERROR_TOO_MANY_OPEN_FILES: return NPT_ERROR_OUT_OF_RESOURCES;
      case ERROR_INVALID_HANDLE:      
      case ERROR_INVALID_ACCESS:
      case ERROR_INVALID_DATA:        return NPT_ERROR_INVALID_PARAMETERS;
      case ERROR_DIR_NOT_EMPTY:       return NPT_ERROR_DIRECTORY_NOT_EMPTY;
      case ERROR_NEGATIVE_SEEK:       return NPT_ERROR_OUT_OF_RANGE;
      default:                        return NPT_FAILURE;
}
}

#include <sys/stat.h>
#include <direct.h>

/*----------------------------------------------------------------------
|   NPT_stat_utf8
+---------------------------------------------------------------------*/
int
NPT_stat_utf8(const char* path, struct __stat64* info)
{
    NPT_WINRT_USE_CHAR_CONVERSION;
    return _wstat64(NPT_WINRT_A2W(path), info);
}

/*----------------------------------------------------------------------
|   NPT_fsopen_utf8
+---------------------------------------------------------------------*/
FILE*
NPT_fsopen_utf8(const char* path, const char* mode, int sh_flags)
{
    NPT_WINRT_USE_CHAR_CONVERSION;
    return _wfsopen(NPT_WINRT_A2W(path), NPT_WINRT_A2W(mode), sh_flags);
}

/*----------------------------------------------------------------------
|   NPT_FilePath::Separator
+---------------------------------------------------------------------*/
const char* const NPT_FilePath::Separator = "\\";

/*----------------------------------------------------------------------
|   NPT_File::GetRoots
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetRoots(NPT_List<NPT_String>& roots)
{
    roots.Clear();
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_File::GetWorkingDir
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetWorkingDir(NPT_String& path)
{
    path.SetLength(0);
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_File::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetInfo(const char* path, NPT_FileInfo* info)
{
	DWORD           attributes = 0;
	WIN32_FILE_ATTRIBUTE_DATA attribute_data;

	NPT_WINRT_USE_CHAR_CONVERSION; 

	if (0 == GetFileAttributesEx(
		NPT_WINRT_A2W(path), 
		GetFileExInfoStandard, 
		&attribute_data)) {
		DWORD error_code = GetLastError();
        return NPT_FAILURE;
    }
	attributes = attribute_data.dwFileAttributes;
	if (attributes == INVALID_FILE_ATTRIBUTES) {
        return NPT_FAILURE;
	}

	if (info != NULL) {
		// (prasad) FIXME - fill in the attribute values in return value
	}

	return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_File::CreateDir
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::CreateDir(const char* path)
{
    NPT_WINRT_USE_CHAR_CONVERSION;
    BOOL result = ::CreateDirectoryW(NPT_WINRT_A2W(path), NULL);
    if (result == 0) {
        return MapError(GetLastError());
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_File::RemoveFile
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::RemoveFile(const char* path)
{
    NPT_WINRT_USE_CHAR_CONVERSION;
    BOOL result = ::DeleteFileW(NPT_WINRT_A2W(path));
    if (result == 0) {
        return MapError(GetLastError());
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_File::RemoveDir
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::RemoveDir(const char* path)
{
    NPT_WINRT_USE_CHAR_CONVERSION;
    BOOL result = ::RemoveDirectoryW(NPT_WINRT_A2W(path));
    if (result == 0) {
        return MapError(GetLastError());
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_File::Rename
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Rename(const char* from_path, const char* to_path)
{
    NPT_WINRT_USE_CHAR_CONVERSION;
	BOOL result = MoveFileEx(NPT_WINRT_A2W(from_path), 
		                     NPT_WINRT_A2W(to_path), 
							 MOVEFILE_COPY_ALLOWED | MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH);
    if (result == 0) {
        return MapError(GetLastError());
    }
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_File_ProcessFindData
+---------------------------------------------------------------------*/
static bool
NPT_File_ProcessFindData(WIN32_FIND_DATAW* find_data)
{
    NPT_WINRT_USE_CHAR_CONVERSION;

    // discard system specific files/shortcuts
    if (NPT_StringsEqual(NPT_WINRT_W2A(find_data->cFileName), ".") || 
        NPT_StringsEqual(NPT_WINRT_W2A(find_data->cFileName), "..")) {
        return false;
    }

    return true;
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
    NPT_WINRT_USE_CHAR_CONVERSION;

    // default return value
    entries.Clear();

    // check the arguments
    if (path == NULL || path[0] == '\0') return NPT_ERROR_INVALID_PARAMETERS;

    // construct a path name with a \* wildcard at the end
    NPT_String path_pattern = path;
    if (path_pattern.EndsWith("\\") || path_pattern.EndsWith("/")) {
        path_pattern += "*";
    } else {
        path_pattern += "\\*";
    }

    // list the entries
    WIN32_FIND_DATAW find_data;
	HANDLE find_handle = FindFirstFileEx(NPT_WINRT_A2W(path_pattern.GetChars()), 
			                             FindExInfoStandard, 
									     &find_data, 
									     FindExSearchNameMatch,
									     NULL,
									     0);
    if (find_handle == INVALID_HANDLE_VALUE) return MapError(GetLastError());
    NPT_Cardinal count = 0;
    do {
        if (NPT_File_ProcessFindData(&find_data)) {
            // continue if we still have entries to skip
            if (start > 0) {
                --start;
                continue;
            }
            entries.Add(NPT_WINRT_W2A(find_data.cFileName));

            // stop when we have reached the maximum requested
            if (max && ++count == max) return NPT_SUCCESS;
        }
    } while (FindNextFileW(find_handle, &find_data));
    DWORD last_error = GetLastError();
    FindClose(find_handle);
    if (last_error != ERROR_NO_MORE_FILES) return MapError(last_error);

    return NPT_SUCCESS;
}
