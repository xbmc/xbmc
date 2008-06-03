/*****************************************************************
|
|   Platinum - Datagram Stream
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_DATAGRAM_H_
#define _PLT_DATAGRAM_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"

/*----------------------------------------------------------------------
|   PLT_InputDatagramStream
+---------------------------------------------------------------------*/
class PLT_InputDatagramStream : public NPT_InputStream
{
public:
    // methods
    PLT_InputDatagramStream(NPT_UdpSocket* socket);
    virtual ~PLT_InputDatagramStream();
    
    NPT_Result GetInfo(NPT_SocketInfo& info);

    // NPT_InputStream methods
    NPT_Result Read(void*     buffer, 
                    NPT_Size  bytes_to_read, 
                    NPT_Size* bytes_read = 0);

    NPT_Result Seek(NPT_Position offset) { NPT_COMPILER_UNUSED(offset); return NPT_FAILURE; }
    NPT_Result Skip(NPT_Position offset) { NPT_COMPILER_UNUSED(offset); return NPT_FAILURE; }
    NPT_Result Tell(NPT_Position& offset){ NPT_COMPILER_UNUSED(offset); return NPT_FAILURE; }
    NPT_Result GetSize(NPT_Size& size)   { NPT_COMPILER_UNUSED(size); return NPT_FAILURE; }
    NPT_Result GetAvailable(NPT_Size& available) { NPT_COMPILER_UNUSED(available); return NPT_FAILURE; }

        
protected:
    NPT_UdpSocket*      m_Socket;
    NPT_SocketInfo      m_Info;
};

typedef NPT_Reference<PLT_InputDatagramStream> PLT_InputDatagramStreamReference;

/*----------------------------------------------------------------------
|   PLT_OutputDatagramStream
+---------------------------------------------------------------------*/
class PLT_OutputDatagramStream : public NPT_OutputStream
{
public:
    // methods
    PLT_OutputDatagramStream(NPT_UdpSocket*           socket, 
                             NPT_Size                 size = 4096,
                             const NPT_SocketAddress* address = NULL);
    virtual ~PLT_OutputDatagramStream();

    // NPT_OutputStream methods
    NPT_Result Write(const void* buffer, NPT_Size bytes_to_write, NPT_Size* bytes_written = NULL);
    NPT_Result Flush();

    NPT_Result Seek(NPT_Position offset)  { NPT_COMPILER_UNUSED(offset); return NPT_FAILURE; }
    NPT_Result Tell(NPT_Position& offset) { NPT_COMPILER_UNUSED(offset); return NPT_FAILURE; }

protected:
    NPT_UdpSocket*     m_Socket;
    NPT_DataBuffer     m_Buffer;
    NPT_SocketAddress* m_Address;
};

typedef NPT_Reference<PLT_OutputDatagramStream> PLT_OutputDatagramStreamReference;

#endif /* _PLT_DATAGRAM_H_ */
