/*****************************************************************
|
|      Neptune - Directory :: PSP Implementation
|
|      Copyright (c) 2004-2008, Plutinosoft, LLC.
|      Author: Sylvain Rebaud (sylvain@plutinosoft.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <stdio.h>

#include <kernel.h>
#include <psptypes.h>
#include <psperror.h>

#include "NptConfig.h"
#include "NptTypes.h"
#include "NptDirectory.h"
#include "NptDebug.h"
#include "NptResults.h"

/*----------------------------------------------------------------------
|       NPT_PSPDirectoryEntry
+---------------------------------------------------------------------*/
class NPT_PSPDirectoryEntry : public NPT_DirectoryEntryInterface
{
public:
    // methods
    NPT_PSPDirectoryEntry(const char* path);
    virtual ~NPT_PSPDirectoryEntry();

    // NPT_DirectoryInterface methods
    virtual NPT_Result GetType(NPT_DirectoryEntryType& type);
    virtual NPT_Result GetSize(NPT_Size& size);

private:
    // members
    NPT_StringObject m_Path;
};

/*----------------------------------------------------------------------
|       NPT_PSPDirectoryEntry::NPT_PSPDirectoryEntry
+---------------------------------------------------------------------*/
NPT_PSPDirectoryEntry::NPT_PSPDirectoryEntry(const char* path) :
    m_Path(path)
{  
}

/*----------------------------------------------------------------------
|       NPT_PSPDirectoryEntry::~NPT_PSPDirectoryEntry
+---------------------------------------------------------------------*/
NPT_PSPDirectoryEntry::~NPT_PSPDirectoryEntry()
{
}

/*----------------------------------------------------------------------
|       NPT_PSPDirectoryEntry::GetType
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPDirectoryEntry::GetType(NPT_DirectoryEntryType& type)
{
    //// converts from Utf8 ?
    //DWORD fa = GetFileAttributes(m_Path);
    //if (fa == 0xFFFFFFFF) {
    //    return NPT_FAILURE;
    //}

    //if ((fa & FILE_ATTRIBUTE_DIRECTORY) == 0) {
    //    type = NPT_FILE_TYPE;
    //} else {
    //    type = NPT_DIRECTORY_TYPE;
    //}

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPDirectoryEntry::GetSize
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPDirectoryEntry::GetSize(NPT_Size& size)
{
    //WIN32_FIND_DATA filedata;

    //// converts to Utf8?
    //HANDLE sizeHandle = FindFirstFile(m_Path, &filedata);
    //if (sizeHandle == INVALID_HANDLE_VALUE) {
    //    FindClose(sizeHandle);
    //    return NPT_FAILURE;
    //}

    //size = filedata.nFileSizeLow;
    //FindClose(sizeHandle);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_DirectoryEntry::NPT_DirectoryEntry
+---------------------------------------------------------------------*/
NPT_DirectoryEntry::NPT_DirectoryEntry(const char* path)
{
    m_Delegate = new NPT_PSPDirectoryEntry(path);
}

/*----------------------------------------------------------------------
|       NPT_PSPDirectory
+---------------------------------------------------------------------*/
class NPT_PSPDirectory : public NPT_DirectoryInterface
{
public:
    // methods
    NPT_PSPDirectory(const char* path);
    virtual ~NPT_PSPDirectory();

    // NPT_DirectoryInterface methods
    virtual NPT_Result GetNextEntry(NPT_StringObject& name, NPT_Size* size = NULL, NPT_DirectoryEntryType* type = NULL);

private:
    // members
    NPT_StringObject m_Path;
};

/*----------------------------------------------------------------------
|       NPT_PSPDirectory::NPT_PSPDirectory
+---------------------------------------------------------------------*/
NPT_PSPDirectory::NPT_PSPDirectory(const char* path) :
    m_Path(path)
{
    if (!m_Path.EndsWith(NPT_WIN32_DIR_DELIMITER_STR)) {
        if (!m_Path.EndsWith(NPT_UNIX_DIR_DELIMITER_STR)) {
            m_Path += NPT_UNIX_DIR_DELIMITER_STR;
        }
    }

    m_Path += "*.*";    
}

/*----------------------------------------------------------------------
|       NPT_PSPDirectory::~NPT_PSPDirectory
+---------------------------------------------------------------------*/
NPT_PSPDirectory::~NPT_PSPDirectory()
{
    //if (m_SearchHandle != NULL) {
    //    FindClose(m_SearchHandle);
    //}
}

/*----------------------------------------------------------------------
|       NPT_PSPDirectory::GetNextEntry
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPDirectory::GetNextEntry(NPT_StringObject& name, NPT_Size* size, NPT_DirectoryEntryType* type)
{
    //WIN32_FIND_DATA filedata;

    //if (m_SearchHandle == NULL) {
    //    m_SearchHandle = FindFirstFile(m_Path, &filedata);
    //    if (m_SearchHandle == INVALID_HANDLE_VALUE) {
    //        m_SearchHandle = NULL;
    //        return NPT_FAILURE;
    //    }   
    //} else {
    //    if (FindNextFile(m_SearchHandle, &filedata) == 0) {
    //        return NPT_FAILURE;
    //    }
    //}

    //// need to convert to Utf8 ?
    //name = filedata.cFileName;

    //if (size) {
    //    *size = filedata.nFileSizeLow;
    //}

    //if (type) {
    //    *type = (filedata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ? NPT_DIRECTORY_TYPE : NPT_FILE_TYPE;
    //}
 
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_Directory::NPT_Directory
+---------------------------------------------------------------------*/
NPT_Directory::NPT_Directory(const char* path)
{
    m_Delegate = new NPT_PSPDirectory(path);
}








