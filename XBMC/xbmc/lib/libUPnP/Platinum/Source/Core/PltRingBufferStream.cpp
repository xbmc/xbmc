/*****************************************************************
|
|   Platinum - Ring Buffer Stream
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltRingBufferStream.h"
#include "NptUtils.h"

/*----------------------------------------------------------------------
|   PLT_RingBufferStream::PLT_RingBufferStream
+---------------------------------------------------------------------*/
PLT_RingBufferStream::PLT_RingBufferStream(NPT_Size buffer_size) : 
    m_TotalBytesRead(0),
    m_TotalBytesWritten(0),
    m_Eos(false)
{
    m_RingBuffer = new NPT_RingBuffer(buffer_size);
}

/*----------------------------------------------------------------------
|   PLT_RingBufferStream::PLT_RingBufferStream
+---------------------------------------------------------------------*/
PLT_RingBufferStream::PLT_RingBufferStream(NPT_RingBufferReference& buffer) : 
    m_RingBuffer(buffer),
    m_TotalBytesRead(0),
    m_TotalBytesWritten(0),
    m_Eos(false)
{
}

/*----------------------------------------------------------------------
|   PLT_RingBufferStream::~PLT_RingBufferStream
+---------------------------------------------------------------------*/
PLT_RingBufferStream::~PLT_RingBufferStream()
{
}

/*----------------------------------------------------------------------
|   PLT_RingBufferStream::Read
+---------------------------------------------------------------------*/
NPT_Result 
PLT_RingBufferStream::Read(void*     buffer, 
                           NPT_Size  max_bytes_to_read, 
                           NPT_Size* bytes_read /*= NULL*/)
{
    NPT_AutoLock autoLock(m_Lock);
    NPT_Size     loc_bytes_read = 0;
    NPT_Size     bytes_avail;
    
    // check how much we have available data
    if ((bytes_avail = m_RingBuffer->GetContiguousAvailable())) {
        // adjust in case we have more than requested
        NPT_Size to_read = (max_bytes_to_read>bytes_avail)?bytes_avail:max_bytes_to_read;

        // read into buffer and advance
        m_RingBuffer->Read((unsigned char*)buffer, to_read);

        // update what we have read so far
        loc_bytes_read = to_read;

        // if we need more, check again in case we wrapped around
        NPT_Size left_to_read = max_bytes_to_read - loc_bytes_read;
        if ((left_to_read > 0) && (bytes_avail = m_RingBuffer->GetContiguousAvailable())) {
            // adjust in case we have more
            to_read = (left_to_read>bytes_avail)?bytes_avail:left_to_read;

            // read into buffer
            m_RingBuffer->Read((unsigned char*)buffer+loc_bytes_read, to_read);

            // update what we have read so far
            loc_bytes_read += to_read;
        }
    }

    // keep track of the total bytes we have read so far
    m_TotalBytesRead += loc_bytes_read;
    if (bytes_read) {
        *bytes_read = loc_bytes_read;
    }

    if (loc_bytes_read) {
        // we have read some chars, so return success
        // even if we have read less than asked
        return NPT_SUCCESS;
    }

    return NPT_ERROR_WOULD_BLOCK;
}

/*----------------------------------------------------------------------
|   PLT_RingBufferStream::Write
+---------------------------------------------------------------------*/
NPT_Result 
PLT_RingBufferStream::Write(const void* buffer, 
                            NPT_Size    bytes_to_write, 
                            NPT_Size*   bytes_written /*= NULL*/)
{
    NPT_AutoLock autoLock(m_Lock);
    NPT_Result res = NPT_ERROR_WOULD_BLOCK;

    NPT_Size byte_total = m_RingBuffer->GetSpace();
    NPT_Size total_bytes_to_write = bytes_to_write<byte_total?bytes_to_write:byte_total;
    if (total_bytes_to_write > 0) {
        m_RingBuffer->Write(buffer, total_bytes_to_write);
    }

    m_TotalBytesWritten += total_bytes_to_write; 

    if (bytes_written) {
        *bytes_written = total_bytes_to_write;
    }

    if (total_bytes_to_write) {
        // we have written some chars, so return success
        // even if we have written less than provided
        return NPT_SUCCESS;
    }

    return NPT_ERROR_WOULD_BLOCK;
}

/*----------------------------------------------------------------------
|   PLT_RingBufferStream::Flush
+---------------------------------------------------------------------*/
NPT_Result 
PLT_RingBufferStream::Flush()
{
    NPT_AutoLock autoLock(m_Lock);
    m_RingBuffer->Flush();
    m_TotalBytesRead = 0;
    m_TotalBytesWritten = 0;
    return NPT_SUCCESS;
}
