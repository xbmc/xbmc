/*****************************************************************
|
|   Neptune - Data Buffer
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptDataBuffer.h"
#include "NptUtils.h"
#include "NptResults.h"

/*----------------------------------------------------------------------
|   NPT_DataBuffer::NPT_DataBuffer
+---------------------------------------------------------------------*/
NPT_DataBuffer::NPT_DataBuffer() :
    m_BufferIsLocal(true),
    m_Buffer(NULL),
    m_BufferSize(0),
    m_DataSize(0)
{
}

/*----------------------------------------------------------------------
|   NPT_DataBuffer::NPT_DataBuffer
+---------------------------------------------------------------------*/
NPT_DataBuffer::NPT_DataBuffer(NPT_Size bufferSize) :
    m_BufferIsLocal(true),
    m_Buffer(bufferSize?new NPT_Byte[bufferSize]:NULL),
    m_BufferSize(bufferSize),
    m_DataSize(0)
{
}

/*----------------------------------------------------------------------
|   NPT_DataBuffer::NPT_DataBuffer
+---------------------------------------------------------------------*/
NPT_DataBuffer::NPT_DataBuffer(const void* data, NPT_Size data_size, bool copy) :
    m_BufferIsLocal(copy),
    m_Buffer(copy?(data_size?new NPT_Byte[data_size]:NULL):reinterpret_cast<NPT_Byte*>(const_cast<void*>(data))),
    m_BufferSize(data_size),
    m_DataSize(data_size)
{
    if (copy && data_size) NPT_CopyMemory(m_Buffer, data, data_size);
}

/*----------------------------------------------------------------------
|   NPT_DataBuffer::NPT_DataBuffer
+---------------------------------------------------------------------*/
NPT_DataBuffer::NPT_DataBuffer(const NPT_DataBuffer& other) :
    m_BufferIsLocal(true),
    m_Buffer(NULL),
    m_BufferSize(other.m_DataSize),
    m_DataSize(other.m_DataSize)
{
    if (m_BufferSize) {
        m_Buffer = new NPT_Byte[m_BufferSize];
        NPT_CopyMemory(m_Buffer, other.m_Buffer, m_BufferSize);
    }
}

/*----------------------------------------------------------------------
|   NPT_DataBuffer::~NPT_DataBuffer
+---------------------------------------------------------------------*/
NPT_DataBuffer::~NPT_DataBuffer()
{
    Clear();
}

/*----------------------------------------------------------------------
|   NPT_DataBuffer::Clear
+---------------------------------------------------------------------*/
NPT_Result
NPT_DataBuffer::Clear()
{
    if (m_BufferIsLocal) {
        delete[] m_Buffer;
    }
    m_Buffer = NULL;
    m_DataSize = 0;
    m_BufferSize = 0;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_DataBuffer::operator=
+---------------------------------------------------------------------*/
NPT_DataBuffer&
NPT_DataBuffer::operator=(const NPT_DataBuffer& copy)
{
    // do nothing if we're assigning to ourselves
    if (this != &copy) {
        Clear();

        m_BufferIsLocal = true;
        m_BufferSize    = copy.m_BufferSize;
        m_DataSize      = copy.m_DataSize;

        if (m_BufferSize) {
            m_Buffer = new NPT_Byte[m_BufferSize];
            NPT_CopyMemory(m_Buffer, copy.m_Buffer, m_BufferSize);
        }
    }
    return *this;
}

/*----------------------------------------------------------------------
|   NPT_DataBuffer::operator==
+---------------------------------------------------------------------*/
bool
NPT_DataBuffer::operator==(const NPT_DataBuffer& other) const
{
    // check that the sizes match
    if (m_DataSize != other.m_DataSize) return false;

    return NPT_MemoryEqual(m_Buffer,
                           other.m_Buffer,
                           m_DataSize);
}

/*----------------------------------------------------------------------
|   NPT_DataBuffer::SetBuffer
+---------------------------------------------------------------------*/
NPT_Result
NPT_DataBuffer::SetBuffer(NPT_Byte* buffer, NPT_Size buffer_size)
{
    Clear();

    // we're now using an external buffer
    m_BufferIsLocal = false;
    m_Buffer = buffer;
    m_BufferSize = buffer_size;
    m_DataSize = 0;
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_DataBuffer::SetBufferSize
+---------------------------------------------------------------------*/
NPT_Result
NPT_DataBuffer::SetBufferSize(NPT_Size buffer_size)
{
    if (m_BufferIsLocal) {
        return ReallocateBuffer(buffer_size);
    } else {
        return NPT_ERROR_NOT_SUPPORTED; // you cannot change the
                                        // buffer management mode
    }
}

/*----------------------------------------------------------------------
|   NPT_DataBuffer::Reserve
+---------------------------------------------------------------------*/
NPT_Result
NPT_DataBuffer::Reserve(NPT_Size size)
{
    if (size <= m_BufferSize) return NPT_SUCCESS;

    // try doubling the buffer to accomodate for the new size
    NPT_Size new_size = m_BufferSize*2;
    if (new_size < size) new_size = size;
    return SetBufferSize(new_size);
}

/*----------------------------------------------------------------------
|   NPT_DataBuffer::SetDataSize
+---------------------------------------------------------------------*/
NPT_Result
NPT_DataBuffer::SetDataSize(NPT_Size size)
{
    if (size > m_BufferSize) {
        // the buffer is too small, we need to reallocate it
        if (m_BufferIsLocal) {
            NPT_CHECK(ReallocateBuffer(size));
        } else { 
            // we cannot reallocate an external buffer
            return NPT_ERROR_NOT_SUPPORTED;
        }
    }
    m_DataSize = size;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_DataBuffer::SetData
+---------------------------------------------------------------------*/
NPT_Result
NPT_DataBuffer::SetData(const NPT_Byte* data, NPT_Size size)
{
    if (size > m_BufferSize) {
        if (m_BufferIsLocal) {
            NPT_CHECK(ReallocateBuffer(size));
        } else {
            return NPT_ERROR_INVALID_STATE;
        }
    }
    NPT_CopyMemory(m_Buffer, data, size);
    m_DataSize = size;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_DataBuffer::ReallocateBuffer
+---------------------------------------------------------------------*/
NPT_Result
NPT_DataBuffer::ReallocateBuffer(NPT_Size size)
{
    // check that the existing data fits
    if (m_DataSize > size) return NPT_ERROR_INVALID_PARAMETERS;

    // allocate a new buffer
    NPT_Byte* newBuffer = new NPT_Byte[size];

    // copy the contents of the previous buffer, if any
    if (m_Buffer && m_DataSize) {
        NPT_CopyMemory(newBuffer, m_Buffer, m_DataSize);
    }

    // destroy the previous buffer
    delete[] m_Buffer;

    // use the new buffer
    m_Buffer = newBuffer;
    m_BufferSize = size;

    return NPT_SUCCESS;
}
