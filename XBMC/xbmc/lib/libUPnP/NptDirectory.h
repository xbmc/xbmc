/*****************************************************************
|
|      Neptune - Directory
|
|      (c) 2001-2003 Gilles Boccon-Gibod
|      Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

#ifndef _NPT_DIRECTORY_H_
#define _NPT_DIRECTORY_H_

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptStrings.h"

/*----------------------------------------------------------------------
|       defines
+---------------------------------------------------------------------*/
/* Win32 directory delimiter */
#define NPT_WIN32_DIR_DELIMITER_CHR '\\'
#define NPT_WIN32_DIR_DELIMITER_STR "\\"

/* UNIX directory delimiter */
#define NPT_UNIX_DIR_DELIMITER_CHR '/'
#define NPT_UNIX_DIR_DELIMITER_STR "/"

typedef enum {
    NPT_FILE_TYPE,
    NPT_DIRECTORY_TYPE
} NPT_DirectoryEntryType;

/*----------------------------------------------------------------------
|       NPT_DirectoryEntryInterface
+---------------------------------------------------------------------*/
class NPT_DirectoryEntryInterface
{
public:
    virtual ~NPT_DirectoryEntryInterface() {}

    // class methods
    virtual NPT_Result GetType(NPT_DirectoryEntryType& type) = 0;
    virtual NPT_Result GetSize(NPT_Size& size) = 0;
};

/*----------------------------------------------------------------------
|       NPT_DirectoryEntry
+---------------------------------------------------------------------*/
class NPT_DirectoryEntry : public NPT_DirectoryEntryInterface
{
public:
    // methods
    NPT_DirectoryEntry(const char* path);

    // methods
    ~NPT_DirectoryEntry() {
        delete m_Delegate;
    }

    NPT_Result GetType(NPT_DirectoryEntryType& type) {
        return m_Delegate->GetType(type);
    }
    NPT_Result GetSize(NPT_Size& size) {
        return m_Delegate->GetSize(size);
    }

    // static helper methods
    static NPT_Result GetType(const char* path, NPT_DirectoryEntryType& type) {
        NPT_DirectoryEntry entry(path);
        return entry.GetType(type);
    }
    static NPT_Result GetSize(const char* path, NPT_Size& size) {
        NPT_DirectoryEntry entry(path);
        return entry.GetSize(size);
    }

private:
    // members
    NPT_DirectoryEntryInterface* m_Delegate;
};

/*----------------------------------------------------------------------
|       NPT_DirectoryInterface
+---------------------------------------------------------------------*/
class NPT_DirectoryInterface
{
public:
    virtual ~NPT_DirectoryInterface() {}

    // class methods
    virtual NPT_Result GetNextEntry(NPT_String& name, NPT_Size* size = NULL, NPT_DirectoryEntryType* type = NULL) = 0;
};

/*----------------------------------------------------------------------
|       NPT_Directory
+---------------------------------------------------------------------*/
class NPT_Directory : public NPT_DirectoryInterface
{
public:
    // methods
    NPT_Directory(const char* path);

    // methods
    ~NPT_Directory() {
        delete m_Delegate;
    }

    NPT_Result GetNextEntry(NPT_String& name, NPT_Size* size = NULL, NPT_DirectoryEntryType* type = NULL) {
        return m_Delegate->GetNextEntry(name, size, type);
    }

private:
    // members
    NPT_DirectoryInterface* m_Delegate;
};

#endif // _NPT_DIRECTORY_H_ 
