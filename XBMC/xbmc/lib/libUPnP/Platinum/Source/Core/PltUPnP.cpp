/*****************************************************************
|
|   Platinum - UPnP Engine
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"
#include "PltUPnP.h"
#include "PltDeviceHost.h"
#include "PltCtrlPoint.h"
#include "PltSsdp.h"

NPT_SET_LOCAL_LOGGER("platinum.core.upnp")

/*----------------------------------------------------------------------
|   PLT_UPnP_CtrlPointStartIterator class
+---------------------------------------------------------------------*/
class PLT_UPnP_CtrlPointStartIterator
{
public:
    PLT_UPnP_CtrlPointStartIterator(PLT_SsdpListenTask* listen_task) :
        m_ListenTask(listen_task)  {}
    virtual ~PLT_UPnP_CtrlPointStartIterator() {}

    NPT_Result operator()(PLT_CtrlPointReference& ctrl_point) const {
        NPT_CHECK_SEVERE(ctrl_point->Start(m_ListenTask));
        return NPT_SUCCESS;
    }

private:
    PLT_SsdpListenTask* m_ListenTask;
};

/*----------------------------------------------------------------------
|   PLT_UPnP_CtrlPointStopIterator class
+---------------------------------------------------------------------*/
class PLT_UPnP_CtrlPointStopIterator
{
public:
    PLT_UPnP_CtrlPointStopIterator(PLT_SsdpListenTask* listen_task) :
        m_ListenTask(listen_task)  {}
    virtual ~PLT_UPnP_CtrlPointStopIterator() {}

    NPT_Result operator()(PLT_CtrlPointReference& ctrl_point) const {
        return ctrl_point->Stop(m_ListenTask);
    }


private:
    PLT_SsdpListenTask* m_ListenTask;
};

/*----------------------------------------------------------------------
|   PLT_UPnP_DeviceStartIterator class
+---------------------------------------------------------------------*/
class PLT_UPnP_DeviceStartIterator
{
public:
    PLT_UPnP_DeviceStartIterator(PLT_SsdpListenTask* listen_task) :
        m_ListenTask(listen_task)  {}
    virtual ~PLT_UPnP_DeviceStartIterator() {}

    NPT_Result operator()(PLT_DeviceHostReference& device_host) const {
        NPT_CHECK_SEVERE(device_host->Start(m_ListenTask));
        return NPT_SUCCESS;
    }

private:
    PLT_SsdpListenTask* m_ListenTask;
};

/*----------------------------------------------------------------------
|   PLT_UPnP_DeviceStopIterator class
+---------------------------------------------------------------------*/
class PLT_UPnP_DeviceStopIterator
{
public:
    PLT_UPnP_DeviceStopIterator(PLT_SsdpListenTask* listen_task) :
        m_ListenTask(listen_task)  {}
    virtual ~PLT_UPnP_DeviceStopIterator() {}

    NPT_Result operator()(PLT_DeviceHostReference& device_host) const {
        return device_host->Stop(m_ListenTask);
    }


private:
    PLT_SsdpListenTask* m_ListenTask;
};

/*----------------------------------------------------------------------
|   PLT_UPnP::PLT_UPnP
+---------------------------------------------------------------------*/
PLT_UPnP::PLT_UPnP(NPT_UInt32 port, bool multicast /* = true */) :
    m_Started(false),
    m_Port(port),
    m_Multicast(multicast),
    m_SsdpListenTask(NULL)
{
}
    
/*----------------------------------------------------------------------
|   PLT_UPnP::~PLT_UPnP
+---------------------------------------------------------------------*/
PLT_UPnP::~PLT_UPnP()
{
    Stop();

    m_CtrlPoints.Clear();
    m_Devices.Clear();
}

/*----------------------------------------------------------------------
|   PLT_UPnP::Start()
+---------------------------------------------------------------------*/
NPT_Result
PLT_UPnP::Start()
{
    NPT_LOG_INFO("Starting UPnP...");

    NPT_AutoLock lock(m_Lock);

    if (m_Started == true) return NPT_FAILURE;

    NPT_Socket* socket = m_Multicast?new NPT_UdpMulticastSocket(): new NPT_UdpSocket();
    NPT_CHECK_SEVERE(socket->Bind(NPT_SocketAddress(NPT_IpAddress::Any, m_Port)));

    /* create the ssdp listener */
    m_SsdpListenTask = new PLT_SsdpListenTask(socket, m_Multicast);
    NPT_CHECK_SEVERE(m_TaskManager.StartTask(m_SsdpListenTask));

    /* start devices & ctrlpoints */
    m_CtrlPoints.Apply(PLT_UPnP_CtrlPointStartIterator(m_SsdpListenTask));
    m_Devices.Apply(PLT_UPnP_DeviceStartIterator(m_SsdpListenTask));

    m_Started = true;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_UPnP::Stop
+---------------------------------------------------------------------*/
NPT_Result
PLT_UPnP::Stop()
{
    NPT_AutoLock lock(m_Lock);

    if (m_Started == false) return NPT_FAILURE;

    NPT_LOG_INFO("Stopping UPnP...");

    // Stop ctrlpoints and devices first
    m_CtrlPoints.Apply(PLT_UPnP_CtrlPointStopIterator(m_SsdpListenTask));
    m_Devices.Apply(PLT_UPnP_DeviceStopIterator(m_SsdpListenTask));

    m_TaskManager.StopAllTasks();
    m_SsdpListenTask = NULL;

    m_Started = false;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_UPnP::AddDevice
+---------------------------------------------------------------------*/
NPT_Result
PLT_UPnP::AddDevice(PLT_DeviceHostReference& device)
{
    NPT_AutoLock lock(m_Lock);

    if (m_Started) {
        NPT_LOG_INFO("Starting Device...");
        NPT_CHECK_SEVERE(device->Start(m_SsdpListenTask));
    }

    m_Devices.Add(device);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_UPnP::RemoveDevice
+---------------------------------------------------------------------*/
NPT_Result
PLT_UPnP::RemoveDevice(PLT_DeviceHostReference& device)
{
    NPT_AutoLock lock(m_Lock);

    if (m_Started) {
        device->Stop(m_SsdpListenTask);
    }

    return m_Devices.Remove(device);
}

/*----------------------------------------------------------------------
|   PLT_UPnP::AddCtrlPoint
+---------------------------------------------------------------------*/
NPT_Result
PLT_UPnP::AddCtrlPoint(PLT_CtrlPointReference& ctrl_point)
{
    NPT_AutoLock lock(m_Lock);

    if (m_Started) {
        NPT_LOG_INFO("Starting Ctrlpoint...");
        NPT_CHECK_SEVERE(ctrl_point->Start(m_SsdpListenTask));
    }

    m_CtrlPoints.Add(ctrl_point);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_UPnP::RemoveCtrlPoint
+---------------------------------------------------------------------*/
NPT_Result
PLT_UPnP::RemoveCtrlPoint(PLT_CtrlPointReference& ctrl_point)
{
    NPT_AutoLock lock(m_Lock);

    if (m_Started) {
        ctrl_point->Stop(m_SsdpListenTask);
    }

    return m_CtrlPoints.Remove(ctrl_point);
}

