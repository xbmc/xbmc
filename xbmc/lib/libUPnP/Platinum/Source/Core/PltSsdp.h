/*****************************************************************
|
|   Platinum - SSDP
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

#ifndef _PLT_SSDP_H_
#define _PLT_SSDP_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltSsdpListener.h"
#include "PltThreadTask.h"
#include "PltHttpServerTask.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_DeviceHost;
typedef NPT_Reference<PLT_DeviceHost> PLT_DeviceHostReference;

/*----------------------------------------------------------------------
|   PLT_SsdpSender class
+---------------------------------------------------------------------*/
class PLT_SsdpSender
{
public:
    static NPT_Result SendSsdp(NPT_HttpRequest&   request, 
                               const char*        usn,
                               const char*        nt,
                               NPT_UdpSocket&     socket,
                               bool               notify,
                               NPT_SocketAddress* addr = NULL);
     
    static NPT_Result SendSsdp(NPT_HttpResponse&  response,
                               const char*        usn,
                               const char*        nt,
                               NPT_UdpSocket&     socket,
                               bool               notify, 
                               NPT_SocketAddress* addr = NULL);

private:
    static NPT_Result FormatPacket(NPT_HttpMessage&   message,
                                   const char*        usn,
                                   const char*        nt,
                                   NPT_UdpSocket&     socket,
                                   bool               notify);
};

/*----------------------------------------------------------------------
|   PLT_SsdpDeviceSearchResponseInterfaceIterator class
+---------------------------------------------------------------------*/
class PLT_SsdpDeviceSearchResponseInterfaceIterator
{
public:
    PLT_SsdpDeviceSearchResponseInterfaceIterator(NPT_SocketAddress* remote_addr,
                                                  PLT_DeviceHostReference& device, 
                                                  const char*        st) :
        m_RemoteAddr(remote_addr), m_Device(device), m_ST(st)  {}
    virtual ~PLT_SsdpDeviceSearchResponseInterfaceIterator() {}
      
    NPT_Result operator()(NPT_NetworkInterface*& if_addr) const;

private:
    NPT_SocketAddress* m_RemoteAddr;
    PLT_DeviceHostReference m_Device;
    NPT_String         m_ST;
};

/*----------------------------------------------------------------------
|   PLT_SsdpDeviceSearchResponseTask class
+---------------------------------------------------------------------*/
class PLT_SsdpDeviceSearchResponseTask : public PLT_ThreadTask
{
public:
    PLT_SsdpDeviceSearchResponseTask(PLT_DeviceHostReference& device, 
                                     NPT_SocketAddress addr,
                                     const char*       st) : 
        m_Device(device), m_Addr(addr), m_ST(st) {}

protected:
    virtual ~PLT_SsdpDeviceSearchResponseTask() {}

    // PLT_ThreadTask methods
    virtual void DoRun();
    
protected:
    PLT_DeviceHostReference m_Device;
    NPT_SocketAddress   m_Addr;
    NPT_String          m_ST;
};

/*----------------------------------------------------------------------
|   PLT_SsdpAnnounceInterfaceIterator class
+---------------------------------------------------------------------*/
class PLT_SsdpAnnounceInterfaceIterator
{
public:
    PLT_SsdpAnnounceInterfaceIterator(PLT_DeviceHostReference& device, bool is_byebye = false, bool broadcast = false) :
        m_Device(device), m_IsByeBye(is_byebye), m_Broadcast(broadcast) {}
      
    NPT_Result operator()(NPT_NetworkInterface*& if_addr) const;
    
private:
    PLT_DeviceHostReference& m_Device;
    bool            m_IsByeBye;
    bool            m_Broadcast;
};

/*----------------------------------------------------------------------
|   PLT_SsdpInitMulticastIterator class
+---------------------------------------------------------------------*/
class PLT_SsdpInitMulticastIterator
{
public:
    PLT_SsdpInitMulticastIterator(NPT_UdpMulticastSocket* socket) :
        m_Socket(socket) {}

    NPT_Result operator()(NPT_NetworkInterface*& if_addr) const {
        NPT_IpAddress addr;
        addr.ResolveName("239.255.255.250");

#if 0
        NPT_List<NPT_NetworkInterfaceAddress>::Iterator niaddr = if_addr->GetAddresses().GetFirstItem();
        if (!niaddr) return NPT_FAILURE;

        //FIXME: Should we iterate through all addresses or at least check for disconnected ones ("0.0.0.0")?

        return m_Socket->JoinGroup(addr, (*niaddr).GetPrimaryAddress());
#else
        return m_Socket->JoinGroup(addr, NPT_IpAddress::Any);
#endif
    }

private:
    NPT_UdpMulticastSocket* m_Socket;
};

