/*****************************************************************
|
|      Neptune - Directory :: Win32 Implementation
|
|      (c) 2004 Sylvain Rebaud
|      Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
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

/*----------------------------------------------------------------------
|       NPT_Win32DirectoryEntry
+---------------------------------------------------------------------*/
class NPT_Win32DirectoryEntry : public NPT_DirectoryEntryInterface
{
public:
    // methods
    NPT_Win32DirectoryEntry(const char* path);
    virtual ~NPT_Win32DirectoryEntry();

    // NPT_DirectoryInterface methods
    virtual NPT_Result GetInfo(NPT_DirectoryEntryInfo& info);

private:
    // members
    NPT_String m_Path;
};

/*----------------------------------------------------------------------
|       NPT_Win32DirectoryEntry::NPT_Win32DirectoryEntry
+---------------------------------------------------------------------*/
NPT_Win32DirectoryEntry::NPT_Win32DirectoryEntry(const char* path) :
    m_Path(path)
{  
    m_Path.TrimRight(NPT_WIN32_DIR_DELIMITER_CHR);
    m_Path.TrimRight(NPT_UNIX_DIR_DELIMITER_CHR);
}

/*----------------------------------------------------------------------
|       NPT_Win32DirectoryEntry::~NPT_Win32DirectoryEntry
+---------------------------------------------------------------------*/
NPT_Win32DirectoryEntry::~NPT_Win32DirectoryEntry()
{
}

/*----------------------------------------------------------------------
|       NPT_Win32DirectoryEntry::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_Win32DirectoryEntry::GetInfo(NPT_DirectoryEntryInfo& info)
{
    WIN32_FIND_DATA filedata;

    // converts to Utf8?
    HANDLE sizeHandle = FindFirstFile(m_Path, &filedata);
    if (sizeHandle == INVALID_HANDLE_VALUE) {
        DWORD err = GetLastError();
        FindClose(sizeHandle);
        return NPT_FAILURE;
    }

    info.size = filedata.nFileSizeLow;
    info.type = (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? NPT_DIRECTORY_TYPE : NPT_FILE_TYPE;

    FindClose(sizeHandle);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_DirectoryEntry::NPT_DirectoryEntry
+---------------------------------------------------------------------*/
NPT_DirectoryEntry::NPT_DirectoryEntry(const char* path)
{
    m_Delegate = new NPT_Win32DirectoryEntry(path);
}

/*----------------------------------------------------------------------
|       NPT_Win32Directory
+---------------------------------------------------------------------*/
class NPT_Win32Directory : public NPT_DirectoryInterface
{
public:
    // methods
    NPT_Win32Directory(const char* path);
    virtual ~NPT_Win32Directory();

    // NPT_DirectoryInterface methods
    virtual NPT_Result GetNextEntry(NPT_String& name, NPT_DirectoryEntryInfo* info = NULL);

private:
    // members
    NPT_String m_Path;
    HANDLE     m_SearchHandle;
};

/*----------------------------------------------------------------------
|       NPT_Win32Directory::NPT_Win32Directory
+---------------------------------------------------------------------*/
NPT_Win32Directory::NPT_Win32Directory(const char* path) :
    m_Path(path),
    m_SearchHandle(NULL)
{
    if (!m_Path.EndsWith(NPT_UNIX_DIR_DELIMITER_STR)) {
        if (!m_Path.EndsWith(NPT_WIN32_DIR_DELIMITER_STR)) {
            m_Path += NPT_WIN32_DIR_DELIMITER_STR;
        }
    }

    m_Path += "*.*";    
}

/*----------------------------------------------------------------------
|       NPT_Win32Directory::~NPT_Win32Directory
+---------------------------------------------------------------------*/
NPT_Win32Directory::~NPT_Win32Directory()
{
    if (m_SearchHandle != NULL) {
        FindClose(m_SearchHandle);
    }
}

/*----------------------------------------------------------------------
|       NPT_Win32Directory::GetNextEntry
+---------------------------------------------------------------------*/
NPT_Result
NPT_Win32Directory::GetNextEntry(NPT_String& name, NPT_DirectoryEntryInfo* info)
{
    WIN32_FIND_DATA filedata;

    if (m_SearchHandle == NULL) {
        m_SearchHandle = FindFirstFile(m_Path, &filedata);
        if (m_SearchHandle == INVALID_HANDLE_VALUE) {
            m_SearchHandle = NULL;
            return NPT_FAILURE;
        }   
    } else {
        if (FindNextFile(m_SearchHandle, &filedata) == 0) {
            return NPT_FAILURE;
        }
    }

    // need to convert to Utf8 ?
    name = filedata.cFileName;

    if (info) {
        info->size = filedata.nFileSizeLow;
        info->type = (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? NPT_DIRECTORY_TYPE : NPT_FILE_TYPE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_Directory::NPT_Directory
+---------------------------------------------------------------------*/
NPT_Directory::NPT_Directory(const char* path)
{
    m_Delegate = new NPT_Win32Directory(path);
}








