/*****************************************************************
|
|      Neptune - Directory :: Posix Implementation
|
|      (c) 2004 Sylvain Rebaud
|      Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptTypes.h"
#include "NptDirectory.h"
#include "NptDebug.h"
#include "NptResults.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>

/*----------------------------------------------------------------------
|       NPT_PosixDirectoryEntry
+---------------------------------------------------------------------*/
class NPT_PosixDirectoryEntry : public NPT_DirectoryEntryInterface
{
public:
    // methods
    NPT_PosixDirectoryEntry(const char* path);
    virtual ~NPT_PosixDirectoryEntry();

    // NPT_DirectoryInterface methods
    virtual NPT_Result GetInfo(NPT_DirectoryEntryInfo& info);

private:
    // members
    NPT_String m_Path;
};

/*----------------------------------------------------------------------
|       NPT_PosixDirectoryEntry::NPT_PosixDirectoryEntry
+---------------------------------------------------------------------*/
NPT_PosixDirectoryEntry::NPT_PosixDirectoryEntry(const char* path) :
    m_Path(path)
{  
}

/*----------------------------------------------------------------------
|       NPT_PosixDirectoryEntry::~NPT_PosixDirectoryEntry
+---------------------------------------------------------------------*/
NPT_PosixDirectoryEntry::~NPT_PosixDirectoryEntry()
{
}

/*----------------------------------------------------------------------
|       NPT_PosixDirectoryEntry::GetSize
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixDirectoryEntry::GetInfo(NPT_DirectoryEntryInfo& info)
{
    struct stat statbuf;

    if (stat(m_Path, &statbuf) == -1) 
        return NPT_ERROR_NO_SUCH_ITEM;

    if (!S_ISDIR(statbuf.st_mode) && !S_ISREG(statbuf.st_mode))
        return NPT_FAILURE;

    info.size = statbuf.st_size;
    info.type = S_ISDIR(statbuf.st_mode) ? NPT_DIRECTORY_TYPE : NPT_FILE_TYPE;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_DirectoryEntry::NPT_DirectoryEntry
+---------------------------------------------------------------------*/
NPT_DirectoryEntry::NPT_DirectoryEntry(const char* path)
{
    m_Delegate = new NPT_PosixDirectoryEntry(path);
}

/*----------------------------------------------------------------------
|       NPT_PosixDirectory
+---------------------------------------------------------------------*/
class NPT_PosixDirectory : public NPT_DirectoryInterface
{
public:
    // methods
    NPT_PosixDirectory(const char* path);
    virtual ~NPT_PosixDirectory();

    // NPT_DirectoryInterface methods
    virtual NPT_Result GetNextEntry(NPT_String& name, NPT_DirectoryEntryInfo* info = NULL);

private:
    // members
    NPT_String m_Path;
    DIR*       m_Dir;
};

/*----------------------------------------------------------------------
|       NPT_PosixDirectory::NPT_PosixDirectory
+---------------------------------------------------------------------*/
NPT_PosixDirectory::NPT_PosixDirectory(const char* path) :
    m_Path(path),
    m_Dir(NULL)
{  
    if (!m_Path.EndsWith(NPT_UNIX_DIR_DELIMITER_STR)) {
        if (!m_Path.EndsWith(NPT_WIN32_DIR_DELIMITER_STR)) {
            m_Path += NPT_UNIX_DIR_DELIMITER_STR;
        }
    }
}

/*----------------------------------------------------------------------
|       NPT_PosixDirectory::~NPT_PosixDirectory
+---------------------------------------------------------------------*/
NPT_PosixDirectory::~NPT_PosixDirectory()
{
    if (m_Dir) closedir(m_Dir);
}

/*----------------------------------------------------------------------
|       NPT_PosixDirectory::GetNextEntry
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixDirectory::GetNextEntry(NPT_String& name, NPT_DirectoryEntryInfo* info)
{
    struct dirent* dp;
    struct stat    statbuf;

    if (!m_Dir) {
        m_Dir = opendir(m_Path);
        if (!m_Dir) return NPT_FAILURE;
    }

    if (!(dp = readdir(m_Dir))) 
        return NPT_ERROR_NO_SUCH_ITEM;


    if (!NPT_String::Compare(dp->d_name, ".", false) || !NPT_String::Compare(dp->d_name, "..", false)) {
        return GetNextEntry(name, info);
    }

    NPT_String file_path = m_Path + dp->d_name;
    if (stat(file_path, &statbuf) == -1) return NPT_FAILURE;

    name = dp->d_name;

    if (info) {
        info->size = statbuf.st_size;
        info->type = S_ISDIR(statbuf.st_mode) ? NPT_DIRECTORY_TYPE : NPT_FILE_TYPE;
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_Directory::NPT_Directory
+---------------------------------------------------------------------*/
NPT_Directory::NPT_Directory(const char* path)
{
    m_Delegate = new NPT_PosixDirectory(path);
}








