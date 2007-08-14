/*****************************************************************
|
|   Platinum - Datagram Stream
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltDatagramStream.h"

/*----------------------------------------------------------------------
|   PLT_InputDatagramStream::PLT_InputDatagramStream
+---------------------------------------------------------------------*/
PLT_InputDatagramStream::PLT_InputDatagramStream(NPT_UdpSocket* socket) : 
    m_Socket(socket)
{
}

/*----------------------------------------------------------------------
|   PLT_InputDatagramStream::~PLT_InputDatagramStream
+---------------------------------------------------------------------*/
PLT_InputDatagramStream::~PLT_InputDatagramStream()
{
}

/*----------------------------------------------------------------------
|   PLT_InputDatagramStream::Read
+---------------------------------------------------------------------*/
NPT_Result 
PLT_InputDatagramStream::Read(void*     buffer, 
                              NPT_Size  bytes_to_read, 
                              NPT_Size* bytes_read /*= 0*/)
{

    if (bytes_read) *bytes_read = 0;

    if (bytes_to_read == 0) {
        return NPT_SUCCESS;
    }

    NPT_DataBuffer data_buffer(buffer, bytes_to_read, false);

    // read data into it now
    NPT_SocketAddress addr;
    NPT_Result res = m_Socket->Receive(data_buffer, &addr);

    // update info
    m_Socket->GetInfo(m_Info);
    m_Info.remote_address = addr;

    if (bytes_read) *bytes_read = data_buffer.GetDataSize();

    return res;
}

/*----------------------------------------------------------------------
|   PLT_OutputDatagramStream::GetInfo
+---------------------------------------------------------------------*/
NPT_Result 
PLT_InputDatagramStream::GetInfo(NPT_SocketInfo& info)
{
    info = m_Info;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_OutputDatagramStream::PLT_OutputDatagramStream
+---------------------------------------------------------------------*/
PLT_OutputDatagramStream::PLT_OutputDatagramStream(NPT_UdpSocket*   socket, 
                                                   NPT_Size         size, 
                                                   const NPT_SocketAddress* address) : 
    m_Socket(socket),
    m_Address(address?new NPT_SocketAddress(address->GetIpAddress(), address->GetPort()):NULL)
{
    m_Buffer.SetBufferSize(size);
}

/*----------------------------------------------------------------------
|   PLT_OutputDatagramStream::~PLT_OutputDatagramStream
+---------------------------------------------------------------------*/
PLT_OutputDatagramStream::~PLT_OutputDatagramStream()
{
    delete m_Address;
}

/*----------------------------------------------------------------------
|   PLT_OutputDatagramStream::Write
+---------------------------------------------------------------------*/
NPT_Result 
PLT_OutputDatagramStream::Write(const void* buffer, NPT_Size bytes_to_write, NPT_Size* bytes_written /* = NULL */)
{
    // calculate if we need to increase the buffer
    NPT_Int32 overflow = bytes_to_write - m_Buffer.GetBufferSize() + m_Buffer.GetDataSize();
    if (overflow > 0) {
        m_Buffer.Reserve(m_Buffer.GetBufferSize() + overflow);
    }
    // copy data in place at the end of what we have there already
    NPT_CopyMemory(m_Buffer.UseData() + m_Buffer.GetDataSize(), buffer, bytes_to_write);
    m_Buffer.SetDataSize(m_Buffer.GetDataSize() + bytes_to_write);

    if (bytes_written) *bytes_written = bytes_to_write;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_OutputDatagramStream::Flush
+---------------------------------------------------------------------*/
NPT_Result 
PLT_OutputDatagramStream::Flush()
{
    // send buffer now
    m_Socket->Send(m_Buffer, m_Address);

    // reset buffer
    m_Buffer.SetDataSize(0);
    return NPT_SUCCESS;
}
