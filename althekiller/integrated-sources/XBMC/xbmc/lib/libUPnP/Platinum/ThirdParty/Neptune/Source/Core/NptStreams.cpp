/*****************************************************************
|
|   Neptune - Byte Streams
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptStreams.h"
#include "NptUtils.h"
#include "NptConstants.h"
#include "NptStrings.h"
#include "NptDebug.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const NPT_Size NPT_INPUT_STREAM_LOAD_DEFAULT_READ_CHUNK = 4096;

/*----------------------------------------------------------------------
|   NPT_InputStream::Load
+---------------------------------------------------------------------*/
NPT_Result
NPT_InputStream::Load(NPT_DataBuffer& buffer, NPT_Size max_read /* = 0 */)
{
    NPT_Result result;
    NPT_Size   total_bytes_read;

    // reset the buffer
    buffer.SetDataSize(0);

    // try to get the stream size
    NPT_Size size;
    if (NPT_SUCCEEDED(GetSize(size))) { 
        // make sure we don't read more than max_read
        if (max_read && max_read < size) size = max_read;
    } else {
        size = max_read;
    } 

    // pre-allocate the buffer
    if (size) NPT_CHECK(buffer.Reserve(size));

    // read the data from the file
    total_bytes_read = 0;
    do {
        NPT_Size  available = 0;
        NPT_Size  bytes_to_read;
        NPT_Size  bytes_read;
        NPT_Byte* data;

        // check if we know how much data is available
        result = GetAvailable(available);
        if (NPT_SUCCEEDED(result) && available) {
            // we know how much is available
            bytes_to_read = available;
        } else {
            bytes_to_read = NPT_INPUT_STREAM_LOAD_DEFAULT_READ_CHUNK;
        }

        // make sure we don't read more than what was asked
        if (size != 0 && total_bytes_read+bytes_to_read>size) {
            bytes_to_read = size-total_bytes_read;
        }

        // stop if we've read everything
        if (bytes_to_read == 0) break;

        // ensure that the buffer has enough space
        NPT_CHECK(buffer.Reserve(total_bytes_read+bytes_to_read));

        // read the data
        data = buffer.UseData()+total_bytes_read;
        result = Read((void*)data, bytes_to_read, &bytes_read);
        if (NPT_SUCCEEDED(result) && bytes_read != 0) {
            total_bytes_read += bytes_read;
            buffer.SetDataSize(total_bytes_read);
        }
    } while(NPT_SUCCEEDED(result) && (size==0 || total_bytes_read < size));

    if (result == NPT_ERROR_EOS) {
        return NPT_SUCCESS;
    } else {
        return result;
    }
}

