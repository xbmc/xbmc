/*****************************************************************
|
|      Neptune - Directory :: Win32 Implementation
|
|      Copyright (c) 2004-2008, Plutinosoft, LLC.
|      Author: Sylvain Rebaud (sylvain@plutinosoft.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#if defined(_XBOX)
#include <xtl.h>
#else
#include <windows.h>
#endif

#include <sys/stat.h>
#include <errno.h>

#if defined(_WIN32)
#include <direct.h>
#include <stdlib.h>
#include <stdio.h>
#else
#include <unistd.h>
#include <dirent.h>
#endif

#include "NptConfig.h"
#include "NptTypes.h"
#include "NptDirectory.h"
#include "NptDebug.h"
#include "NptResults.h"
#include "NptUtils.h"

/*----------------------------------------------------------------------
|   Win32 adaptation
+---------------------------------------------------------------------*/
#if defined(_WIN32)
#include "NptWin32Utils.h"
#define mkdir(_path,_mode) _mkdir(_path)
#define getcwd _getcwd
#define unlink _unlink
#define rmdir  _rmdir
#define S_ISDIR(_m) (((_m)&_S_IFMT) == _S_IFDIR) 
#define S_ISREG(_m) (((_m)&_S_IFMT) == _S_IFREG) 
#define S_IWUSR _S_IWRITE
#endif

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const char* const NPT_DIR_DELIMITER_STR = "\\";
const char        NPT_DIR_DELIMITER_CHR = '\\'; 

/*----------------------------------------------------------------------
|   NPT_Win32DirectoryEntry
+---------------------------------------------------------------------*/
class NPT_Win32DirectoryEntry : public NPT_DirectoryEntryInterface
{
public:
    // methods
    NPT_Win32DirectoryEntry(const char* path);
    virtual ~NPT_Win32DirectoryEntry();

    // NPT_DirectoryEntryInterface methods
    virtual NPT_Result GetInfo(NPT_DirectoryEntryInfo* info = NULL);

private:
    // members
    NPT_String m_Path;
};

/*----------------------------------------------------------------------
|   NPT_Win32DirectoryEntry::NPT_Win32DirectoryEntry
+---------------------------------------------------------------------*/
NPT_Win32DirectoryEntry::NPT_Win32DirectoryEntry(const char* path) :
    m_Path(path)
{  
    // replace delimiters with the proper one for the platform
    m_Path.Replace((NPT_DIR_DELIMITER_CHR == '/')?'\\':'/', NPT_DIR_DELIMITER_CHR);

    // remove trailing slashes
    m_Path.TrimRight(NPT_DIR_DELIMITER_CHR);
}

/*----------------------------------------------------------------------
|   NPT_Win32DirectoryEntry::~NPT_Win32DirectoryEntry
+---------------------------------------------------------------------*/
NPT_Win32DirectoryEntry::~NPT_Win32DirectoryEntry()
{
}

