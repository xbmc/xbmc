/*****************************************************************
|
|   Platinum - Ring buffer stream
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

#ifndef _PLT_RING_BUFFER_STREAM_H_
#define _PLT_RING_BUFFER_STREAM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptStreams.h"
#include "NptRingBuffer.h"
#include "NptThreads.h"

/*----------------------------------------------------------------------
|   PLT_RingBufferStream class
+---------------------------------------------------------------------*/
class PLT_RingBufferStream : public NPT_DelegatingInputStream,   
                             public NPT_DelegatingOutputStream
{
public:
    PLT_RingBufferStream(NPT_Size buffer_size = 4096, bool blocking = true);
    PLT_RingBufferStream(NPT_RingBufferReference& buffer, bool blocking = true);
    virtual ~PLT_RingBufferStream();

    void SetEos() {m_Eos = true;}

    // NPT_InputStream methods
    NPT_Result Read(void*     buffer, 
                    NPT_Size  bytes_to_read, 
                    NPT_Size* bytes_read = NULL);
    NPT_Result GetSize(NPT_Size& size)  { 
        NPT_AutoLock autoLock(m_Lock);
        size = m_TotalBytesWritten;  
        return NPT_SUCCESS;
    }
    NPT_Result GetAvailable(NPT_Size& available) { 
        NPT_AutoLock autoLock(m_Lock);
        available = m_RingBuffer->GetAvailable();
        return NPT_SUCCESS;
    }

    // NPT_OutputStream methods
    NPT_Result Write(const void* buffer, 
                     NPT_Size    bytes_to_write, 
                     NPT_Size*   bytes_written = NULL);
    NPT_Result Flush();

protected:
    // NPT_DelegatingInputStream methods
    NPT_Result InputSeek(NPT_Position offset) {
        NPT_COMPILER_UNUSED(offset);
        return NPT_FAILURE;
    }
    NPT_Result InputTell(NPT_Position& offset) { 
        NPT_AutoLock autoLock(m_Lock);
        offset = m_TotalBytesRead; 
        return NPT_SUCCESS;
    }

    // NPT_DelegatingOutputStream methods
    NPT_Result OutputSeek(NPT_Position offset) {
        NPT_COMPILER_UNUSED(offset);
        return NPT_FAILURE;
    }
    NPT_Result OutputTell(NPT_Position& offset) {
        NPT_AutoLock autoLock(m_Lock);
        offset = m_TotalBytesWritten; 
        return NPT_SUCCESS;
    }

private:
    NPT_RingBufferReference     m_RingBuffer;
    NPT_Offset                  m_TotalBytesRead;
    NPT_Offset                  m_TotalBytesWritten;
    NPT_Mutex                   m_Lock;
    bool                        m_Eos;
    bool                        m_Blocking;
};

typedef NPT_Reference<PLT_RingBufferStream> PLT_RingBufferStreamReference;

#endif // _PLT_RING_BUFFER_STREAM_H_ 
