/*****************************************************************
|
|      Neptune - Directory
|
|      (c) 2001-2003 Gilles Boccon-Gibod
|      Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptDirectory.h"

/*----------------------------------------------------------------------
|   NPT_DirectoryAppendToPath
+---------------------------------------------------------------------*/
NPT_Result 
NPT_DirectoryAppendToPath(NPT_String& path, const char* value)
{
    if (!value) return NPT_ERROR_INVALID_PARAMETERS;

    NPT_String tmp = value;

    // make sure path will end with only one trailing delimiter
    path.TrimRight('/');
    path.TrimRight('\\');

    path += NPT_DIR_DELIMITER_STR;

    // make sure value to append doesn't start with delimiters
    tmp.TrimLeft('/');
    tmp.TrimLeft('\\');

    // append value
    path += tmp;

    // replace delimiters with the proper one for the platform
    path.Replace((NPT_DIR_DELIMITER_CHR == '/')?'\\':'/', NPT_DIR_DELIMITER_CHR);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_DirectorySplitFilePath
+---------------------------------------------------------------------*/
NPT_Result 
NPT_DirectorySplitFilePath(const char* filepath, 
                           NPT_String& path, 
                           NPT_String& filename)
{
    if (!filepath || filepath[0] == '\0') 
        return NPT_ERROR_INVALID_PARAMETERS;

    path = filepath;

    char       last_char;
    NPT_Int32  i = path.GetLength();
    do {
        last_char = path[i-1];
        if (last_char == '\\' || last_char == '/') 
            break;
    } while (--i);

    // we need at least one delimiter and it cannot be last
    if (i == 0 || i == (NPT_Int32)path.GetLength())  {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // assign filename
    filename = filepath+i;

    // truncate path & remove trailing slashes
    NPT_CHECK(path.SetLength(i-1));

    // remove excessive delimiters
    path.TrimRight("/");
    path.TrimRight("\\");
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_DirectoryCreate
+---------------------------------------------------------------------*/
NPT_Result 
NPT_DirectoryCreate(const char* path, bool create_parents)
{
    NPT_Result res = NPT_SUCCESS;
    NPT_String fullpath = path;

    // replace delimiters with the proper one for the platform
    fullpath.Replace((NPT_DIR_DELIMITER_CHR == '/')?'\\':'/', NPT_DIR_DELIMITER_CHR);
    // remove excessive delimiters
    fullpath.TrimRight(NPT_DIR_DELIMITER_CHR);

    if (create_parents) {
        NPT_String parent_path;

        // look for a delimiter from the beginning
        int delimiter = fullpath.Find(NPT_DIR_DELIMITER_CHR, 0);
        while (delimiter > 0 && NPT_SUCCEEDED(res)) {
            // copy the path up to the delimiter
            parent_path = fullpath.SubString(0, delimiter);

            // create the directory non recursively
            res = NPT_DirectoryCreate(parent_path, false);   

            // look for the next delimiter
            delimiter = fullpath.Find(NPT_DIR_DELIMITER_CHR, delimiter + 1);
        }

        if (NPT_FAILED(res)) return res;
    }

    // create directory
    return NPT_Directory::Create(fullpath);
}

/*----------------------------------------------------------------------
|   NPT_DirectoryRemove
+---------------------------------------------------------------------*/
NPT_Result 
NPT_DirectoryRemove(const char* path, bool recursively)
{
    NPT_Result             res;
    NPT_DirectoryEntryInfo info;
    NPT_String             root_path = path;

    // replace delimiters with the proper one for the platform
    root_path.Replace((NPT_DIR_DELIMITER_CHR == '/')?'\\':'/', NPT_DIR_DELIMITER_CHR);
    // remove excessive delimiters
    root_path.TrimRight(NPT_DIR_DELIMITER_CHR);

    NPT_CHECK(NPT_DirectoryEntry::GetInfo(root_path, info));
    if (info.type == NPT_FILE_TYPE || !recursively) {
        return NPT_Directory::Remove(root_path);
    }

    {
        // enumerate all entries
        NPT_Directory  dir(path);
        NPT_String     entry_path;
        NPT_String     name;

        do {
            res = dir.GetNextEntry(name, &info);
            if (NPT_SUCCEEDED(res)) {
                // build full path to entry
                entry_path = root_path;

                // append filename
                NPT_DirectoryAppendToPath(entry_path, name);

                // try to remove entry recursively
                res = NPT_DirectoryRemove(entry_path, true);
            }
        } while (NPT_SUCCEEDED(res));
    }

    // if no more items in directory, try to delete it
    if (res == NPT_ERROR_NO_SUCH_ITEM) {
        return NPT_Directory::Remove(path);
    }

    return res;
}
