/*
 *  Neptune - Files :: XBMC Implementation
 *
 *  Copyright (c) 2002-2008, Axiomatic Systems, LLC.
 *  All rights reserved.
 *
 *  SPDX-License-Identifier: BSD-3-Clause
 *  See LICENSES/README.md for more information.
 */

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "File.h"
#include "FileFactory.h"
#include "PasswordManager.h"
#include "URL.h"
#include "utils/URIUtils.h"

#include <limits>

#include <Neptune/Source/Core/NptDebug.h>
#include <Neptune/Source/Core/NptFile.h>
#include <Neptune/Source/Core/NptStrings.h>
#include <Neptune/Source/Core/NptUtils.h>

#ifdef TARGET_WINDOWS
#define S_IWUSR _S_IWRITE
#define S_ISDIR(m) ((m & _S_IFDIR) != 0)
#define S_ISREG(m) ((m & _S_IFREG) != 0)
#endif

using namespace XFILE;

typedef NPT_Reference<IFile> NPT_XbmcFileReference;

/*----------------------------------------------------------------------
|   NPT_XbmcFileStream
+---------------------------------------------------------------------*/
class NPT_XbmcFileStream
{
public:
    // constructors and destructor
  explicit NPT_XbmcFileStream(const NPT_XbmcFileReference& file) : m_FileReference(file) {}

    // NPT_FileInterface methods
    NPT_Result Seek(NPT_Position offset);
    NPT_Result Tell(NPT_Position& offset);
    NPT_Result Flush();

protected:
    // constructors and destructors
    virtual ~NPT_XbmcFileStream() = default;

    // members
    NPT_XbmcFileReference m_FileReference;
};

/*----------------------------------------------------------------------
|   NPT_XbmcFileStream::Seek
+---------------------------------------------------------------------*/
NPT_Result
NPT_XbmcFileStream::Seek(NPT_Position offset)
{
    int64_t result;

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
    int64_t result = m_FileReference->GetPosition();
    if (result >= 0) {
        offset = (NPT_Position)result;
        return NPT_SUCCESS;
    } else {
        return NPT_FAILURE;
    }
}

