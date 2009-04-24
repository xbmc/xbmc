/*****************************************************************
|
|   Neptune - Files :: XBMC Implementation
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "stdafx.h"
#include "IFile.h"
#include "FileFactory.h"
#include "utils/log.h"
#include "Util.h"
#include "URL.h"

#ifdef max
#undef max
#endif

#ifdef min
#undef min
#endif

#include <limits>


#include "NptUtils.h"
#include "NptFile.h"
#include "NptThreads.h"
#include "NptInterfaces.h"
#include "NptStrings.h"
#include "NptDebug.h"

using namespace XFILE;

typedef NPT_Reference<IFile> NPT_XbmcFileReference;

/*----------------------------------------------------------------------
|   NPT_XbmcFileStream
+---------------------------------------------------------------------*/
class NPT_XbmcFileStream
{
public:
    // constructors and destructor
    NPT_XbmcFileStream(NPT_XbmcFileReference file) :
      m_FileReference(file) {}

    // NPT_FileInterface methods
    NPT_Result Seek(NPT_Position offset);
    NPT_Result Tell(NPT_Position& offset);

protected:
    // constructors and destructors
    virtual ~NPT_XbmcFileStream() {}

    // members
    NPT_XbmcFileReference m_FileReference;
};

/*----------------------------------------------------------------------
|   NPT_XbmcFileStream::Seek
+---------------------------------------------------------------------*/
NPT_Result
NPT_XbmcFileStream::Seek(NPT_Position offset)
{
    __int64 result;

    result = m_FileReference->Seek(offset, SEEK_SET)    ;
    if (result >= 0) {
        return NPT_SUCCESS;
    } else {
        return NPT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   NPT_XbmcFileStream::Tell
+---------------------------------------------------------------------*/
NPT_Result
NPT_XbmcFileStream::Tell(NPT_Position& offset)
{
    __int64 result = m_FileReference->GetPosition();
    if (result >= 0) {
        offset = (NPT_Position)result;
        return NPT_SUCCESS;
    } else {
        return NPT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   NPT_XbmcFileInputStream
+---------------------------------------------------------------------*/
class NPT_XbmcFileInputStream : public NPT_InputStream,
                                private NPT_XbmcFileStream
                                
{
public:
    // constructors and destructor
    NPT_XbmcFileInputStream(NPT_XbmcFileReference& file, NPT_LargeSize size) :
        NPT_XbmcFileStream(file), m_Size(size) {}

    // NPT_InputStream methods
    NPT_Result Read(void*     buffer, 
                    NPT_Size  bytes_to_read, 
                    NPT_Size* bytes_read);
    NPT_Result Seek(NPT_Position offset) {
        return NPT_XbmcFileStream::Seek(offset);
    }
    NPT_Result Tell(NPT_Position& offset) {
        return NPT_XbmcFileStream::Tell(offset);
    }
    NPT_Result GetSize(NPT_LargeSize& size);
    NPT_Result GetAvailable(NPT_LargeSize& available);

private:
    // members
    NPT_LargeSize m_Size;
};

/*----------------------------------------------------------------------
|   NPT_XbmcFileInputStream::Read
+---------------------------------------------------------------------*/
NPT_Result
NPT_XbmcFileInputStream::Read(void*     buffer, 
                              NPT_Size  bytes_to_read, 
                              NPT_Size* bytes_read)
{
    unsigned int nb_read;

    // check the parameters
    if (buffer == NULL) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // read from the file
    nb_read = m_FileReference->Read(buffer, bytes_to_read);    
    if (nb_read > 0) {
        if (bytes_read) *bytes_read = (NPT_Size)nb_read;
        return NPT_SUCCESS;
    } else {
        if (bytes_read) *bytes_read = 0;
        return NPT_ERROR_EOS;
    //} else { // currently no way to indicate failure
    //    if (bytes_read) *bytes_read = 0;
    //    return NPT_ERROR_READ_FAILED;
    }
}

/*----------------------------------------------------------------------
|   NPT_XbmcFileInputStream::GetSize
+---------------------------------------------------------------------*/
NPT_Result
NPT_XbmcFileInputStream::GetSize(NPT_LargeSize& size)
{
    size = m_Size;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_XbmcFileInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result
NPT_XbmcFileInputStream::GetAvailable(NPT_LargeSize& available)
{
    __int64 offset = m_FileReference->GetPosition();
    if (offset >= 0 && (NPT_LargeSize)offset <= m_Size) {
        available = m_Size - offset;
        return NPT_SUCCESS;
    } else {
        available = 0;
        return NPT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   NPT_XbmcFileOutputStream
+---------------------------------------------------------------------*/
class NPT_XbmcFileOutputStream : public NPT_OutputStream,
                                 private NPT_XbmcFileStream
{
public:
    // constructors and destructor
    NPT_XbmcFileOutputStream(NPT_XbmcFileReference& file) :
        NPT_XbmcFileStream(file) {}

    // NPT_OutputStream methods
    NPT_Result Write(const void* buffer, 
                     NPT_Size    bytes_to_write, 
                     NPT_Size*   bytes_written);
    NPT_Result Seek(NPT_Position offset) {
        return NPT_XbmcFileStream::Seek(offset);
    }
    NPT_Result Tell(NPT_Position& offset) {
        return NPT_XbmcFileStream::Tell(offset);
    }
};

/*----------------------------------------------------------------------
|   NPT_XbmcFileOutputStream::Write
+---------------------------------------------------------------------*/
NPT_Result
NPT_XbmcFileOutputStream::Write(const void* buffer, 
                                NPT_Size    bytes_to_write, 
                                NPT_Size*   bytes_written)
{
    int nb_written;
    nb_written = m_FileReference->Write(buffer, bytes_to_write);    

    if (nb_written > 0) {
        if (bytes_written) *bytes_written = (NPT_Size)nb_written;
        return NPT_SUCCESS;
    } else {
        if (bytes_written) *bytes_written = 0;
        return NPT_ERROR_WRITE_FAILED;
    }
}

/*----------------------------------------------------------------------
|   NPT_XbmcFile
+---------------------------------------------------------------------*/
class NPT_XbmcFile: public NPT_FileInterface
{
public:
    // constructors and destructor
    NPT_XbmcFile(NPT_File& delegator);
   ~NPT_XbmcFile();

    // NPT_FileInterface methods
    NPT_Result Open(OpenMode mode);
    NPT_Result Close();
    NPT_Result GetSize(NPT_LargeSize& size);
    NPT_Result GetInputStream(NPT_InputStreamReference& stream);
    NPT_Result GetOutputStream(NPT_OutputStreamReference& stream);

private:
    // members
    NPT_File&             m_Delegator;
    OpenMode              m_Mode;
    NPT_XbmcFileReference m_FileReference;
    NPT_LargeSize         m_Size;
};

/*----------------------------------------------------------------------
|   NPT_XbmcFile::NPT_XbmcFile
+---------------------------------------------------------------------*/
NPT_XbmcFile::NPT_XbmcFile(NPT_File& delegator) :
    m_Delegator(delegator),
    m_Mode(0),
    m_Size(0)
{
}

/*----------------------------------------------------------------------
|   NPT_XbmcFile::~NPT_XbmcFile
+---------------------------------------------------------------------*/
NPT_XbmcFile::~NPT_XbmcFile()
{
    Close();
}

/*----------------------------------------------------------------------
|   NPT_XbmcFile::Open
+---------------------------------------------------------------------*/
NPT_Result
NPT_XbmcFile::Open(NPT_File::OpenMode mode)
{
    NPT_XbmcFileReference file;

    // check if we're already open
    if (!m_FileReference.IsNull()) {
        return NPT_ERROR_FILE_ALREADY_OPEN;
    }

    // store the mode
    m_Mode = mode;

    // check for special names
    const char* name = (const char*)m_Delegator.GetPath();
    if (NPT_StringsEqual(name, NPT_FILE_STANDARD_INPUT)) {
        return NPT_ERROR_FILE_NOT_READABLE;
    } else if (NPT_StringsEqual(name, NPT_FILE_STANDARD_OUTPUT)) {
        return NPT_ERROR_FILE_NOT_WRITABLE;
    } else if (NPT_StringsEqual(name, NPT_FILE_STANDARD_ERROR)) {
        return NPT_ERROR_FILE_NOT_WRITABLE;
    } else {

        file = CFileFactory::CreateLoader(name);
        if (file.IsNull()) {
            return NPT_ERROR_NO_SUCH_FILE;
        }

        bool result;
        CURL* url = new CURL(name);
        /* path is not fully qualified so assume it's relative to home dir */
        if (url->GetFileName().IsEmpty()) {
            delete url;
            CStdString homepath;
            CUtil::GetHomePath(homepath);
            url = new CURL(homepath + "/" + name);
        }

        // compute mode
        if (mode & NPT_FILE_OPEN_MODE_WRITE) {
            result = file->OpenForWrite(*url, (mode & NPT_FILE_OPEN_MODE_TRUNCATE)?true:false);
        } else {
            result = file->Open(*url);
        }

        delete url;
        if (!result) return NPT_ERROR_NO_SUCH_FILE;

        m_Size = (NPT_LargeSize)file->GetLength();
    }

    // store reference
    m_FileReference = file;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_XbmcFile::Close
+---------------------------------------------------------------------*/
NPT_Result
NPT_XbmcFile::Close()
{
    // release the file reference
    m_FileReference = NULL;

    // reset the mode
    m_Mode = 0;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_XbmcFile::GetSize
+---------------------------------------------------------------------*/
NPT_Result 
NPT_XbmcFile::GetSize(NPT_LargeSize& size)
{
    // default value
    size = 0;

    // check that the file is open
    if (m_FileReference.IsNull()) return NPT_ERROR_FILE_NOT_OPEN;

    // return the size
    size = m_Size;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_XbmcFile::GetInputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_XbmcFile::GetInputStream(NPT_InputStreamReference& stream)
{
    // default value
    stream = NULL;

    // check that the file is open
    if (m_FileReference.IsNull()) return NPT_ERROR_FILE_NOT_OPEN;

    // check that the mode is compatible
    if (!(m_Mode & NPT_FILE_OPEN_MODE_READ)) {
        return NPT_ERROR_FILE_NOT_READABLE;
    }

    // create a stream
    stream = new NPT_XbmcFileInputStream(m_FileReference, m_Size);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_XbmcFile::GetOutputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_XbmcFile::GetOutputStream(NPT_OutputStreamReference& stream)
{
    // default value
    stream = NULL;

    // check that the file is open
    if (m_FileReference.IsNull()) return NPT_ERROR_FILE_NOT_OPEN;

    // check that the mode is compatible
    if (!(m_Mode & NPT_FILE_OPEN_MODE_WRITE)) {
        return NPT_ERROR_FILE_NOT_WRITABLE;
    }
    
    // create a stream
    stream = new NPT_XbmcFileOutputStream(m_FileReference);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_File::NPT_File
+---------------------------------------------------------------------*/
NPT_File::NPT_File(const char* path) :
    m_Path(path)
{
    m_Delegate = new NPT_XbmcFile(*this);
}

/*----------------------------------------------------------------------
|   NPT_File::operator=
+---------------------------------------------------------------------*/
NPT_File& 
NPT_File::operator=(const NPT_File& file)
{
    if (this != &file) {
        delete m_Delegate;
        m_Path = file.m_Path;
        m_Info = file.m_Info;
        m_Delegate = new NPT_XbmcFile(*this);
    }
    return *this;
}


