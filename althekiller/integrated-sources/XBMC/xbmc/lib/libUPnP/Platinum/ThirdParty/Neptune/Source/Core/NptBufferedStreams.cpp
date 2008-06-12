/*****************************************************************
|
|   Neptune - Buffered Streams
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptInterfaces.h"
#include "NptConstants.h"
#include "NptBufferedStreams.h"
#include "NptUtils.h"

/*----------------------------------------------------------------------
|   NPT_BufferedInputStream::NPT_BufferedInputStream
+---------------------------------------------------------------------*/
NPT_BufferedInputStream::NPT_BufferedInputStream(NPT_InputStreamReference& source, NPT_Size buffer_size) :
    m_Source(source),
    m_SkipNewline(false),
    m_Eos(false)
{
    // setup the read buffer
    m_Buffer.data     = NULL;
    m_Buffer.offset   = 0;
    m_Buffer.valid    = 0;
    m_Buffer.size     = buffer_size;
}

/*----------------------------------------------------------------------
|   NPT_BufferedInputStream::~NPT_BufferedInputStream
+---------------------------------------------------------------------*/
NPT_BufferedInputStream::~NPT_BufferedInputStream()
{
    // release the buffer
    delete[] m_Buffer.data;
}

/*----------------------------------------------------------------------
|   NPT_BufferedInputStream::SetBufferSize
+---------------------------------------------------------------------*/
NPT_Result
NPT_BufferedInputStream::SetBufferSize(NPT_Size size)
{
    if (m_Buffer.data != NULL) {
        // we already have a buffer
        if (m_Buffer.size < size) {
            // the current buffer is too small, reallocate
            NPT_Byte* buffer = new NPT_Byte[size];
            if (buffer == NULL) return NPT_ERROR_OUT_OF_MEMORY;

            // copy existing data
            NPT_Size need_to_copy = m_Buffer.valid - m_Buffer.offset;
            if (need_to_copy) {
                NPT_CopyMemory((void*)buffer, 
                               m_Buffer.data+m_Buffer.offset,
                               need_to_copy);
            }

            // use the new buffer
            delete[] m_Buffer.data;
            m_Buffer.data = buffer;
            m_Buffer.valid -= m_Buffer.offset;
            m_Buffer.offset = 0;
        }
    }
    m_Buffer.size = size;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BufferedInputStream::FillBuffer
+---------------------------------------------------------------------*/
NPT_Result
NPT_BufferedInputStream::FillBuffer()
{
    // shortcut
    if (m_Eos) return NPT_ERROR_EOS;

    // check that there is nothing left in the buffer and the buffer
    // size is not 0
    NPT_ASSERT(m_Buffer.valid == m_Buffer.offset);
    NPT_ASSERT(m_Buffer.size != 0);

    // allocate the read buffer if it has not been done yet
    if (m_Buffer.data == NULL) {
        m_Buffer.data = new NPT_Byte[m_Buffer.size];
        if (m_Buffer.data == NULL) return NPT_ERROR_OUT_OF_MEMORY;
    }

    // refill the buffer
    m_Buffer.offset = 0;
    NPT_Result result = m_Source->Read(m_Buffer.data, m_Buffer.size, &m_Buffer.valid);
    if (NPT_FAILED(result)) m_Buffer.valid = 0;
    return result;
}

/*----------------------------------------------------------------------
|   NPT_BufferedInputStream::ReleaseBuffer
+---------------------------------------------------------------------*/
NPT_Result
NPT_BufferedInputStream::ReleaseBuffer()
{
    NPT_ASSERT(m_Buffer.size == 0);
    NPT_ASSERT(m_Buffer.offset == m_Buffer.valid);

    delete[] m_Buffer.data;
    m_Buffer.data = NULL;
    m_Buffer.offset = 0;
    m_Buffer.valid = 0;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BufferedInputStream::ReadLine
+---------------------------------------------------------------------*/
NPT_Result
NPT_BufferedInputStream::ReadLine(char* buffer, NPT_Size size, NPT_Size* chars_read)
{
    NPT_Result result = NPT_SUCCESS;
    char*      buffer_start = buffer;
    bool       skip_newline = false;

    // check parameters
    if (buffer == NULL || size < 1) return NPT_ERROR_INVALID_PARAMETERS;

    // read until EOF or newline
    while (buffer-buffer_start < (long)(size-1)) {
        while (m_Buffer.offset != m_Buffer.valid) {
            // there is some data left in the buffer
            NPT_Byte c = m_Buffer.data[m_Buffer.offset++];
            if (c == '\r') {
                skip_newline = true;
                goto done;
            } else if (c == '\n') {
                if (m_SkipNewline && (buffer == buffer_start)) {
                    continue;
                }
                goto done;
            } else {
                *buffer++ = c;
            }
        }

        if (m_Buffer.size == 0 && !m_Eos) {
            // unbuffered mode
            if (m_Buffer.data != NULL) ReleaseBuffer();
            while (NPT_SUCCEEDED(result = m_Source->Read(buffer, 1, NULL))) {
                if (*buffer == '\r') {
                    m_SkipNewline = true;
                    goto done;
                } else if (*buffer == '\n') {
                    goto done;
                } else {
                    ++buffer;
                }
            }
        } else {
            // refill the buffer
            result = FillBuffer();
        }
        if (NPT_FAILED(result)) goto done;
    }

done:
    // update the newline skipping state
    m_SkipNewline = skip_newline;

    // NULL-terminate the line
    *buffer = '\0';

    // return what we have
    if (chars_read) *chars_read = (NPT_Size)(buffer-buffer_start);
    if (result == NPT_ERROR_EOS) {
        m_Eos = true;
        if (buffer != buffer_start) {
            // we have reached the end of the stream, but we have read
            // some chars, so do not return EOS now
            return NPT_SUCCESS;
        }
    }
    return result;
}

/*----------------------------------------------------------------------
|   NPT_BufferedInputStream::ReadLine
+---------------------------------------------------------------------*/
NPT_Result
NPT_BufferedInputStream::ReadLine(NPT_String& line,
                                  NPT_Size    max_chars)
{
    // clear the line
    line.SetLength(0);

    // reserve space for the chars
    line.Reserve(max_chars);

    // read the line
    NPT_Size chars_read = 0;
    NPT_CHECK(ReadLine(line.UseChars(), max_chars, &chars_read));

    // adjust the length of the string object
    line.SetLength(chars_read);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_BufferedInputStream::Read
+---------------------------------------------------------------------*/
NPT_Result 
NPT_BufferedInputStream::Read(void*     buffer, 
                              NPT_Size  bytes_to_read, 
                              NPT_Size* bytes_read)
{
    NPT_Result result = NPT_SUCCESS;
    NPT_Size   total_read = 0;
    NPT_Size   buffered;

    // check for a possible shortcut
    if (bytes_to_read == 0) return NPT_SUCCESS;

    // skip a newline char if needed
    if (m_SkipNewline) {
        m_SkipNewline = false;
        result = Read(buffer, 1, NULL);
        if (NPT_FAILED(result)) goto done;
        NPT_Byte c = *(NPT_Byte*)buffer;
        if (c != '\n') {
            buffer = (void*)((NPT_Byte*)buffer+1);
            --bytes_to_read;
            total_read = 1;
        }
    }

    // compute how much is buffered
    buffered = m_Buffer.valid-m_Buffer.offset;
    if (bytes_to_read > buffered) {
        // there is not enough in the buffer, take what's there
        NPT_CopyMemory(buffer, 
                       m_Buffer.data + m_Buffer.offset,
                       buffered);
        buffer = (void*)((NPT_Byte*)buffer+buffered);
        m_Buffer.offset += buffered;
        bytes_to_read -= buffered;
        total_read += buffered;

        // read the rest from the source
        if (m_Buffer.size == 0) {
            // unbuffered mode, read directly into the supplied buffer
            if (m_Buffer.data != NULL) ReleaseBuffer();
            NPT_Size local_read = 0;
            result = m_Source->Read(buffer, bytes_to_read, &local_read);
            if (NPT_SUCCEEDED(result)) {
                total_read += local_read; 
            }
            goto done;
        } else {
            // refill the buffer
            result = FillBuffer();
            if (NPT_FAILED(result)) goto done;
            buffered = m_Buffer.valid;
            if (bytes_to_read > buffered) bytes_to_read = buffered;
        }
    }

    // get what we can from the buffer
    NPT_CopyMemory(buffer, 
                   m_Buffer.data + m_Buffer.offset,
                   bytes_to_read);
    m_Buffer.offset += bytes_to_read;
    total_read += bytes_to_read;

done:
    if (bytes_read) *bytes_read = total_read;
    if (result == NPT_ERROR_EOS) { 
        m_Eos = true;
        if (total_read != 0) {
            // we have reached the end of the stream, but we have read
            // some chars, so do not return EOS now
            return NPT_SUCCESS;
        }
    }
    return result;
}

/*----------------------------------------------------------------------
|   NPT_BufferedInputStream::Seek
+---------------------------------------------------------------------*/
NPT_Result 
NPT_BufferedInputStream::Seek(NPT_Position /*offset*/)
{
    // not implemented yet
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_BufferedInputStream::Tell
+---------------------------------------------------------------------*/
NPT_Result 
NPT_BufferedInputStream::Tell(NPT_Position& offset)
{
    // not implemented yet
    offset = 0;
    return NPT_ERROR_NOT_IMPLEMENTED;
}

/*----------------------------------------------------------------------
|   NPT_BufferedInputStream::GetSize
+---------------------------------------------------------------------*/
NPT_Result 
NPT_BufferedInputStream::GetSize(NPT_Size& size)
{
    return m_Source->GetSize(size);
}

/*----------------------------------------------------------------------
|   NPT_BufferedInputStream::GetAvailable
+---------------------------------------------------------------------*/
NPT_Result 
NPT_BufferedInputStream::GetAvailable(NPT_Size& available)
{
    NPT_Size source_available = 0;
    NPT_Result result = m_Source->GetAvailable(source_available);
    if (NPT_SUCCEEDED(result)) {
        available = m_Buffer.valid-m_Buffer.offset + source_available;
        return NPT_SUCCESS;
    } else {
        available = 0;
        return result;
    }
}
