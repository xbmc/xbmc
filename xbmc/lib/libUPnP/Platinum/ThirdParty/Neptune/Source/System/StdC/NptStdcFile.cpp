/*****************************************************************
|
|   Neptune - Files :: Standard C Implementation
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#define _LARGEFILE_SOURCE
#define _LARGEFILE_SOURCE64
#define _FILE_OFFSET_BITS 64

#include <stdio.h>
#if !defined(_WIN32_WCE)
#include <string.h>
#include <sys/stat.h>
#include <errno.h>
#else
#include <stdio.h>
#define errno GetLastError()
#endif

#include "NptConfig.h"
#include "NptUtils.h"
#include "NptFile.h"
#include "NptThreads.h"
#include "NptInterfaces.h"
#include "NptStrings.h"
//#include "NptDebug.h"
#include "NptLogging.h"

#if defined(NPT_CONFIG_HAVE_SHARE_H)
#include <share.h>
#endif

#if defined(_MSC_VER) && _MSC_VER < 1500
extern "C" {
    __int64 __cdecl _ftelli64(FILE *);
    int __cdecl _fseeki64(FILE *, __int64, int);
}
#endif

/*----------------------------------------------------------------------
|   logging
+---------------------------------------------------------------------*/
NPT_SET_LOCAL_LOGGER("neptune.stdc.file")

/*----------------------------------------------------------------------
|   compatibility wrappers
+---------------------------------------------------------------------*/
static int fopen_wrapper(FILE**      file,
                         const char* filename,
                         const char* mode)
{
#if defined(NPT_CONFIG_HAVE_FSOPEN)
    // secure with shared read access only
    *file = _fsopen(filename, mode, SH_DENYWR);
#elif defined(NPT_CONFIG_HAVE_FOPEN_S)
    return fopen_s(file, filename, mode);
#else
    *file = fopen(filename, mode);
#endif

#if defined(_WIN32_WCE)
    if (*file == NULL) return ENOENT;
#else
    if (*file == NULL) return errno;
#endif
    return 0;
}

/*----------------------------------------------------------------------
|   MapErrno
+---------------------------------------------------------------------*/
static NPT_Result
MapErrno(int err) {
    switch (err) {
      case EACCES:       return NPT_ERROR_PERMISSION_DENIED;
      case EPERM:        return NPT_ERROR_PERMISSION_DENIED;
      case ENOENT:       return NPT_ERROR_NO_SUCH_FILE;
#if defined(ENAMETOOLONG)
      case ENAMETOOLONG: return NPT_ERROR_INVALID_PARAMETERS;
#endif
      case EBUSY:        return NPT_ERROR_FILE_BUSY;
      case EROFS:        return NPT_ERROR_FILE_NOT_WRITABLE;
      case ENOTDIR:      return NPT_ERROR_FILE_NOT_DIRECTORY;
      default:           return NPT_ERROR_ERRNO(err);
    }
}

/*----------------------------------------------------------------------
|   NPT_StdcFileWrapper
+---------------------------------------------------------------------*/
class NPT_StdcFileWrapper
{
public:
    // constructors and destructor
    NPT_StdcFileWrapper(FILE* file) : m_File(file) {}
    ~NPT_StdcFileWrapper() {
        if (m_File != NULL && 
            m_File != stdin && 
            m_File != stdout && 
            m_File != stderr) {
            fclose(m_File);
        }
    }

    // methods
    FILE* GetFile() { return m_File; }

private:
    // members
    FILE* m_File;
};

typedef NPT_Reference<NPT_StdcFileWrapper> NPT_StdcFileReference;

/*----------------------------------------------------------------------
|   NPT_StdcFileStream
+---------------------------------------------------------------------*/
class NPT_StdcFileStream
{
public:
    // constructors and destructor
    NPT_StdcFileStream(NPT_StdcFileReference file) :
      m_FileReference(file) {}

    // NPT_FileInterface methods
    NPT_Result Seek(NPT_Position offset);
    NPT_Result Tell(NPT_Position& offset);
    NPT_Result Flush();

protected:
    // constructors and destructors
    virtual ~NPT_StdcFileStream() {}

    // members
    NPT_StdcFileReference m_FileReference;
};

