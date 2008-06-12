/*****************************************************************
|
|   Neptune - Files
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_FILE_H_
#define _NPT_FILE_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptStreams.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const int NPT_ERROR_NO_SUCH_FILE      = NPT_ERROR_BASE_FILE - 0;
const int NPT_ERROR_FILE_NOT_OPEN     = NPT_ERROR_BASE_FILE - 1;
const int NPT_ERROR_FILE_BUSY         = NPT_ERROR_BASE_FILE - 2;
const int NPT_ERROR_FILE_ALREADY_OPEN = NPT_ERROR_BASE_FILE - 3;
const int NPT_ERROR_FILE_NOT_READABLE = NPT_ERROR_BASE_FILE - 4;
const int NPT_ERROR_FILE_NOT_WRITABLE = NPT_ERROR_BASE_FILE - 5;

const unsigned int NPT_FILE_OPEN_MODE_READ       = 0x01;
const unsigned int NPT_FILE_OPEN_MODE_WRITE      = 0x02;
const unsigned int NPT_FILE_OPEN_MODE_CREATE     = 0x04;
const unsigned int NPT_FILE_OPEN_MODE_TRUNCATE   = 0x08;
const unsigned int NPT_FILE_OPEN_MODE_UNBUFFERED = 0x10;
const unsigned int NPT_FILE_OPEN_MODE_APPEND     = 0x20;

#define NPT_FILE_STANDARD_INPUT  "@STDIN"
#define NPT_FILE_STANDARD_OUTPUT "@STDOUT"
#define NPT_FILE_STANDARD_ERROR  "@STDERR"

/*----------------------------------------------------------------------
|   class references
+---------------------------------------------------------------------*/
class NPT_DataBuffer;

/*----------------------------------------------------------------------
|   NPT_FileInterface
+---------------------------------------------------------------------*/
class NPT_FileInterface
{
public:
    // types
    typedef unsigned int OpenMode;

    // constructors and destructor
    virtual ~NPT_FileInterface() {}

    // methods
    virtual NPT_Result Open(OpenMode mode) = 0;
    virtual NPT_Result Close() = 0;
    //virtual bool       Exists() = 0;
    //virtual NPT_Result Delete() = 0;
    //virtual NPT_Result Rename(const char* name) = 0;
    virtual NPT_Result GetSize(NPT_Size& size) = 0;
    virtual NPT_Result GetInputStream(NPT_InputStreamReference& stream) = 0;
    virtual NPT_Result GetOutputStream(NPT_OutputStreamReference& stream) = 0;
};

/*----------------------------------------------------------------------
|   NPT_File
+---------------------------------------------------------------------*/
class NPT_File : public NPT_FileInterface
{
public:
    // class methods
    static NPT_Result Load(const char* filename, NPT_String& data, NPT_FileInterface::OpenMode mode = NPT_FILE_OPEN_MODE_READ);
    static NPT_Result Load(const char* filename, NPT_DataBuffer& buffer, NPT_FileInterface::OpenMode mode = NPT_FILE_OPEN_MODE_READ);
    static NPT_Result Save(const char* filename, NPT_DataBuffer& buffer);

    // constructors and destructor
    NPT_File(const char* name);
   ~NPT_File() { delete m_Delegate; }

    // methods
    NPT_Result Load(NPT_DataBuffer& buffer);
    NPT_Result Save(NPT_DataBuffer& buffer);

    // NPT_FileInterface methods
    NPT_Result Open(OpenMode mode) {
        return m_Delegate->Open(mode);
    }
    NPT_Result Close() {
        return m_Delegate->Close();
    }
    NPT_Result GetSize(NPT_Size& size) {
        return m_Delegate->GetSize(size);
    }
    NPT_Result GetInputStream(NPT_InputStreamReference& stream) {
        return m_Delegate->GetInputStream(stream);
    }
    NPT_Result GetOutputStream(NPT_OutputStreamReference& stream) {
        return m_Delegate->GetOutputStream(stream);
    }

protected:
    // members
    NPT_FileInterface* m_Delegate;
};

#endif // _NPT_FILE_H_ 
