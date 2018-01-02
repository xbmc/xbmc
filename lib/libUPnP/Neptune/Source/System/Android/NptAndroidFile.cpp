/*****************************************************************
|
|   Neptune - Files :: Android Implementation
|
|   (c) 2001-2016 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptConfig.h"
#include "NptUtils.h"
#include "NptFile.h"
#include "NptThreads.h"
#include "NptInterfaces.h"
#include "NptStrings.h"
#include "NptDebug.h"

#include <errno.h>
#include <unistd.h>
#include <fcntl.h>

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
|   NPT_AndroidFileWrapper
+---------------------------------------------------------------------*/
class NPT_AndroidFileWrapper
{
public:
    // constructors and destructor
    NPT_AndroidFileWrapper(int fd, const char* name) : m_FD(fd), m_Position(0), m_Name(name) {}
    ~NPT_AndroidFileWrapper() {
        if (m_FD >= 0 &&
            m_FD != STDIN_FILENO &&
            m_FD != STDOUT_FILENO &&
            m_FD != STDERR_FILENO) {
            close(m_FD);
        }
    }

    // members
    int          m_FD;
    NPT_Position m_Position;
    NPT_String   m_Name;
};

typedef NPT_Reference<NPT_AndroidFileWrapper> NPT_AndroidFileReference;

/*----------------------------------------------------------------------
|   NPT_AndroidFileStream
+---------------------------------------------------------------------*/
class NPT_AndroidFileStream
{
public:
    // constructors and destructor
    NPT_AndroidFileStream(NPT_AndroidFileReference file) :
      m_FileReference(file) {}

    // NPT_FileInterface methods
    NPT_Result Seek(NPT_Position offset);
    NPT_Result Tell(NPT_Position& offset);
    NPT_Result Flush();

protected:
    // constructors and destructors
    virtual ~NPT_AndroidFileStream() {}

    // members
    NPT_AndroidFileReference m_FileReference;
};

