/*****************************************************************
|
|      Neptune - File Byte Stream :: Standard C Implementation
|
|      (c) 2001-2003 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <malloc.h>

#include <kernel.h>
#include <psptypes.h>
#include <psperror.h>
#include <mediaman.h>
#include <umderror.h>
#include <fatms.h>

#include "NptFile.h"

/*----------------------------------------------------------------------
|       NPT_PSPFileWrapper
+---------------------------------------------------------------------*/
class NPT_PSPFileWrapper
{
public:
    // constructors and destructor
    NPT_PSPFileWrapper(SceUID file) : m_File(file) {}
    ~NPT_PSPFileWrapper() {
        if (m_File >= 0) {
            sceIoClose(m_File);
        }
    }

    // methods
    SceUID GetFile() { return m_File; }

private:
    // members
    SceUID m_File;
};

typedef NPT_Reference<NPT_PSPFileWrapper> NPT_PSPFileReference;

/*----------------------------------------------------------------------
|       NPT_PSPFileStream
+---------------------------------------------------------------------*/
class NPT_PSPFileStream
{
public:
    // constructors and destructor
    NPT_PSPFileStream(NPT_PSPFileReference file) :
      m_FileReference(file), m_Position(0) {}

    // NPT_FileInterface methods
    NPT_Result Seek(NPT_Offset offset);
    NPT_Result Tell(NPT_Offset& offset);

protected:
    // constructors and destructors
    virtual ~NPT_PSPFileStream() {}

    // members
    NPT_PSPFileReference m_FileReference;
    SceOff               m_Position;
};

/*----------------------------------------------------------------------
|       NPT_PSPFileStream::Seek
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPFileStream::Seek(NPT_Offset offset)
{
    SceOff result;

    result = sceIoLseek(m_FileReference->GetFile(), offset, SEEK_SET);
    if (result >= 0) {
        m_Position = offset;
        return NPT_SUCCESS;
    } else {
        return NPT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|       NPT_PSPFileStream::Tell
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPFileStream::Tell(NPT_Offset& offset)
{
    offset = m_Position;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPFileInputStream
+---------------------------------------------------------------------*/
class NPT_PSPFileInputStream : public NPT_InputStream,
                               private NPT_PSPFileStream
                                
{
public:
    // constructors and destructor
    NPT_PSPFileInputStream(NPT_PSPFileReference& file, NPT_Size size) :
        NPT_PSPFileStream(file), m_Size(size) {}

    // NPT_InputStream methods
    NPT_Result Read(void*     buffer, 
                    NPT_Size  bytes_to_read, 
                    NPT_Size* bytes_read);
    NPT_Result Seek(NPT_Offset offset) {
        return NPT_PSPFileStream::Seek(offset);
    }
    NPT_Result Tell(NPT_Offset& offset) {
        return NPT_PSPFileStream::Tell(offset);
    }
    NPT_Result GetSize(NPT_Size& size);
    NPT_Result GetAvailable(NPT_Size& available);

private:
    // members
    NPT_Size m_Size;
};

/*----------------------------------------------------------------------
|       NPT_PSPFileInputStream::Read
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPFileInputStream::Read(void*     buffer, 
                             NPT_Size  bytes_to_read, 
                             NPT_Size* bytes_read)
{
    size_t nb_read;

    if (buffer == NULL) {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    nb_read = sceIoRead(m_FileReference->GetFile(), (SceChar8*)buffer, bytes_to_read);

    if (nb_read > 0) {
        if (bytes_read) *bytes_read = (NPT_Size)nb_read;
        m_Position += nb_read;
        return NPT_SUCCESS;
    } else { 
        if (bytes_read) *bytes_read = 0;
        return NPT_ERROR_EOS;
    }
}

/*----------------------------------------------------------------------
|       NPT_PSPFileInputStream::GetSize
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPFileInputStream::GetSize(NPT_Size& size)
{
    size = m_Size;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPFileInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPFileInputStream::GetAvailable(NPT_Size& available)
{
    long offset = m_Position;
    if (offset >= 0 && (NPT_Size)offset <= m_Size) {
        available = m_Size = offset;
        return NPT_SUCCESS;
    } else {
        available = 0;
        return NPT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|       NPT_PSPFileOutputStream
+---------------------------------------------------------------------*/
class NPT_PSPFileOutputStream : public NPT_OutputStream,
                                private NPT_PSPFileStream
{
public:
    // constructors and destructor
    NPT_PSPFileOutputStream(NPT_PSPFileReference& file) :
        NPT_PSPFileStream(file) {}

    // NPT_InputStream methods
    NPT_Result Write(const void* buffer, 
                     NPT_Size    bytes_to_write, 
                     NPT_Size*   bytes_written);
    NPT_Result Seek(NPT_Offset offset) {
        return NPT_PSPFileStream::Seek(offset);
    }
    NPT_Result Tell(NPT_Offset& offset) {
        return NPT_PSPFileStream::Tell(offset);
    }
};