/*----------------------------------------------------------------------
|   NPT_StdcFileStream::Seek
+---------------------------------------------------------------------*/
NPT_Result
NPT_StdcFileStream::Seek(NPT_Position offset)
{
    size_t result;

    result = NPT_fseek(m_FileReference->GetFile(), offset, SEEK_SET);
    if (result == 0) {
        return NPT_SUCCESS;
    } else {
        return NPT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   NPT_StdcFileStream::Tell
+---------------------------------------------------------------------*/
NPT_Result
NPT_StdcFileStream::Tell(NPT_Position& offset)
{
    offset = NPT_ftell(m_FileReference->GetFile());
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_StdcFileStream::Flush
+---------------------------------------------------------------------*/
NPT_Result
NPT_StdcFileStream::Flush()
{
    fflush(m_FileReference->GetFile());
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_StdcFileInputStream
+---------------------------------------------------------------------*/
class NPT_StdcFileInputStream : public NPT_InputStream,
                                private NPT_StdcFileStream
                                
{
public:
    // constructors and destructor
    NPT_StdcFileInputStream(NPT_StdcFileReference& file, NPT_LargeSize size) :
        NPT_StdcFileStream(file), m_Size(size) {}

    // NPT_InputStream methods
    NPT_Result Read(void*     buffer, 
                    NPT_Size  bytes_to_read, 
                    NPT_Size* bytes_read);
    NPT_Result Seek(NPT_Position offset) {
        return NPT_StdcFileStream::Seek(offset);
    }
    NPT_Result Tell(NPT_Position& offset) {
        return NPT_StdcFileStream::Tell(offset);
    }
    NPT_Result GetSize(NPT_LargeSize& size);
    NPT_Result GetAvailable(NPT_LargeSize& available);

private:
    // members
    NPT_LargeSize m_Size;
};

/*----------------------------------------------------------------------
|   NPT_StdcFileInputStream::Read
+---------------------------------------------------------------------*/
NPT_Result
NPT_StdcFileInputStream::Read(void*     buffer, 
                              NPT_Size  bytes_to_read, 
                              NPT_Size* bytes_read)
{
    size_t nb_read;

    // check the parameters
    if (buffer == NULL) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // read from the file
    nb_read = fread(buffer, 1, bytes_to_read, m_FileReference->GetFile());
    if (nb_read > 0) {
        if (bytes_read) *bytes_read = (NPT_Size)nb_read;
        return NPT_SUCCESS;
    } else if (feof(m_FileReference->GetFile())) {
        if (bytes_read) *bytes_read = 0;
        return NPT_ERROR_EOS;
    } else {
        if (bytes_read) *bytes_read = 0;
        return NPT_ERROR_READ_FAILED;
    }
}

/*----------------------------------------------------------------------
|   NPT_StdcFileInputStream::GetSize
+---------------------------------------------------------------------*/
NPT_Result
NPT_StdcFileInputStream::GetSize(NPT_LargeSize& size)
{
    size = m_Size;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_StdcFileInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result
NPT_StdcFileInputStream::GetAvailable(NPT_LargeSize& available)
{
    NPT_Int64 offset = NPT_ftell(m_FileReference->GetFile());
    if (offset >= 0 && (NPT_LargeSize)offset <= m_Size) {
        available = m_Size - offset;
        return NPT_SUCCESS;
    } else {
        available = 0;
        return NPT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   NPT_StdcFileOutputStream
+---------------------------------------------------------------------*/
class NPT_StdcFileOutputStream : public NPT_OutputStream,
                                 private NPT_StdcFileStream
{
public:
    // constructors and destructor
    NPT_StdcFileOutputStream(NPT_StdcFileReference& file) :
        NPT_StdcFileStream(file) {}

    // NPT_InputStream methods
    NPT_Result Write(const void* buffer, 
                     NPT_Size    bytes_to_write, 
                     NPT_Size*   bytes_written);
    NPT_Result Seek(NPT_Position offset) {
        return NPT_StdcFileStream::Seek(offset);
    }
    NPT_Result Tell(NPT_Position& offset) {
        return NPT_StdcFileStream::Tell(offset);
    }
    NPT_Result Flush() {
        return NPT_StdcFileStream::Flush();
    }
};

/*----------------------------------------------------------------------
|   NPT_StdcFileOutputStream::Write
+---------------------------------------------------------------------*/
NPT_Result
NPT_StdcFileOutputStream::Write(const void* buffer, 
                                NPT_Size    bytes_to_write, 
                                NPT_Size*   bytes_written)
{
    size_t nb_written;

    nb_written = fwrite(buffer, 1, bytes_to_write, m_FileReference->GetFile());

    if (nb_written > 0) {
        if (bytes_written) *bytes_written = (NPT_Size)nb_written;
        return NPT_SUCCESS;
    } else {
        if (bytes_written) *bytes_written = 0;
        return NPT_ERROR_WRITE_FAILED;
    }
}

/*----------------------------------------------------------------------
|   NPT_StdcFile
+---------------------------------------------------------------------*/
class NPT_StdcFile: public NPT_FileInterface
{
public:
    // constructors and destructor
    NPT_StdcFile(NPT_File& delegator);
   ~NPT_StdcFile();

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
    NPT_StdcFileReference m_FileReference;
};

/*----------------------------------------------------------------------
|   NPT_StdcFile::NPT_StdcFile
+---------------------------------------------------------------------*/
NPT_StdcFile::NPT_StdcFile(NPT_File& delegator) :
    m_Delegator(delegator),
    m_Mode(0)
{
}

/*----------------------------------------------------------------------
|   NPT_StdcFile::~NPT_StdcFile
+---------------------------------------------------------------------*/
NPT_StdcFile::~NPT_StdcFile()
{
    Close();
}

/*----------------------------------------------------------------------
|   NPT_StdcFile::Open
+---------------------------------------------------------------------*/
NPT_Result
NPT_StdcFile::Open(NPT_File::OpenMode mode)
{
    FILE* file = NULL;
    
    // check if we're already open
    if (!m_FileReference.IsNull()) {
        return NPT_ERROR_FILE_ALREADY_OPEN;
    }

    // store the mode
    m_Mode = mode;

    // check for special names
    const char* name = (const char*)m_Delegator.GetPath();
    if (NPT_StringsEqual(name, NPT_FILE_STANDARD_INPUT)) {
        file = stdin;
    } else if (NPT_StringsEqual(name, NPT_FILE_STANDARD_OUTPUT)) {
        file = stdout;
    } else if (NPT_StringsEqual(name, NPT_FILE_STANDARD_ERROR)) {
        file = stderr;
    } else {
        // compute mode
        const char* fmode = "";
        if (mode & NPT_FILE_OPEN_MODE_WRITE) {
            if (mode & NPT_FILE_OPEN_MODE_CREATE) {
                if (mode & NPT_FILE_OPEN_MODE_TRUNCATE) {
                    /* write, read, create, truncate */
                    fmode = "w+b";
                } else {
                    /* write, read, create */
                    fmode = "a+b";
                }
            } else {
                if (mode & NPT_FILE_OPEN_MODE_TRUNCATE) {
                    /* write, read, truncate */
                    fmode = "w+b";
                } else {
                    /* write, read */
                    fmode = "r+b";
                }
            }
        } else {
            /* read only */
            fmode = "rb";
        }

        // open the file
        int open_result = fopen_wrapper(&file, name, fmode);

        // test the result of the open
        if (open_result != 0) return MapErrno(errno);
    }

    // unbuffer the file if needed 
    if ((mode & NPT_FILE_OPEN_MODE_UNBUFFERED) && file != NULL) {
#if !defined(_WIN32_WCE)
        setvbuf(file, NULL, _IONBF, 0);
#endif
    }   
    
    // create a reference to the FILE object
    m_FileReference = new NPT_StdcFileWrapper(file);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_StdcFile::Close
+---------------------------------------------------------------------*/
NPT_Result
NPT_StdcFile::Close()
{
    // release the file reference
    m_FileReference = NULL;

    // reset the mode
    m_Mode = 0;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_StdcFile::GetSize
+---------------------------------------------------------------------*/
NPT_Result 
NPT_StdcFile::GetSize(NPT_LargeSize& size)
{
    // default value
    size = 0;

    // check that the file is open
    if (m_FileReference.IsNull()) return NPT_ERROR_FILE_NOT_OPEN;

    // get the size from the info (call GetInfo() in case it has not
    // yet been called)
    NPT_FileInfo info;
    NPT_CHECK_FATAL(m_Delegator.GetInfo(info));
    size = info.m_Size;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_StdcFile::GetInputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_StdcFile::GetInputStream(NPT_InputStreamReference& stream)
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
    NPT_LargeSize size = 0;
    GetSize(size);
    stream = new NPT_StdcFileInputStream(m_FileReference, size);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_StdcFile::GetOutputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_StdcFile::GetOutputStream(NPT_OutputStreamReference& stream)
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
    stream = new NPT_StdcFileOutputStream(m_FileReference);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_File::NPT_File
+---------------------------------------------------------------------*/
NPT_File::NPT_File(const char* path) :
    m_Path(path)
{
    m_Delegate = new NPT_StdcFile(*this);
    
    if (NPT_StringsEqual(path, NPT_FILE_STANDARD_INPUT)  ||
        NPT_StringsEqual(path, NPT_FILE_STANDARD_OUTPUT) ||
        NPT_StringsEqual(path, NPT_FILE_STANDARD_ERROR)) {
        m_Info.m_Type = NPT_FileInfo::FILE_TYPE_SPECIAL;
    } 
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
        m_Delegate = new NPT_StdcFile(*this);
    }
    return *this;
}

