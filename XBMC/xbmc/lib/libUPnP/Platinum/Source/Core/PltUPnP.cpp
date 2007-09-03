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
    PLT_UPnP_CtrlPointStartIterator(PLT_TaskManager* task_manager, PLT_SsdpListenTask* listen_task) :
        m_TaskManager(task_manager), m_ListenTask(listen_task)  {}
    virtual ~PLT_UPnP_CtrlPointStartIterator() {}

    NPT_Result operator()(PLT_CtrlPointReference& ctrl_point) const {
        NPT_CHECK_SEVERE(ctrl_point->Start(m_TaskManager));
        return m_ListenTask->AddListener(ctrl_point.AsPointer());
    }

private:
    PLT_TaskManager*    m_TaskManager;
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
        m_ListenTask->RemoveListener(ctrl_point.AsPointer());
        return ctrl_point->Stop();
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
    PLT_UPnP_DeviceStartIterator(PLT_TaskManager* task_manager, PLT_SsdpListenTask* listen_task) :
        m_TaskManager(task_manager), m_ListenTask(listen_task)  {}
    virtual ~PLT_UPnP_DeviceStartIterator() {}

    NPT_Result operator()(PLT_DeviceHostReference& device_host) const {
        NPT_CHECK_SEVERE(device_host->Start(m_TaskManager, device_host));
        return m_ListenTask->AddListener(device_host.AsPointer());
    }

private:
    PLT_TaskManager*    m_TaskManager;
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
        m_ListenTask->RemoveListener(device_host.AsPointer());
        return device_host->Stop();
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
    NPT_CHECK_SEVERE(StartTask(m_SsdpListenTask));

    /* start devices & ctrlpoints */
    m_CtrlPoints.Apply(PLT_UPnP_CtrlPointStartIterator(this, m_SsdpListenTask));
    m_Devices.Apply(PLT_UPnP_DeviceStartIterator(this, m_SsdpListenTask));

    m_Started = true;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_UPnP::Stop
+---------------------------------------------------------------------*/
NPT_Result
PLT_UPnP::Stop()
{
    NPT_LOG_INFO("Stopping UPnP...");

    // override here to cleanup ctrlpoints and devices
    // before tasks are stopped since they might internally
    // still have references to tasks and will try to stop them
    // manually

    NPT_AutoLock lock(m_Lock);

    if (m_Started == false) return NPT_FAILURE;

    m_CtrlPoints.Apply(PLT_UPnP_CtrlPointStopIterator(m_SsdpListenTask));
    m_Devices.Apply(PLT_UPnP_DeviceStopIterator(m_SsdpListenTask));

    StopAllTasks();
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
        NPT_Result res = device->Start(this, device);
        if (NPT_SUCCEEDED(res)) {
            // add listener after device is started since it
            // needs to be aware of the task manager (this)
            m_SsdpListenTask->AddListener(device.AsPointer());
        } else {
            NPT_LOG_SEVERE("Failed to start Device...");
            return res;
        }
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

    // HACK: we keep the device in the list just in case it's being used by threads/tasks
    // without a reference
    // Should not be done here, can be done by called if it want's to, otherwise 
    // there is no way to cleanup properly
    m_SsdpListenTask->RemoveListener(device.AsPointer());
    m_Devices.Remove(device);
    return device->Stop();
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
        NPT_Result res = ctrl_point->Start(this);
        if (NPT_SUCCEEDED(res)) {
            // add listener after ctrl point is started since it
            // needs to be aware of the task manager (this)
            m_SsdpListenTask->AddListener(ctrl_point.AsPointer());
        } else {
            NPT_LOG_SEVERE("Failed to start CtrlPoint ...");
            return res;
        }
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

    // HACK: we keep the ctrl point in the list just in case it's being used by threads/tasks
    // without a reference
    return ctrl_point->Stop();
}

