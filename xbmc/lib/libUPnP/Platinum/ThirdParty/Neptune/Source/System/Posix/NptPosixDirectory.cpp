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
#include <stdio.h>

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const char* const NPT_DIR_DELIMITER_STR = "/";
const char        NPT_DIR_DELIMITER_CHR = '/'; 

/*----------------------------------------------------------------------
|       NPT_PosixDirectoryEntry
+---------------------------------------------------------------------*/
class NPT_PosixDirectoryEntry : public NPT_DirectoryEntryInterface
{
public:
    // methods
    NPT_PosixDirectoryEntry(const char* path);
    virtual ~NPT_PosixDirectoryEntry();

    // NPT_DirectoryEntryInterface methods
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
    // replace delimiters with the proper one for the platform
    m_Path.Replace((NPT_DIR_DELIMITER_CHR == '/')?'\\':'/', NPT_DIR_DELIMITER_CHR);

    // remove trailing slashes
    m_Path.TrimRight(NPT_DIR_DELIMITER_CHR);
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

    info.size = S_ISDIR(statbuf.st_mode) ? 0 : statbuf.st_size;
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
    virtual NPT_Result GetInfo(NPT_DirectoryInfo& info);
    virtual NPT_Result GetNextEntry(NPT_String& name, NPT_DirectoryEntryInfo* info = NULL);

private:
    // members
    NPT_String   m_Path;
    DIR*         m_Dir;
    NPT_Cardinal m_Count;
    bool         m_Validated;
};

/*----------------------------------------------------------------------
|       NPT_PosixDirectory::NPT_PosixDirectory
+---------------------------------------------------------------------*/
NPT_PosixDirectory::NPT_PosixDirectory(const char* path) :
    m_Path(path),
    m_Dir(NULL),
    m_Count(0),
    m_Validated(false)
{  
    // replace delimiters with the proper one for the platform
    m_Path.Replace((NPT_DIR_DELIMITER_CHR == '/')?'\\':'/', NPT_DIR_DELIMITER_CHR);

    // remove trailing slashes
    m_Path.TrimRight(NPT_DIR_DELIMITER_CHR);
}

/*----------------------------------------------------------------------
|       NPT_PosixDirectory::~NPT_PosixDirectory
+---------------------------------------------------------------------*/
NPT_PosixDirectory::~NPT_PosixDirectory()
{
    if (m_Dir) closedir(m_Dir);
}

/*----------------------------------------------------------------------
|   NPT_Win32Directory::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixDirectory::GetInfo(NPT_DirectoryInfo& info)
{
    if (!m_Validated) {
        NPT_CHECK(NPT_Directory::GetEntryCount(m_Path, m_Count));
        m_Validated = true;
    }

    info.entry_count = m_Count;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PosixDirectory::GetNextEntry
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixDirectory::GetNextEntry(NPT_String& name, NPT_DirectoryEntryInfo* info)
{
    struct dirent* dp;
    struct stat    statbuf;

    // reset output params first
    name = "";

    if (!m_Dir) {
        m_Dir = opendir(m_Path);
        if (!m_Dir) return NPT_FAILURE;
    }

    if (!(dp = readdir(m_Dir))) 
        return NPT_ERROR_NO_SUCH_ITEM;


    if (!NPT_String::Compare(dp->d_name, ".", false) || !NPT_String::Compare(dp->d_name, "..", false)) {
        return GetNextEntry(name, info);
    }

    // discard system specific files/shortcuts
    NPT_String file_path = m_Path;
    NPT_DirectoryAppendToPath(file_path, dp->d_name);
    if (stat(file_path, &statbuf) == -1) return NPT_FAILURE;

    // assign output params
    name = dp->d_name;
    if (info) {
        info->size = S_ISDIR(statbuf.st_mode) ? 0 : statbuf.st_size;
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


/*----------------------------------------------------------------------
|   NPT_Directory::GetEntryCount
+---------------------------------------------------------------------*/
NPT_Result 
NPT_Directory::GetEntryCount(const char* path, NPT_Cardinal& count)
{
    DIR*           dir = NULL;
    struct dirent* dp;
//    struct stat    statbuf;
    NPT_String     root_path = path;

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
    dir = opendir(root_path);
    if (!dir) return NPT_FAILURE;

    // lopp and disregard system specific files
    while ((dp = readdir(dir))  != NULL) {
        // Get entry's information.
//        if (stat(dp->d_name, &statbuf) == -1)
//            continue;

        if (NPT_String::Compare(dp->d_name, ".", false) && 
            NPT_String::Compare(dp->d_name, "..", false)) 
            ++count;
    };

    closedir(dir);
    return NPT_SUCCESS;
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
        return mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH)?NPT_FAILURE:NPT_SUCCESS;
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
        if (info.type == NPT_DIRECTORY_TYPE) {
            res = rmdir(path)?NPT_FAILURE:NPT_SUCCESS;
        } else {
            res = unlink(path)?NPT_FAILURE:NPT_SUCCESS;
        }

        if (NPT_FAILED(res)) {
            NPT_Debug("NPT_Directory::Remove - errno=%d, Dir = '%s'", 
                errno, path);
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
        res = rename(input, output)?NPT_FAILURE:NPT_SUCCESS;
        if (NPT_FAILED(res)) {
            NPT_Debug("NPT_Directory::Move - errno=%d, Input = '%s', Output = '%s'", 
                errno, input, output);
        }
    }

    return res;
}