/*----------------------------------------------------------------------
|   PLT_SsdpDeviceAnnounceTask class
+---------------------------------------------------------------------*/
class PLT_SsdpDeviceAnnounceTask : public PLT_ThreadTask
{
public:
    PLT_SsdpDeviceAnnounceTask(PLT_DeviceHostReference& device, 
                               NPT_TimeInterval repeat,
                               bool             is_byebye_first = false,
                               bool             broadcast = false) : 
        m_Device(device), m_Repeat(repeat), m_IsByeByeFirst(is_byebye_first), m_IsBroadcast(broadcast) {}

protected:
    virtual ~PLT_SsdpDeviceAnnounceTask() {}

    // PLT_ThreadTask methods
    virtual void DoRun();

protected:
    PLT_DeviceHostReference     m_Device;
    NPT_TimeInterval            m_Repeat;
    bool                        m_IsByeByeFirst;
    bool                        m_IsBroadcast;
};

/*----------------------------------------------------------------------
|   PLT_NetworkInterfaceAddressSearchIterator class
+---------------------------------------------------------------------*/
class PLT_NetworkInterfaceAddressSearchIterator
{
public:
    PLT_NetworkInterfaceAddressSearchIterator(NPT_String ip) : m_Ip(ip)  {}
    virtual ~PLT_NetworkInterfaceAddressSearchIterator() {}

    NPT_Result operator()(NPT_NetworkInterface*& addr) const {
        NPT_List<NPT_NetworkInterfaceAddress>::Iterator niaddr = addr->GetAddresses().GetFirstItem();
        if (!niaddr) return NPT_FAILURE;

        return (m_Ip.Compare((*niaddr).GetPrimaryAddress().ToString(), true) == 0) ? NPT_SUCCESS : NPT_FAILURE;
    }

private:
    NPT_String m_Ip;
};

/*----------------------------------------------------------------------
|   PLT_SsdpPacketListenerIterator class
+---------------------------------------------------------------------*/
class PLT_SsdpPacketListenerIterator
{
public:
    PLT_SsdpPacketListenerIterator(NPT_HttpRequest& request, NPT_SocketInfo info) :
      m_Request(request), m_Info(info) {}

    NPT_Result operator()(PLT_SsdpPacketListener*& listener) const {
        return listener->OnSsdpPacket(m_Request, m_Info);
    }

private:
    NPT_HttpRequest& m_Request;
    NPT_SocketInfo   m_Info;
};

/*----------------------------------------------------------------------
|   PLT_SsdpListenTask class
+---------------------------------------------------------------------*/
class PLT_SsdpListenTask : public PLT_HttpServerSocketTask
{
public:
    PLT_SsdpListenTask(NPT_Socket* socket, bool multicast = true) : 
        PLT_HttpServerSocketTask(socket, true), 
        m_IsMulticast(multicast) {}

    NPT_Result AddListener(PLT_SsdpPacketListener* listener) {
        NPT_AutoLock lock(m_Mutex);
        m_Listeners.Add(listener);
        return NPT_SUCCESS;
    }

    NPT_Result RemoveListener(PLT_SsdpPacketListener* listener) {
        NPT_AutoLock lock(m_Mutex);
        m_Listeners.Remove(listener);
        return NPT_SUCCESS;
    }

protected:
    virtual ~PLT_SsdpListenTask() {}

    // PLT_ThreadTask methods
    virtual void DoInit();

    // PLT_HttpServerSocketTask methods
    NPT_Result GetInputStream(NPT_InputStreamReference& stream);
    NPT_Result GetInfo(NPT_SocketInfo& info);
    NPT_Result ProcessRequest(NPT_HttpRequest&   request, 
                              NPT_SocketInfo     info, 
                              NPT_HttpResponse*& response,
                              bool&              headers_only);

protected:
    PLT_InputDatagramStreamReference  m_Datagram;
    bool                              m_IsMulticast;
    NPT_List<PLT_SsdpPacketListener*> m_Listeners;
    NPT_Mutex                         m_Mutex;
};

/*----------------------------------------------------------------------
|   PLT_SsdpSearchTask class
+---------------------------------------------------------------------*/
class PLT_SsdpSearchTask : public PLT_ThreadTask
{
public:
    PLT_SsdpSearchTask(NPT_UdpSocket*                  socket,
                       PLT_SsdpSearchResponseListener* listener, 
                       NPT_HttpRequest*                request,
                       NPT_Timeout                     timeout,
                       bool                            repeat = true);

protected:
    virtual ~PLT_SsdpSearchTask();

    // PLT_ThreadTask methods
    virtual void DoAbort();
    virtual void DoRun();

    virtual NPT_Result ProcessResponse(NPT_Result        res, 
                                       NPT_HttpRequest*  request, 
                                       NPT_SocketInfo&   info, 
                                       NPT_HttpResponse* response);

private:
    PLT_SsdpSearchResponseListener* m_Listener;
    NPT_HttpRequest*                m_Request;
    NPT_Timeout                     m_Timeout;
    bool                            m_Repeat;
    NPT_UdpSocket*                  m_Socket;
};

#endif /* _PLT_SSDP_H_ */
