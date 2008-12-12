/*****************************************************************
|
|      Neptune - Directory :: Posix Implementation
|
|      Copyright (c) 2004-2008, Plutinosoft, LLC.
|      Author: Sylvain Rebaud (sylvain@plutinosoft.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#define _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE64
#define _FILE_OFFSET_BITS 64

#include "NptConfig.h"
#include "NptTypes.h"
#include "NptDirectory.h"
#include "NptDebug.h"
#include "NptResults.h"
#include "NptLogging.h"
#include "NptFile.h"

#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <unistd.h>
#include <stdio.h>

/*----------------------------------------------------------------------
|       logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("neptune.directory.posix")

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const char* const NPT_DIR_DELIMITER_STR = "/";
const char        NPT_DIR_DELIMITER_CHR = '/'; 

/*----------------------------------------------------------------------
|   MapErrno
+---------------------------------------------------------------------*/
static NPT_Result
MapErrno(int err) {
    switch (err) {
      case EACCES:       return NPT_ERROR_PERMISSION_DENIED;
      case EPERM:        return NPT_ERROR_PERMISSION_DENIED;
      case ENOENT:       return NPT_ERROR_NO_SUCH_FILE;
      case ENAMETOOLONG: return NPT_ERROR_INVALID_PARAMETERS;
      case EBUSY:        return NPT_ERROR_FILE_BUSY;
      case EROFS:        return NPT_ERROR_FILE_NOT_WRITABLE;
      case ENOTDIR:      return NPT_ERROR_FILE_NOT_DIRECTORY;
      case EEXIST:       return NPT_ERROR_FILE_ALREADY_EXISTS;
      case ENOSPC:       return NPT_ERROR_FILE_NOT_ENOUGH_SPACE;
      case ENOTEMPTY:    return NPT_ERROR_DIRECTORY_NOT_EMPTY;
      default:           return NPT_ERROR_ERRNO(err);
    }
}

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
    virtual NPT_Result GetInfo(NPT_DirectoryEntryInfo* info = NULL);

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
|       NPT_PosixDirectoryEntry::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixDirectoryEntry::GetInfo(NPT_DirectoryEntryInfo* info /* = NULL */)
{
    NPT_stat_struct statbuf;

    if (NPT_stat(m_Path, &statbuf) == -1) 
        return MapErrno(errno);
    
    if (!S_ISDIR(statbuf.st_mode) && !S_ISREG(statbuf.st_mode))
        return NPT_FAILURE;

    if (info) {
        info->size     = S_ISDIR(statbuf.st_mode) ? 0 : statbuf.st_size;
        info->type     = S_ISDIR(statbuf.st_mode) ? NPT_DIRECTORY_TYPE : NPT_FILE_TYPE;        
        info->created  = (unsigned long)statbuf.st_ctime;
        info->modified = (unsigned long)statbuf.st_mtime;
    }
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
    virtual NPT_Result GetInfo(NPT_DirectoryInfo* info = NULL);
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
NPT_PosixDirectory::GetInfo(NPT_DirectoryInfo* info /* = NULL */)
{
    if (!m_Validated) {
        NPT_CHECK_FATAL(NPT_Directory::GetEntryCount(m_Path, m_Count));
        m_Validated = true;
    }

    if (info) info->entry_count = m_Count;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PosixDirectory::GetNextEntry
+---------------------------------------------------------------------*/
NPT_Result
NPT_PosixDirectory::GetNextEntry(NPT_String& name, NPT_DirectoryEntryInfo* info /* = NULL */)
{
    struct dirent* dp;

    // reset output params first
    name = "";

    if (!m_Dir) {
        m_Dir = opendir(m_Path);
        if (m_Dir == 0) return MapErrno(errno);
    }

    if (!(dp = readdir(m_Dir))) 
        return NPT_ERROR_NO_SUCH_ITEM;


    if (!NPT_String::Compare(dp->d_name, ".", false) || 
        !NPT_String::Compare(dp->d_name, "..", false)) {
        return GetNextEntry(name, info);
    }

    // discard system specific files/shortcuts
    NPT_String file_path = m_Path;
    NPT_DirectoryAppendToPath(file_path, dp->d_name);

    // assign output params
    name = dp->d_name;
    if (!info) return NPT_SUCCESS;

    return NPT_DirectoryEntry::GetInfo(file_path, info);
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
    NPT_String     root_path = path;
    NPT_Result     res;

    // reset output params first
    count = 0;    

    // replace delimiters with the proper one for the platform
    root_path.Replace((NPT_DIR_DELIMITER_CHR == '/')?'\\':'/', NPT_DIR_DELIMITER_CHR);
    // remove trailing slashes
    root_path.TrimRight(NPT_DIR_DELIMITER_CHR);

    // verify it's a directory and not a file
    NPT_DirectoryEntryInfo info;
    NPT_CHECK_FATAL(NPT_DirectoryEntry::GetInfo(root_path, &info));
    if (info.type == NPT_FILE_TYPE) {
        NPT_LOG_WARNING_1("Path not a directory: %s", (const char*)root_path);
        res = NPT_ERROR_INVALID_PARAMETERS;
        goto failure;
    }

    // start enumerating files
    dir = opendir(root_path);
    if (!dir) {
        res = MapErrno(errno);
        NPT_LOG_FATAL_2("Error %d opening path: %s", res, (const char*)root_path);
        goto failure;
    }

    // lopp and disregard system specific files
    while ((dp = readdir(dir))  != NULL) {
        if (NPT_String::Compare(dp->d_name, ".", false) && 
            NPT_String::Compare(dp->d_name, "..", false)) 
            ++count;
    };

    closedir(dir);
    return NPT_SUCCESS;
    
failure:
    NPT_LOG_WARNING_1("Failed to retrieve item counts for path: %s", path); 
    return res;
}

/*----------------------------------------------------------------------
|   NPT_Directory::Create
+---------------------------------------------------------------------*/
NPT_Result 
NPT_Directory::Create(const char* path)
{
    int result;

    // check if path exists, if so no need to create it
    NPT_DirectoryInfo info;
    if (NPT_FAILED(NPT_Directory::GetInfo(path, &info))) {
        NPT_LOG_INFO_1("Creating path: %s", path); 
        result = mkdir(path, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH);
        if (result != 0) return MapErrno(errno);
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
    res = NPT_DirectoryEntry::GetInfo(path, &info);
    if (NPT_SUCCEEDED(res)) {
        int result;
        
        if (info.type == NPT_DIRECTORY_TYPE) {
            result = rmdir(path);
        } else {
            result = unlink(path);
        }
        
        if (result != 0) {
            res = MapErrno(errno);
            NPT_LOG_WARNING_2("Failed %d to remove: Dir = '%s'", 
                res, path);        
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
   // make sure the path exists
    NPT_Result res = NPT_DirectoryEntry::GetInfo(input);
    if (NPT_SUCCEEDED(res)) {
        int result = rename(input, output);
        if (result != 0) {
            res = MapErrno(errno);
            NPT_LOG_WARNING_3("Failed %d to rename: Input = '%s', Output = '%s'", 
                res, input, output);
        }
    }

    return res;
}