/*----------------------------------------------------------------------
|   NPT_InputStream::ReadFully
+---------------------------------------------------------------------*/
NPT_Result
NPT_InputStream::ReadFully(void* buffer, NPT_Size bytes_to_read)
{
    // shortcut
    if (bytes_to_read == 0) return NPT_SUCCESS;

    // read until failure
    NPT_Size bytes_read;
    while (bytes_to_read) {
        NPT_Result result = Read(buffer, bytes_to_read, &bytes_read);
        if (NPT_FAILED(result)) return result;
        if (bytes_read == 0) return NPT_ERROR_INTERNAL;
        NPT_ASSERT(bytes_read <= bytes_to_read);
        bytes_to_read -= bytes_read;
        buffer = (void*)(((NPT_Byte*)buffer)+bytes_read);
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_InputStream::Skip
+---------------------------------------------------------------------*/
NPT_Result
NPT_InputStream::Skip(NPT_Size count)
{
    // get the current location
    NPT_Position position;
    NPT_CHECK(Tell(position));

    // seek ahead
    return Seek(position+count);
}

/*----------------------------------------------------------------------
|   NPT_OutputStream::WriteFully
+---------------------------------------------------------------------*/
NPT_Result
NPT_OutputStream::WriteFully(const void* buffer, NPT_Size bytes_to_write)
{
    // shortcut
    if (bytes_to_write == 0) return NPT_SUCCESS;

    // write until failure
    NPT_Size bytes_written;
    while (bytes_to_write) {
        NPT_Result result = Write(buffer, bytes_to_write, &bytes_written);
        if (NPT_FAILED(result)) return result;
        if (bytes_written == 0) return NPT_ERROR_INTERNAL;
        NPT_ASSERT(bytes_written <= bytes_to_write);
        bytes_to_write -= bytes_written;
        buffer = (const void*)(((const NPT_Byte*)buffer)+bytes_written);
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_OutputStream::WriteString
+---------------------------------------------------------------------*/
NPT_Result
NPT_OutputStream::WriteString(const char* buffer)
{
    // shortcut
    NPT_Size string_length;
    if (buffer == NULL || (string_length = NPT_StringLength(buffer)) == 0) {
        return NPT_SUCCESS;
    }

    // write the string
    return WriteFully((const void*)buffer, string_length);
}

/*----------------------------------------------------------------------
|   NPT_OutputStream::WriteLine
+---------------------------------------------------------------------*/
NPT_Result
NPT_OutputStream::WriteLine(const char* buffer)
{
    NPT_CHECK(WriteString(buffer));
    NPT_CHECK(WriteFully((const void*)"\r\n", 2));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_MemoryStream::NPT_MemoryStream
+---------------------------------------------------------------------*/
NPT_MemoryStream::NPT_MemoryStream(NPT_Size initial_capacity) : 
    m_Buffer(initial_capacity),
    m_ReadOffset(0),
    m_WriteOffset(0)
{
}

/*----------------------------------------------------------------------
|   NPT_MemoryStream::NPT_MemoryStream
+---------------------------------------------------------------------*/
NPT_MemoryStream::NPT_MemoryStream(const void* data, NPT_Size size) : 
    m_Buffer(data, size),
    m_ReadOffset(0),
    m_WriteOffset(0)
{
}

/*----------------------------------------------------------------------
|   NPT_MemoryStream::Read
+---------------------------------------------------------------------*/
NPT_Result 
NPT_MemoryStream::Read(void*     buffer, 
                       NPT_Size  bytes_to_read, 
                       NPT_Size* bytes_read)
{
    // check for shortcut
    if (bytes_to_read == 0) {
        if (bytes_read) *bytes_read = 0;
        return NPT_SUCCESS;
    }

    // clip to what's available
    NPT_Size available = m_Buffer.GetDataSize();
    if (m_ReadOffset+bytes_to_read > available) {
        bytes_to_read = available-m_ReadOffset;
    }

    // copy the data
    if (bytes_to_read) {
        NPT_CopyMemory(buffer, (void*)(((char*)m_Buffer.UseData())+m_ReadOffset), bytes_to_read);
        m_ReadOffset += bytes_to_read;
    } 
    if (bytes_read) *bytes_read = bytes_to_read;

    return bytes_to_read?NPT_SUCCESS:NPT_ERROR_EOS; 
}

/*----------------------------------------------------------------------
|   NPT_MemoryStream::InputSeek
+---------------------------------------------------------------------*/
NPT_Result 
NPT_MemoryStream::InputSeek(NPT_Position offset)
{
    if (offset > m_Buffer.GetDataSize()) {
        return NPT_ERROR_INVALID_PARAMETERS;
    } else {
        m_ReadOffset = offset;
        return NPT_SUCCESS;
    }
}

/*----------------------------------------------------------------------
|   NPT_MemoryStream::Write
+---------------------------------------------------------------------*/
NPT_Result 
NPT_MemoryStream::Write(const void* data, 
                        NPT_Size    bytes_to_write, 
                        NPT_Size*   bytes_written)
{
    NPT_CHECK(m_Buffer.Reserve(m_WriteOffset+bytes_to_write));

    NPT_CopyMemory(m_Buffer.UseData()+m_WriteOffset, data, bytes_to_write);
    m_WriteOffset += bytes_to_write;
    if (m_WriteOffset > m_Buffer.GetDataSize()) {
        m_Buffer.SetDataSize(m_WriteOffset);
    }
    if (bytes_written) *bytes_written = bytes_to_write;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_MemoryStream::OutputSeek
+---------------------------------------------------------------------*/
NPT_Result 
NPT_MemoryStream::OutputSeek(NPT_Position offset)
{
    if (offset <= m_Buffer.GetDataSize()) {
        m_WriteOffset = offset;
        return NPT_SUCCESS;
    } else {
        return NPT_ERROR_INVALID_PARAMETERS;
    }
}

/*----------------------------------------------------------------------
|   NPT_MemoryStream::SetSize
+---------------------------------------------------------------------*/
NPT_Result 
NPT_MemoryStream::SetSize(NPT_Size size)
{
    // try to resize the data buffer
    NPT_CHECK(m_Buffer.SetDataSize(size));

    // adjust the read and write offsets
    if (m_ReadOffset > size) m_ReadOffset = size;
    if (m_WriteOffset > size) m_WriteOffset = size;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_StreamToStreamCopy
+---------------------------------------------------------------------*/
const unsigned int NPT_STREAM_COPY_BUFFER_SIZE = 4096; // copy 4k at a time
NPT_Result 
NPT_StreamToStreamCopy(NPT_InputStream&  from, 
                       NPT_OutputStream& to,
                       NPT_Position      offset /* = 0 */,
                       NPT_Size          size   /* = 0, 0 means the entire stream */)
{
    // seek into the input if required
    if (offset) {
        NPT_CHECK(from.Seek(offset));
    }

    // allocate a buffer for the transfer
    NPT_Size bytes_transfered = 0;
    NPT_Byte* buffer = new NPT_Byte[NPT_STREAM_COPY_BUFFER_SIZE];
    NPT_Result result = NPT_SUCCESS;
    if (buffer == NULL) return NPT_ERROR_OUT_OF_MEMORY;

    // copy until an error occurs or the end of stream is reached
    for (;;) {
        // read some data
        NPT_Size   bytes_to_read = NPT_STREAM_COPY_BUFFER_SIZE;
        NPT_Size   bytes_read = 0;
        if (size) {
            // a max size was specified
            if (bytes_to_read > size-bytes_transfered) {
                bytes_to_read = size-bytes_transfered;
            }
        }
        result = from.Read(buffer, bytes_to_read, &bytes_read);
        if (NPT_FAILED(result)) {
            if (result == NPT_ERROR_EOS) result = NPT_SUCCESS;
            break;
        }
        if (bytes_read == 0) continue;
        
        // write the data
        result = to.WriteFully(buffer, bytes_read);
        if (NPT_FAILED(result)) break;

        // update the counts
        if (size) {
            bytes_transfered += bytes_read;
            if (bytes_transfered >= size) break;
        }
    }

    // free the buffer and return
    delete[] buffer;
    return result;
}

/*----------------------------------------------------------------------
|   NPT_StringOutputStream::NPT_StringOutputStream
+---------------------------------------------------------------------*/
NPT_StringOutputStream::NPT_StringOutputStream(NPT_Size size) :
    m_String(new NPT_String),
    m_StringIsOwned(true)
{
    m_String->Reserve(size);
}


/*----------------------------------------------------------------------
|   NPT_StringOutputStream::NPT_StringOutputStream
+---------------------------------------------------------------------*/
NPT_StringOutputStream::NPT_StringOutputStream(NPT_String* storage) :
    m_String(storage),
    m_StringIsOwned(false)
{
}

/*----------------------------------------------------------------------
|   NPT_StringOutputStream::~NPT_StringOutputStream
+---------------------------------------------------------------------*/
NPT_StringOutputStream::~NPT_StringOutputStream()
{
    if (m_StringIsOwned) delete m_String;
}

/*----------------------------------------------------------------------
|   NPT_StringOutputStream::Write
+---------------------------------------------------------------------*/
NPT_Result 
NPT_StringOutputStream::Write(const void* buffer, NPT_Size bytes_to_write, NPT_Size* bytes_written /* = NULL */)
{
     m_String->Append((const char*)buffer, bytes_to_write);
    if (bytes_written) *bytes_written = bytes_to_write;
    return NPT_SUCCESS;
}
