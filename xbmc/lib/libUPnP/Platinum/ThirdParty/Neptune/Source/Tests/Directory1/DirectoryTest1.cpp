/*****************************************************************
|
|      Directory Test Program 1
|
|      (c) 2005 Gilles Boccon-Gibod
|      Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "Neptune.h"
#include "NptDebug.h"

#define CHECK(x) {                                  \
    if (!(x)) {                                     \
        printf("TEST FAILED line %d\n", __LINE__);  \
        exit(1);                                    \
    }                                               \
}

/*----------------------------------------------------------------------
|   FindNextAvailableDirectory
+---------------------------------------------------------------------*/
static NPT_Result
FindNextAvailableDirectory(const char* root_path, NPT_String& path)
{
    NPT_String             base_path;
    NPT_DirectoryEntryInfo info;
    NPT_Result             res;
    NPT_Cardinal           index = 0;

    // verify arguments
    if (!root_path) return NPT_FAILURE;

    // create base path from root path 
    base_path = root_path;
    NPT_DirectoryAppendToPath(base_path, "Tmp");

    do {
        // copy base path into our return value 
        path = base_path;

        // append a number 
        path += NPT_String::FromInteger(index);

        // try to open directory 
        res = NPT_DirectoryEntry::GetInfo(path, info);
        if (NPT_FAILED(res)) break;

    } while (++index < 100);

    return (index<100)?NPT_SUCCESS:NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   CreateEntry
+---------------------------------------------------------------------*/
static NPT_Result
CreateEntry(const char* root_path, const char* name, bool directory)
{
    NPT_Result res;
    NPT_String path = root_path;
    NPT_DirectoryAppendToPath(path, name);

    if (directory) {
        res = NPT_DirectoryCreate(path, true);
    } else {
        NPT_File* file = new NPT_File(path);
        res = file->Open(NPT_FILE_OPEN_MODE_TRUNCATE | NPT_FILE_OPEN_MODE_WRITE | NPT_FILE_OPEN_MODE_CREATE);
        delete file;
    }

    return res;
}

/*----------------------------------------------------------------------
|   TestRemoveEmptyDirectory
+---------------------------------------------------------------------*/
void
TestRemoveEmptyDirectory()
{
    NPT_DirectoryEntryInfo info;
    NPT_String             path;
    NPT_Result             res;

    // read path from environment variable
    res = NPT_GetEnvironment("NEPTUNE_TEST_ROOT", path);
    CHECK(NPT_SUCCEEDED(res));

    // find the next directory path that does not exist
    res = FindNextAvailableDirectory(path, path);
    CHECK(NPT_SUCCEEDED(res));

    // create directory object
    res = NPT_DirectoryCreate(path, true);
    CHECK(NPT_SUCCEEDED(res));

    // remove empty directory
    res = NPT_DirectoryRemove(path, false);
    CHECK(NPT_SUCCEEDED(res));

    // verify directory does not exist any more
    res = NPT_DirectoryEntry::GetInfo(path, info);
    CHECK(res == NPT_ERROR_NO_SUCH_ITEM);
}

/*----------------------------------------------------------------------
|   TestRemoveDirectory
+---------------------------------------------------------------------*/
void
TestRemoveDirectory()
{
    NPT_DirectoryEntryInfo info;
    NPT_String             path;
    NPT_Result             res;

    // read path from environment variable
    res = NPT_GetEnvironment("NEPTUNE_TEST_ROOT", path);
    CHECK(NPT_SUCCEEDED(res));

    // find the next directory path that does not exist
    res = FindNextAvailableDirectory(path, path);
    CHECK(NPT_SUCCEEDED(res));

    // create directory object
    res = NPT_DirectoryCreate(path, true);
    CHECK(NPT_SUCCEEDED(res));

    // create some subdirectories
    res = CreateEntry(path, "foof1", true);
    CHECK(NPT_SUCCEEDED(res));
    res = CreateEntry(path, "foof2", true);
    CHECK(NPT_SUCCEEDED(res));
    res = CreateEntry(path, "foof2.dir", true);
    CHECK(NPT_SUCCEEDED(res));

    // create some files
    res = CreateEntry(path, "foo1", false);
    CHECK(NPT_SUCCEEDED(res));
    res = CreateEntry(path, "foo2.", false);
    CHECK(NPT_SUCCEEDED(res));
    res = CreateEntry(path, "foo2.e", false);
    CHECK(NPT_SUCCEEDED(res));
    res = CreateEntry(path, "foo2.e3", false);
    CHECK(NPT_SUCCEEDED(res));
    res = CreateEntry(path, "foo2.gas", false);
    CHECK(NPT_SUCCEEDED(res));
    res = CreateEntry(path, "foo4.tee", false);
    CHECK(NPT_SUCCEEDED(res));

    // remove directory
    res = NPT_DirectoryRemove(path, true);
    CHECK(NPT_SUCCEEDED(res));

    // verify directory does not exist any more
    res = NPT_DirectoryEntry::GetInfo(path, info);
    CHECK(res == NPT_ERROR_NO_SUCH_ITEM);
}

/*----------------------------------------------------------------------
|   TestFormatPath
+---------------------------------------------------------------------*/
void
TestFormatPath()
{
    NPT_String path;
    NPT_String filename;
    NPT_Result res;

    res = NPT_DirectorySplitFilePath("", path, filename);
    CHECK(NPT_FAILED(res));

    res = NPT_DirectorySplitFilePath("/", path, filename);
    CHECK(NPT_FAILED(res));

    res = NPT_DirectorySplitFilePath("\\", path, filename);
    CHECK(NPT_FAILED(res));

    res = NPT_DirectorySplitFilePath("/blah/", path, filename);
    CHECK(NPT_FAILED(res));

    res = NPT_DirectorySplitFilePath("blah", path, filename);
    CHECK(NPT_FAILED(res));

    res = NPT_DirectorySplitFilePath("blah\\", path, filename);
    CHECK(NPT_FAILED(res));

    res = NPT_DirectorySplitFilePath("blah/", path, filename);
    CHECK(NPT_FAILED(res));

    res = NPT_DirectorySplitFilePath("C:\\blah\\", path, filename);
    CHECK(NPT_FAILED(res));

    res = NPT_DirectorySplitFilePath("/foo/bar", path, filename);
    CHECK(NPT_SUCCEEDED(res));
    CHECK("/foo" == path);
    CHECK("bar" == filename);

    res = NPT_DirectorySplitFilePath("/foo//bar", path, filename);
    CHECK(NPT_SUCCEEDED(res));
    CHECK("/foo" == path);
    CHECK("bar" == filename);

    res = NPT_DirectorySplitFilePath("C:\\foo\\bar", path, filename);
    CHECK(NPT_SUCCEEDED(res));
    CHECK("C:\\foo" == path);
    CHECK("bar" == filename);

    res = NPT_DirectorySplitFilePath("C:\\foo\\\\bar", path, filename);
    CHECK(NPT_SUCCEEDED(res));
    CHECK("C:\\foo" == path);
    CHECK("bar" == filename);

    if (NPT_DIR_DELIMITER_CHR == '\\') {
        path = "C:\\foo\\";

        res = NPT_DirectoryAppendToPath(path, "\\bar\\boo");
        CHECK(NPT_SUCCEEDED(res));
        CHECK("C:\\foo\\bar\\boo" == path);

        path = "C:\\foo";
        res = NPT_DirectoryAppendToPath(path, "\\bar\\boo");
        CHECK(NPT_SUCCEEDED(res));
        CHECK("C:\\foo\\bar\\boo" == path);

        path = "C:\\foo";
        res = NPT_DirectoryAppendToPath(path, "bar\\boo");
        CHECK(NPT_SUCCEEDED(res));
        CHECK("C:\\foo\\bar\\boo" == path);

        path = "C:\\foo";
        res = NPT_DirectoryAppendToPath(path, "\\bar\\boo\\");
        CHECK(NPT_SUCCEEDED(res));
        CHECK("C:\\foo\\bar\\boo\\" == path);

        path = "C:\\foo";
        res = NPT_DirectoryAppendToPath(path, "bar/boo");
        CHECK(NPT_SUCCEEDED(res));
        CHECK("C:\\foo\\bar\\boo" == path);

        path = "C:\\foo";
        res = NPT_DirectoryAppendToPath(path, "/bar/boo/");
        CHECK(NPT_SUCCEEDED(res));
        CHECK("C:\\foo\\bar\\boo\\" == path);
    } else {
        path = "/foo";

        res = NPT_DirectoryAppendToPath(path, "/bar/boo");
        CHECK(NPT_SUCCEEDED(res));
        CHECK("/foo/bar/boo" == path);

        path = "/foo";
        res = NPT_DirectoryAppendToPath(path, "/bar/boo");
        CHECK(NPT_SUCCEEDED(res));
        CHECK("/foo/bar/boo" == path);

        path = "/foo";
        res = NPT_DirectoryAppendToPath(path, "bar/boo");
        CHECK(NPT_SUCCEEDED(res));
        CHECK("/foo/bar/boo" == path);

        path = "/foo";
        res = NPT_DirectoryAppendToPath(path, "/bar/boo/");
        CHECK(NPT_SUCCEEDED(res));
        CHECK("/foo/bar/boo/" == path);

        path = "/foo";
        res = NPT_DirectoryAppendToPath(path, "\\bar\\boo");
        CHECK(NPT_SUCCEEDED(res));
        CHECK("/foo/bar/boo" == path);

        path = "/foo";
        res = NPT_DirectoryAppendToPath(path, "\\bar\\boo\\");
        CHECK(NPT_SUCCEEDED(res));
        CHECK("/foo/bar/boo/" == path);
    }
}

/*----------------------------------------------------------------------
|   main
+---------------------------------------------------------------------*/
int
main(int /*argc*/, char** /*argv*/)
{
    TestRemoveEmptyDirectory();
    TestRemoveDirectory();
    TestFormatPath();
}
