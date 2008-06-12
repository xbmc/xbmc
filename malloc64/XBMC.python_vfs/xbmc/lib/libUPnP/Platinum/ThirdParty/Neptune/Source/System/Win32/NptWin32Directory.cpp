/*****************************************************************
|
|      Neptune - Directory :: Win32 Implementation
|
|      (c) 2004 Sylvain Rebaud
|      Author: Sylvain Rebaud (sylvain@rebaud.com)
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

#include "NptConfig.h"
#include "NptTypes.h"
#include "NptDirectory.h"
#include "NptDebug.h"
#include "NptResults.h"
#include "NptUtils.h"

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
    virtual NPT_Result GetInfo(NPT_DirectoryEntryInfo& info);

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
NPT_Win32DirectoryEntry::GetInfo(NPT_DirectoryEntryInfo& info)
{   
    // FindFirstFile doesn't work for root directories such as C: 
    if (m_Path.GetLength() == 2 && m_Path[1] == ':') {
        // Make sure there's always a trailing delimiter for root directories
        DWORD attributes = GetFileAttributes(m_Path + NPT_DIR_DELIMITER_CHR);
        if (attributes == -1) return NPT_ERROR_NO_SUCH_ITEM;

        NPT_ASSERT(attributes & FILE_ATTRIBUTE_DIRECTORY);

        info.size = 0;
        info.type = NPT_DIRECTORY_TYPE;
    } else {
        WIN32_FIND_DATA filedata;
        HANDLE sizeHandle = FindFirstFile(m_Path, &filedata);
        if (sizeHandle == INVALID_HANDLE_VALUE) {
            FindClose(sizeHandle);
            return NPT_ERROR_NO_SUCH_ITEM;
        }

        info.size = (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 0 : filedata.nFileSizeLow;
        info.type = (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? NPT_DIRECTORY_TYPE : NPT_FILE_TYPE;

        FindClose(sizeHandle);
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
    virtual NPT_Result GetInfo(NPT_DirectoryInfo& info);
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
        FindClose(m_SearchHandle);
    }
}

/*----------------------------------------------------------------------
|   NPT_Win32Directory::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_Win32Directory::GetInfo(NPT_DirectoryInfo& info)
{
    if (!m_Validated) {
        NPT_CHECK(NPT_Directory::GetEntryCount(m_Path, m_Count));
        m_Validated = true;
    }

    info.entry_count = m_Count;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_Win32Directory::GetNextEntry
+---------------------------------------------------------------------*/
NPT_Result
NPT_Win32Directory::GetNextEntry(NPT_String& name, NPT_DirectoryEntryInfo* info)
{
    WIN32_FIND_DATA filedata;

    // reset output params first
    name = "";

    if (m_SearchHandle == NULL) {
        NPT_String root_path = m_Path;
        NPT_DirectoryAppendToPath(root_path, "*");

        m_SearchHandle = FindFirstFile(root_path, &filedata);
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
        if (FindNextFile(m_SearchHandle, &filedata) == 0) {
            // no more entries?
            if (GetLastError() == ERROR_NO_MORE_FILES) 
                return NPT_ERROR_NO_SUCH_ITEM;

            return NPT_FAILURE;
        }
    }

    // discard system specific files/shortcuts
    if (NPT_StringsEqual(filedata.cFileName, ".") || NPT_StringsEqual(filedata.cFileName, "..")) {
        return GetNextEntry(name, info);
    }

    // assign output params
    name = filedata.cFileName;
    if (info) {
        info->size = (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? 0 : filedata.nFileSizeLow;
        info->type = (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? NPT_DIRECTORY_TYPE : NPT_FILE_TYPE;
    }

    return NPT_SUCCESS;
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
    WIN32_FIND_DATA filedata;
    HANDLE          handle;
    NPT_Result      res = NPT_SUCCESS;
    NPT_String      root_path = path;

    // reset output params first
    count = 0;    

    // replace delimiters with the proper one for the platform
    root_path.Replace((NPT_DIR_DELIMITER_CHR == '/')?'\\':'/', NPT_DIR_DELIMITER_CHR);
    // remove trailing slashes
    root_path.TrimRight(NPT_DIR_DELIMITER_CHR);

    // verify it's a directory and not a file
    NPT_DirectoryEntryInfo info;
    NPT_CHECK(NPT_DirectoryEntry::GetInfo(root_path, info));
    if (info.type == NPT_FILE_TYPE) return NPT_ERROR_INVALID_PARAMETERS;

    // start enumerating files
    NPT_DirectoryAppendToPath(root_path, "*");
    handle = FindFirstFile(root_path, &filedata);
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
        if (strcmp(filedata.cFileName, ".") && strcmp(filedata.cFileName, "..")) 
            ++count;
    } while (FindNextFile(handle, &filedata));

    if (GetLastError() != ERROR_NO_MORE_FILES) {
        res = NPT_FAILURE;
    }

    FindClose(handle);
    return res;
}

/*----------------------------------------------------------------------
|   NPT_Directory::Create
+---------------------------------------------------------------------*/
NPT_Result 
NPT_Directory::Create(const char* path)
{
    // check if path exists, if so no need to create it
    NPT_DirectoryInfo info;
    if (NPT_FAILED(NPT_Directory::GetInfo(path, info))) {
        return CreateDirectory(path, NULL)?NPT_SUCCESS:NPT_FAILURE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_Directory::Remove
+---------------------------------------------------------------------*/
NPT_Result 
NPT_Directory::Remove(const char* path)
{
    NPT_Result             res;
    NPT_DirectoryEntryInfo info;

    // make sure the path exists
    res = NPT_DirectoryEntry::GetInfo(path, info);
    if (NPT_SUCCEEDED(res)) {
        // delete path 
        if (info.type == NPT_DIRECTORY_TYPE) {
            res = RemoveDirectory(path)?NPT_SUCCESS:NPT_FAILURE;
        } else {
            res = DeleteFile(path)?NPT_SUCCESS:NPT_FAILURE;
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
    NPT_Result             res;
    NPT_DirectoryEntryInfo info;

    // make sure the path exists
    res = NPT_DirectoryEntry::GetInfo(input, info);
    if (NPT_SUCCEEDED(res)) {
        res = MoveFile(input, output)?NPT_SUCCESS:NPT_FAILURE;
        if (NPT_FAILED(res)) {
            int err = GetLastError();
            NPT_Debug("NPT_Directory::Move - Win32 Error=%d, Input = '%s', Output = '%s'", 
                err, input, output);
        }
    }

    return res;
}
