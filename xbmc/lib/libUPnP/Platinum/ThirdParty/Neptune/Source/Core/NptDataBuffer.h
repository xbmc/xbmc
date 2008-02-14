/*****************************************************************
|
|   Neptune - Datagram Packets
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_DATA_BUFFER_H_
#define _NPT_DATA_BUFFER_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptConstants.h"

/*----------------------------------------------------------------------
|   NPT_DataBuffer
+---------------------------------------------------------------------*/
class NPT_DataBuffer 
{
 public:
    // constructors & destructor
    NPT_DataBuffer();              // size unknown until first set 
    NPT_DataBuffer(NPT_Size size); // initial size specified
    NPT_DataBuffer(const void* data, NPT_Size size, bool copy = true); // initial data and size specified
    NPT_DataBuffer(const NPT_DataBuffer& other);
    virtual ~NPT_DataBuffer();

    // operators
    NPT_DataBuffer& operator=(const NPT_DataBuffer& copy);
    bool            operator==(const NPT_DataBuffer& other) const;

    // data buffer handling methods
    virtual NPT_Result SetBuffer(NPT_Byte* buffer, NPT_Size bufferSize);
    virtual NPT_Result SetBufferSize(NPT_Size bufferSize);
    virtual NPT_Size   GetBufferSize() const { return m_BufferSize; }
    virtual NPT_Result Reserve(NPT_Size size);
    virtual NPT_Result Clear();

    // data handling methods
    virtual const NPT_Byte* GetData() const { return m_Buffer; }
    virtual NPT_Byte*       UseData() { return m_Buffer; };
    virtual NPT_Size        GetDataSize() const { return m_DataSize; }
    virtual NPT_Result      SetDataSize(NPT_Size size);
    virtual NPT_Result      SetData(const NPT_Byte* data, NPT_Size dataSize);

 protected:
    // members
    bool      m_BufferIsLocal;
    NPT_Byte* m_Buffer;
    NPT_Size  m_BufferSize;
    NPT_Size  m_DataSize;

    // methods
    NPT_Result ReallocateBuffer(NPT_Size size);
};

#endif // _NPT_DATA_BUFFER_H_