/*----------------------------------------------------------------------
|   NPT_AndroidFileStream::Seek
+---------------------------------------------------------------------*/
NPT_Result
NPT_AndroidFileStream::Seek(NPT_Position offset)
{
    off64_t result = lseek64(m_FileReference->m_FD, offset, SEEK_SET);
    if (result < 0) {
        return MapErrno(errno);
    }

    m_FileReference->m_Position = offset;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_AndroidFileStream::Tell
+---------------------------------------------------------------------*/
NPT_Result
NPT_AndroidFileStream::Tell(NPT_Position& offset)
{
    offset = m_FileReference->m_Position;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_AndroidFileStream::Flush
+---------------------------------------------------------------------*/
NPT_Result
NPT_AndroidFileStream::Flush()
{
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_AndroidFileInputStream
+---------------------------------------------------------------------*/
class NPT_AndroidFileInputStream : public NPT_InputStream,
                                private NPT_AndroidFileStream

{
public:
    // constructors and destructor
    NPT_AndroidFileInputStream(NPT_AndroidFileReference& file) :
        NPT_AndroidFileStream(file) {}

    // NPT_InputStream methods
    NPT_Result Read(void*     buffer,
                    NPT_Size  bytes_to_read,
                    NPT_Size* bytes_read);
    NPT_Result Seek(NPT_Position offset) {
        return NPT_AndroidFileStream::Seek(offset);
    }
    NPT_Result Tell(NPT_Position& offset) {
        return NPT_AndroidFileStream::Tell(offset);
    }
    NPT_Result GetSize(NPT_LargeSize& size);
    NPT_Result GetAvailable(NPT_LargeSize& available);
};

/*----------------------------------------------------------------------
|   NPT_AndroidFileInputStream::Read
+---------------------------------------------------------------------*/
NPT_Result
NPT_AndroidFileInputStream::Read(void*     buffer,
                                 NPT_Size  bytes_to_read,
                                 NPT_Size* bytes_read)
{
    // check the parameters
    if (buffer == NULL) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    // read from the file
    ssize_t nb_read = read(m_FileReference->m_FD, buffer, bytes_to_read);
    if (nb_read > 0) {
        if (bytes_read) *bytes_read = (NPT_Size)nb_read;
        m_FileReference->m_Position += nb_read;
        return NPT_SUCCESS;
    } else if (nb_read == 0) {
        if (bytes_read) *bytes_read = 0;
        return NPT_ERROR_EOS;
    } else {
        if (bytes_read) *bytes_read = 0;
        return MapErrno(errno);
    }
}

/*----------------------------------------------------------------------
|   NPT_AndroidFileInputStream::GetSize
+---------------------------------------------------------------------*/
NPT_Result
NPT_AndroidFileInputStream::GetSize(NPT_LargeSize& size)
{
    NPT_FileInfo file_info;
    NPT_Result result = NPT_File::GetInfo(m_FileReference->m_Name, &file_info);
    if (NPT_FAILED(result)) return result;
    size = file_info.m_Size;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_AndroidFileInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result
NPT_AndroidFileInputStream::GetAvailable(NPT_LargeSize& available)
{
    NPT_LargeSize size = 0;

    if (NPT_SUCCEEDED(GetSize(size)) && m_FileReference->m_Position <= size) {
        available = size - m_FileReference->m_Position;
        return NPT_SUCCESS;
    } else {
        available = 0;
        return NPT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   NPT_AndroidFileOutputStream
+---------------------------------------------------------------------*/
class NPT_AndroidFileOutputStream : public NPT_OutputStream,
                                    private NPT_AndroidFileStream
{
public:
    // constructors and destructor
    NPT_AndroidFileOutputStream(NPT_AndroidFileReference& file) :
        NPT_AndroidFileStream(file) {}

    // NPT_InputStream methods
    NPT_Result Write(const void* buffer,
                     NPT_Size    bytes_to_write,
                     NPT_Size*   bytes_written);
    NPT_Result Seek(NPT_Position offset) {
        return NPT_AndroidFileStream::Seek(offset);
    }
    NPT_Result Tell(NPT_Position& offset) {
        return NPT_AndroidFileStream::Tell(offset);
    }
    NPT_Result Flush() {
        return NPT_AndroidFileStream::Flush();
    }
};

/*----------------------------------------------------------------------
|   NPT_AndroidFileOutputStream::Write
+---------------------------------------------------------------------*/
NPT_Result
NPT_AndroidFileOutputStream::Write(const void* buffer,
                                   NPT_Size    bytes_to_write,
                                   NPT_Size*   bytes_written)
{
    if (bytes_to_write == 0) {
        if (bytes_written) *bytes_written = 0;
        return NPT_SUCCESS;
    }

    ssize_t nb_written = write(m_FileReference->m_FD, buffer, bytes_to_write);

    if (nb_written > 0) {
        if (bytes_written) *bytes_written = (NPT_Size)nb_written;
        m_FileReference->m_Position += nb_written;
        return NPT_SUCCESS;
    } else {
        if (bytes_written) *bytes_written = 0;
        return MapErrno(errno);
    }
}

/*----------------------------------------------------------------------
|   NPT_AndroidFile
+---------------------------------------------------------------------*/
class NPT_AndroidFile: public NPT_FileInterface
{
public:
    // constructors and destructor
    NPT_AndroidFile(NPT_File& delegator);
   ~NPT_AndroidFile();

    // NPT_FileInterface methods
    NPT_Result Open(OpenMode mode);
    NPT_Result Close();
    NPT_Result GetInputStream(NPT_InputStreamReference& stream);
    NPT_Result GetOutputStream(NPT_OutputStreamReference& stream);

private:
    // members
    NPT_File&                m_Delegator;
    OpenMode                 m_Mode;
    NPT_AndroidFileReference m_FileReference;
};

/*----------------------------------------------------------------------
|   NPT_AndroidFile::NPT_AndroidFile
+---------------------------------------------------------------------*/
NPT_AndroidFile::NPT_AndroidFile(NPT_File& delegator) :
    m_Delegator(delegator),
    m_Mode(0)
{
}

/*----------------------------------------------------------------------
|   NPT_AndroidFile::~NPT_AndroidFile
+---------------------------------------------------------------------*/
NPT_AndroidFile::~NPT_AndroidFile()
{
    Close();
}

/*----------------------------------------------------------------------
|   NPT_AndroidFile::Open
+---------------------------------------------------------------------*/
NPT_Result
NPT_AndroidFile::Open(NPT_File::OpenMode mode)
{
    // check if we're already open
    if (!m_FileReference.IsNull()) {
        return NPT_ERROR_FILE_ALREADY_OPEN;
    }

    // store the mode
    m_Mode = mode;

    // check for special names
    const char* name = (const char*)m_Delegator.GetPath();
    int fd = -1;
    if (NPT_StringsEqual(name, NPT_FILE_STANDARD_INPUT)) {
        fd = STDIN_FILENO;
    } else if (NPT_StringsEqual(name, NPT_FILE_STANDARD_OUTPUT)) {
        fd = STDOUT_FILENO;
    } else if (NPT_StringsEqual(name, NPT_FILE_STANDARD_ERROR)) {
        fd = STDERR_FILENO;
    } else {
        int open_flags   = 0;
        int create_perm  = 0;

        if (mode & NPT_FILE_OPEN_MODE_WRITE) {
            if (mode & NPT_FILE_OPEN_MODE_READ) {
                open_flags |= O_RDWR;
            } else {
                open_flags |= O_WRONLY;
            }
            if (mode & NPT_FILE_OPEN_MODE_APPEND) {
                open_flags |= O_APPEND;
            }
            if (mode & NPT_FILE_OPEN_MODE_CREATE) {
                open_flags |= O_CREAT;
                create_perm = 0666;
            }
            if (mode & NPT_FILE_OPEN_MODE_TRUNCATE) {
                open_flags |= O_TRUNC;
            }
        } else {
            open_flags |= O_RDONLY;
        }

        fd = open(name, open_flags, create_perm);

        // test the result of the open
        if (fd < 0) return MapErrno(errno);
    }

    // create a reference to the file descriptor
    m_FileReference = new NPT_AndroidFileWrapper(fd, name);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_AndroidFile::Close
+---------------------------------------------------------------------*/
NPT_Result
NPT_AndroidFile::Close()
{
    // release the file reference
    m_FileReference = NULL;

    // reset the mode
    m_Mode = 0;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_AndroidFile::GetInputStream
+---------------------------------------------------------------------*/
NPT_Result
NPT_AndroidFile::GetInputStream(NPT_InputStreamReference& stream)
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
    stream = new NPT_AndroidFileInputStream(m_FileReference);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_AndroidFile::GetOutputStream
+---------------------------------------------------------------------*/
NPT_Result
NPT_AndroidFile::GetOutputStream(NPT_OutputStreamReference& stream)
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
    stream = new NPT_AndroidFileOutputStream(m_FileReference);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_File::NPT_File
+---------------------------------------------------------------------*/
NPT_File::NPT_File(const char* path) : m_Path(path), m_IsSpecial(false)
{
    m_Delegate = new NPT_AndroidFile(*this);

    if (NPT_StringsEqual(path, NPT_FILE_STANDARD_INPUT)  ||
        NPT_StringsEqual(path, NPT_FILE_STANDARD_OUTPUT) ||
        NPT_StringsEqual(path, NPT_FILE_STANDARD_ERROR)) {
        m_IsSpecial = true;
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
        m_Path      = file.m_Path;
        m_IsSpecial = file.m_IsSpecial;
        m_Delegate  = new NPT_AndroidFile(*this);
    }
    return *this;
}