/*----------------------------------------------------------------------
|   NPT_XbmcFileStream::Flush
+---------------------------------------------------------------------*/
NPT_Result
NPT_XbmcFileStream::Flush()
{
    m_FileReference->Flush();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_XbmcFileInputStream
+---------------------------------------------------------------------*/
class NPT_XbmcFileInputStream : public NPT_InputStream,
                                private NPT_XbmcFileStream

{
public:
    // constructors and destructor
    explicit NPT_XbmcFileInputStream(NPT_XbmcFileReference& file) :
        NPT_XbmcFileStream(file) {}

    // NPT_InputStream methods
    NPT_Result Read(void*     buffer,
                    NPT_Size  bytes_to_read,
                    NPT_Size* bytes_read) override;
    NPT_Result Seek(NPT_Position offset) override {
        return NPT_XbmcFileStream::Seek(offset);
    }
    NPT_Result Tell(NPT_Position& offset) override {
        return NPT_XbmcFileStream::Tell(offset);
    }
    NPT_Result GetSize(NPT_LargeSize& size) override;
    NPT_Result GetAvailable(NPT_LargeSize& available) override;
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
    size = m_FileReference->GetLength();
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_XbmcFileInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result
NPT_XbmcFileInputStream::GetAvailable(NPT_LargeSize& available)
{
    int64_t offset = m_FileReference->GetPosition();
    NPT_LargeSize size = 0;

    if (NPT_SUCCEEDED(GetSize(size)) && offset >= 0 && (NPT_LargeSize)offset <= size) {
        available = size - offset;
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
    explicit NPT_XbmcFileOutputStream(NPT_XbmcFileReference& file) :
        NPT_XbmcFileStream(file) {}

    // NPT_OutputStream methods
    NPT_Result Write(const void* buffer,
                     NPT_Size    bytes_to_write,
                     NPT_Size*   bytes_written) override;
    NPT_Result Seek(NPT_Position offset) override {
        return NPT_XbmcFileStream::Seek(offset);
    }
    NPT_Result Tell(NPT_Position& offset) override {
        return NPT_XbmcFileStream::Tell(offset);
    }
    NPT_Result Flush() override {
        return NPT_XbmcFileStream::Flush();
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
    explicit NPT_XbmcFile(NPT_File& delegator);
   ~NPT_XbmcFile() override;

    // NPT_FileInterface methods
    NPT_Result Open(OpenMode mode) override;
    NPT_Result Close() override;
    NPT_Result GetInputStream(NPT_InputStreamReference& stream) override;
    NPT_Result GetOutputStream(NPT_OutputStreamReference& stream) override;

private:
    // members
    NPT_File&             m_Delegator;
    OpenMode m_Mode = 0;
    NPT_XbmcFileReference m_FileReference;
};

/*----------------------------------------------------------------------
|   NPT_XbmcFile::NPT_XbmcFile
+---------------------------------------------------------------------*/
NPT_XbmcFile::NPT_XbmcFile(NPT_File& delegator) : m_Delegator(delegator)
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
        CURL url(URIUtils::SubstitutePath(name));

        if (CPasswordManager::GetInstance().IsURLSupported(url) && url.GetUserName().empty())
          CPasswordManager::GetInstance().AuthenticateURL(url);

        // compute mode
        if (mode & NPT_FILE_OPEN_MODE_WRITE)
          result = file->OpenForWrite(url, (mode & NPT_FILE_OPEN_MODE_TRUNCATE) ? true : false);
        else
          result = file->Open(url);

        if (!result) return NPT_ERROR_NO_SUCH_FILE;
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
    stream = new NPT_XbmcFileInputStream(m_FileReference);

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
|   NPT_FilePath::Separator
+---------------------------------------------------------------------*/
const char* const NPT_FilePath::Separator = "/";

/*----------------------------------------------------------------------
|   NPT_File::NPT_File
+---------------------------------------------------------------------*/
NPT_File::NPT_File(const char* path) : m_Path(path)
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
        m_Delegate = new NPT_XbmcFile(*this);
    }
    return *this;
}

/*----------------------------------------------------------------------
|   NPT_File::GetRoots
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetRoots(NPT_List<NPT_String>& roots)
{
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   NPT_File::CreateDir
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::CreateDir(const char* path)
{
    return NPT_ERROR_PERMISSION_DENIED;
}

/*----------------------------------------------------------------------
|   NPT_File::RemoveFile
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::RemoveFile(const char* path)
{
    return NPT_ERROR_PERMISSION_DENIED;
}

/*----------------------------------------------------------------------
|   NPT_File::RemoveDir
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::RemoveDir(const char* path)
{
    return NPT_ERROR_PERMISSION_DENIED;
}

/*----------------------------------------------------------------------
|   NPT_File::Rename
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::Rename(const char* from_path, const char* to_path)
{
    return NPT_ERROR_PERMISSION_DENIED;
}

/*----------------------------------------------------------------------
|   NPT_File::ListDir
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::ListDir(const char*           path,
                  NPT_List<NPT_String>& entries,
                  NPT_Ordinal           start /* = 0 */,
                  NPT_Cardinal          max   /* = 0 */)
{
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   NPT_File::GetWorkingDir
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetWorkingDir(NPT_String& path)
{
    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   NPT_File::GetInfo
+---------------------------------------------------------------------*/
NPT_Result
NPT_File::GetInfo(const char* path, NPT_FileInfo* info)
{
  struct __stat64 stat_buffer = {};
  int result;

  if (!info)
    return NPT_FAILURE;

  *info = NPT_FileInfo();

  result = CFile::Stat(path, &stat_buffer);
  if (result != 0)
    return MapErrno(errno);
  if (info)
  {
    info->m_Size = stat_buffer.st_size;
    if (S_ISREG(stat_buffer.st_mode))
    {
      info->m_Type = NPT_FileInfo::FILE_TYPE_REGULAR;
    }
    else if (S_ISDIR(stat_buffer.st_mode))
    {
      info->m_Type = NPT_FileInfo::FILE_TYPE_DIRECTORY;
    }
    else
    {
      info->m_Type = NPT_FileInfo::FILE_TYPE_OTHER;
    }
    info->m_AttributesMask &= NPT_FILE_ATTRIBUTE_READ_ONLY;
    if ((stat_buffer.st_mode & S_IWUSR) == 0)
    {
      info->m_Attributes &= NPT_FILE_ATTRIBUTE_READ_ONLY;
    }
    info->m_CreationTime.SetSeconds(0);
    info->m_ModificationTime.SetSeconds(stat_buffer.st_mtime);
  }

    return NPT_SUCCESS;
}

