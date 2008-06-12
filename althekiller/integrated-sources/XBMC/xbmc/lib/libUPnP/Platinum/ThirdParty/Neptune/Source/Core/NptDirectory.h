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
#include "NptResults.h"
#include "NptTime.h"

/*----------------------------------------------------------------------
|   defines
+---------------------------------------------------------------------*/
extern const char* const NPT_DIR_DELIMITER_STR;
extern const char        NPT_DIR_DELIMITER_CHR;

typedef enum {
    NPT_FILE_TYPE,
    NPT_DIRECTORY_TYPE
} NPT_DirectoryEntryType;

typedef struct {
    NPT_Cardinal entry_count;
} NPT_DirectoryInfo;

typedef struct {
    NPT_DirectoryEntryType type;
    NPT_Size               size;
    NPT_TimeStamp          created;
    NPT_TimeStamp          modified;
} NPT_DirectoryEntryInfo;

/*----------------------------------------------------------------------
|       NPT_DirectoryEntryInterface
+---------------------------------------------------------------------*/
class NPT_DirectoryEntryInterface
{
public:
    virtual ~NPT_DirectoryEntryInterface() {}

    // class methods
    virtual NPT_Result GetInfo(NPT_DirectoryEntryInfo& info) = 0;
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

    NPT_Result GetInfo(NPT_DirectoryEntryInfo& info) {
        return m_Delegate->GetInfo(info);
    }

    // static helper methods
    static NPT_Result GetInfo(const char* path, NPT_DirectoryEntryInfo& info) {
        NPT_DirectoryEntry entry(path);
        return entry.GetInfo(info);
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
    virtual NPT_Result GetInfo(NPT_DirectoryInfo& info) = 0;
    virtual NPT_Result GetNextEntry(NPT_String& name, NPT_DirectoryEntryInfo* info = NULL) = 0;
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

    NPT_Result GetInfo(NPT_DirectoryInfo& info) {
        return m_Delegate->GetInfo(info);
    }

    NPT_Result GetNextEntry(NPT_String& name, NPT_DirectoryEntryInfo* info = NULL) {
        return m_Delegate->GetNextEntry(name, info);
    }

    // static methods
    static NPT_Result GetInfo(const char* path, NPT_DirectoryInfo& info) {
        NPT_Directory dir(path);
        return dir.GetInfo(info);
    }
    static NPT_Result GetEntryCount(const char* path, NPT_Cardinal& count);
    static NPT_Result Create(const char* path);
    static NPT_Result Remove(const char* path);
    static NPT_Result Move(const char* input, const char* output);

private:
    // members
    NPT_DirectoryInterface* m_Delegate;
};

extern NPT_Result NPT_DirectoryAppendToPath(NPT_String& path, const char* value);
extern NPT_Result NPT_DirectorySplitFilePath(const char* filepath, 
                                             NPT_String& path, 
                                             NPT_String& filename);
extern NPT_Result NPT_DirectoryCreate(const char* path, bool create_parents);
extern NPT_Result NPT_DirectoryRemove(const char* path, bool recursively);

#endif // _NPT_DIRECTORY_H_ 