/*----------------------------------------------------------------------
|   NPT_Win32DirectoryEntry::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_Win32DirectoryEntry::GetInfo(NPT_DirectoryEntryInfo* info /* = NULL */)
{   
    NPT_WIN32_USE_CHAR_CONVERSION;

    // FindFirstFile doesn't work for root directories such as C: 
    if (m_Path.GetLength() == 2 && m_Path[1] == ':') {
        // Make sure there's always a trailing delimiter for root directories
        DWORD attributes = NPT_GetFileAttributes(NPT_WIN32_A2W(m_Path + NPT_DIR_DELIMITER_CHR));
        if (attributes == -1) return NPT_ERROR_NO_SUCH_ITEM;

        NPT_ASSERT(attributes & FILE_ATTRIBUTE_DIRECTORY);
        if (info) {
            info->size = 0;
            info->type = NPT_DIRECTORY_TYPE;
        }
    } else {
        // get the file info
        NPT_stat_struct statbuf;
        int result = NPT_stat(NPT_WIN32_A2W(m_Path), &statbuf);
        if (result != 0) return NPT_ERROR_NO_SUCH_ITEM;

        if (!S_ISDIR(statbuf.st_mode) && !S_ISREG(statbuf.st_mode))
            return NPT_FAILURE;

        if (info) {
            info->size     = (NPT_Size)(S_ISDIR(statbuf.st_mode) ? 0 : statbuf.st_size);
            info->type     = S_ISDIR(statbuf.st_mode) ? NPT_DIRECTORY_TYPE : NPT_FILE_TYPE;
            info->created  = (unsigned long)statbuf.st_ctime;
            info->modified = (unsigned long)statbuf.st_mtime;
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_DirectoryEntry::NPT_DirectoryEntry
+---------------------------------------------------------------------*/
NPT_DirectoryEntry::NPT_DirectoryEntry(const char* path)
{
    m_Delegate = new NPT_Win32DirectoryEntry(path);
}

/*----------------------------------------------------------------------
|   NPT_Win32Directory
+---------------------------------------------------------------------*/
class NPT_Win32Directory : public NPT_DirectoryInterface
{
public:
    // methods
    NPT_Win32Directory(const char* path);
    virtual ~NPT_Win32Directory();

    // NPT_DirectoryInterface methods
    virtual NPT_Result GetInfo(NPT_DirectoryInfo* info = NULL);
    virtual NPT_Result GetNextEntry(NPT_String& name, NPT_DirectoryEntryInfo* info = NULL);

private:
    // members
    NPT_String   m_Path;
    HANDLE       m_SearchHandle;
    NPT_Cardinal m_Count;
    bool         m_Validated;
};

/*----------------------------------------------------------------------
|   NPT_Win32Directory::NPT_Win32Directory
+---------------------------------------------------------------------*/
NPT_Win32Directory::NPT_Win32Directory(const char* path) :
    m_Path(path),
    m_SearchHandle(NULL),
    m_Count(0),
    m_Validated(false)
{
    // replace delimiters with the proper one for the platform
    m_Path.Replace((NPT_DIR_DELIMITER_CHR == '/')?'\\':'/', NPT_DIR_DELIMITER_CHR);

    // remove trailing slashes
    m_Path.TrimRight(NPT_DIR_DELIMITER_CHR);
}

/*----------------------------------------------------------------------
|   NPT_Win32Directory::~NPT_Win32Directory
+---------------------------------------------------------------------*/
NPT_Win32Directory::~NPT_Win32Directory()
{
    if (m_SearchHandle != NULL) {
        NPT_FindClose(m_SearchHandle);
    }
}

/*----------------------------------------------------------------------
|   NPT_Win32Directory::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_Win32Directory::GetInfo(NPT_DirectoryInfo* info)
{
    if (!m_Validated) {
        NPT_CHECK(NPT_Directory::GetEntryCount(m_Path, m_Count));
        m_Validated = true;
    }

    if (info) info->entry_count = m_Count;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_Win32Directory::GetNextEntry
+---------------------------------------------------------------------*/
NPT_Result
NPT_Win32Directory::GetNextEntry(NPT_String& name, NPT_DirectoryEntryInfo* info)
{
    NPT_WIN32_USE_CHAR_CONVERSION;
    NPT_WIN32_FIND_DATA filedata;

    // reset output params first
    name = "";

    if (m_SearchHandle == NULL) {
        NPT_String root_path = m_Path;
        NPT_DirectoryAppendToPath(root_path, "*");

        m_SearchHandle = NPT_FindFirstFile(NPT_WIN32_A2W(root_path), &filedata);
        if (m_SearchHandle == INVALID_HANDLE_VALUE) {
            m_SearchHandle = NULL;
            switch (GetLastError()) {
                case ERROR_FILE_NOT_FOUND:
                case ERROR_PATH_NOT_FOUND:
                case ERROR_NO_MORE_FILES:
                    return NPT_ERROR_NO_SUCH_ITEM;

                default:
                    return NPT_FAILURE;
            }
        } 
    } else {
        if (NPT_FindNextFile(m_SearchHandle, &filedata) == 0) {
            // no more entries?
            if (GetLastError() == ERROR_NO_MORE_FILES) 
                return NPT_ERROR_NO_SUCH_ITEM;

            return NPT_FAILURE;
        }
    }

    // discard system specific files/shortcuts
    if (NPT_StringsEqual(NPT_WIN32_W2A(filedata.cFileName), ".") || 
        NPT_StringsEqual(NPT_WIN32_W2A(filedata.cFileName), "..")) {
        return GetNextEntry(name, info);
    }

    // assign output params
    name = NPT_WIN32_W2A(filedata.cFileName);
    if (!info) return NPT_SUCCESS;
        
    return NPT_DirectoryEntry::GetInfo(m_Path + NPT_DIR_DELIMITER_STR + name, info);
}

/*----------------------------------------------------------------------
|   NPT_Directory::NPT_Directory
+---------------------------------------------------------------------*/
NPT_Directory::NPT_Directory(const char* path)
{
    m_Delegate = new NPT_Win32Directory(path);
}

/*----------------------------------------------------------------------
|   NPT_Directory::GetEntryCount
+---------------------------------------------------------------------*/
NPT_Result 
NPT_Directory::GetEntryCount(const char* path, NPT_Cardinal& count)
{
    NPT_WIN32_USE_CHAR_CONVERSION;
    NPT_WIN32_FIND_DATA filedata;
    HANDLE              handle;
    NPT_Result          res = NPT_SUCCESS;
    NPT_String          root_path = path;

    // reset output params first
    count = 0;    

    // replace delimiters with the proper one for the platform
    root_path.Replace((NPT_DIR_DELIMITER_CHR == '/')?'\\':'/', NPT_DIR_DELIMITER_CHR);
    // remove trailing slashes
    root_path.TrimRight(NPT_DIR_DELIMITER_CHR);

    // verify it's a directory and not a file
    NPT_DirectoryEntryInfo info;
    NPT_CHECK(NPT_DirectoryEntry::GetInfo(root_path, &info));
    if (info.type == NPT_FILE_TYPE) return NPT_ERROR_INVALID_PARAMETERS;

    // start enumerating files
    NPT_DirectoryAppendToPath(root_path, "*");
    handle = NPT_FindFirstFile(NPT_WIN32_A2W(root_path), &filedata);
    if (handle == INVALID_HANDLE_VALUE) {
        switch (GetLastError()) {
            case ERROR_FILE_NOT_FOUND:
            case ERROR_PATH_NOT_FOUND:
            case ERROR_NO_MORE_FILES:
                return NPT_SUCCESS;

            default:
                return NPT_FAILURE;
        }
    }

    // count and disregard system specific files
    do {
        // discard system specific files/shortcuts
        if (!NPT_StringsEqual(NPT_WIN32_W2A(filedata.cFileName), ".") || 
            !NPT_StringsEqual(NPT_WIN32_W2A(filedata.cFileName), ".."))
            ++count;
    } while (NPT_FindNextFile(handle, &filedata));

    if (GetLastError() != ERROR_NO_MORE_FILES) {
        res = NPT_FAILURE;
    }

    NPT_FindClose(handle);
    return res;
}

/*----------------------------------------------------------------------
|   NPT_Directory::Create
+---------------------------------------------------------------------*/
NPT_Result 
NPT_Directory::Create(const char* path)
{
    NPT_WIN32_USE_CHAR_CONVERSION;

    // check if path exists, if so no need to create it
    NPT_DirectoryInfo info;
    if (NPT_FAILED(NPT_Directory::GetInfo(path, &info))) {
        return NPT_CreateDirectory(NPT_WIN32_A2W(path), NULL)?NPT_SUCCESS:NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_Directory::Remove
+---------------------------------------------------------------------*/
NPT_Result 
NPT_Directory::Remove(const char* path)
{
    NPT_WIN32_USE_CHAR_CONVERSION;
    NPT_Result             res;
    NPT_DirectoryEntryInfo info;

    // make sure the path exists
    res = NPT_DirectoryEntry::GetInfo(path, &info);
    if (NPT_SUCCEEDED(res)) {
        // delete path 
        if (info.type == NPT_DIRECTORY_TYPE) {
            res = NPT_RemoveDirectory(NPT_WIN32_A2W(path))?NPT_SUCCESS:NPT_FAILURE;
        } else {
            res = NPT_DeleteFile(NPT_WIN32_A2W(path))?NPT_SUCCESS:NPT_FAILURE;
        }

        if (NPT_FAILED(res)) {
            NPT_Debug("NPT_Directory::Remove - Win32 Error=%d, Dir = '%s'", 
                GetLastError(), path);
        }
    }

    return res;
}

/*----------------------------------------------------------------------
|   NPT_Directory::Move
+---------------------------------------------------------------------*/
NPT_Result 
NPT_Directory::Move(const char* input, const char* output)
{
    NPT_WIN32_USE_CHAR_CONVERSION;
    // make sure the path exists
    NPT_Result res = NPT_DirectoryEntry::GetInfo(input);
    if (NPT_SUCCEEDED(res)) {
        res = NPT_MoveFile(NPT_WIN32_A2W(input), NPT_WIN32_A2W(output))?NPT_SUCCESS:NPT_FAILURE;
        if (NPT_FAILED(res)) {
            int err = GetLastError();
            NPT_Debug("NPT_Directory::Move - Win32 Error=%d, Input = '%s', Output = '%s'", 
                err, input, output);
        }
    }

    return res;
}