/*----------------------------------------------------------------------
|       NPT_PSPFileOutputStream::Write
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPFileOutputStream::Write(const void* buffer, 
                               NPT_Size    bytes_to_write, 
                               NPT_Size*   bytes_written)
{
    size_t nb_written;

    nb_written = sceIoWrite(m_FileReference->GetFile(), (SceChar8*)buffer, bytes_to_write);

    if (nb_written > 0) {
        if (bytes_written) *bytes_written = (NPT_Size)nb_written;
        m_Position += nb_written;
        return NPT_SUCCESS;
    } else {
        if (bytes_written) *bytes_written = 0;
        return NPT_ERROR_WRITE_FAILED;
    }
}

/*----------------------------------------------------------------------
|       NPT_PSPFile
+---------------------------------------------------------------------*/
class NPT_PSPFile: public NPT_FileInterface
{
public:
    // constructors and destructor
    NPT_PSPFile(const char* name);
   ~NPT_PSPFile();

    // NPT_FileInterface methods
    NPT_Result Open(OpenMode mode);
    NPT_Result Close();
    NPT_Result GetSize(NPT_Size& size);
    NPT_Result GetInputStream(NPT_InputStreamReference& stream);
    NPT_Result GetOutputStream(NPT_OutputStreamReference& stream);

private:
    // members
    NPT_String            m_Name;
    OpenMode              m_Mode;
    NPT_PSPFileReference  m_FileReference;
    NPT_Size              m_Size;
};

/*----------------------------------------------------------------------
|       NPT_PSPFile::NPT_PSPFile
+---------------------------------------------------------------------*/
NPT_PSPFile::NPT_PSPFile(const char* name) :
    m_Name(name),
    m_Mode(0),
    m_Size(0)
{
}

/*----------------------------------------------------------------------
|       NPT_PSPFile::~NPT_PSPFile
+---------------------------------------------------------------------*/
NPT_PSPFile::~NPT_PSPFile()
{
    Close();
}

/*----------------------------------------------------------------------
|       NPT_PSPFile::Open
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPFile::Open(NPT_File::OpenMode mode)
{
    SceUID file = -1;

    // check if we're already open
    if (!m_FileReference.IsNull()) {
        return NPT_ERROR_FILE_ALREADY_OPEN;
    }

    // store the mode
    m_Mode = mode;

    // check for special names
    const char* name = (const char*)m_Name;
    int flags = 0;
    SceMode perms = 0; // files opened/created on the PSP require certain permissions

    /* compute mode */
    if (mode & NPT_FILE_BYTE_STREAM_MODE_WRITE) {
        perms = 0777; // since we're possibly creating a file, give it full permissions
        if (mode & NPT_FILE_BYTE_STREAM_MODE_CREATE) {
            if (mode & NPT_FILE_BYTE_STREAM_MODE_TRUNCATE) {
                /* write, read, create, truncate */
                flags = SCE_O_RDWR | SCE_O_CREAT | SCE_O_TRUNC;
            } else {
                /* write, read, create */
                flags = SCE_O_RDWR | SCE_O_CREAT | SCE_O_APPEND;
            }
        } else {
            if (mode & NPT_FILE_BYTE_STREAM_MODE_TRUNCATE) {
                /* write, read, truncate */
                flags = SCE_O_RDWR | SCE_O_CREAT | SCE_O_TRUNC;
            } else {
                /* write, read */
                flags = SCE_O_RDWR;
            }
        }
    } else {
        /* read only */
        flags = SCE_O_RDONLY;
    }

    // open the file
    file = sceIoOpen(name, flags, perms);

    // test the result of the open
    if (file < 0) {
        if (file == (SceUID)SCE_ERROR_ERRNO_ENOENT ) {
            return NPT_ERROR_NO_SUCH_FILE;
        } else if (file == (SceUID)SCE_ERROR_ERRNO_EACCESS ) {
            return NPT_ERROR_PERMISSION_DENIED;
        } else {
            return NPT_FAILURE;
        }
    } else {
        // get the size
        SceOff off;
        off = sceIoLseek(file, 0, SCE_SEEK_END);
        if (off >= 0) {
            m_Size = off;
            sceIoLseek(file, 0, SCE_SEEK_SET);
        }
    }

    // create a reference to the FILE object
    m_FileReference = new NPT_PSPileWrapper(file);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPile::Close
+---------------------------------------------------------------------*/
NPT_Result
NPT_PSPile::Close()
{
    // release the file reference
    m_FileReference = NULL;

    // reset the mode
    m_Mode = 0;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPile::GetSize
+---------------------------------------------------------------------*/
NPT_Result 
NPT_PSPile::GetSize(NPT_Size& size)
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
|       NPT_PSPile::GetInputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_PSPile::GetInputStream(NPT_InputStreamReference& stream)
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
    stream = new NPT_PSPileInputStream(m_FileReference, m_Size);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_PSPile::GetOutputStream
+---------------------------------------------------------------------*/
NPT_Result 
NPT_PSPile::GetOutputStream(NPT_OutputStreamReference& stream)
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
    stream = new NPT_PSPileOutputStream(m_FileReference);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|       NPT_File::NPT_File
+---------------------------------------------------------------------*/
NPT_File::NPT_File(const char* name)
{
    m_Delegate = new NPT_PSPile(name);
}












