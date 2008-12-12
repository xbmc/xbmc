/*****************************************************************
|
|   Platinum - Device Data
|
|   Copyright (c) 2004-2008, Plutinosoft, LLC.
|   Author: Sylvain Rebaud (sylvain@plutinosoft.com)
|
****************************************************************/

#ifndef _PLT_DEVICE_DATA_H_
#define _PLT_DEVICE_DATA_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "Neptune.h"

/*----------------------------------------------------------------------
|   forward declarations
+---------------------------------------------------------------------*/
class PLT_Service;
class PLT_DeviceData;

typedef NPT_Reference<PLT_DeviceData> PLT_DeviceDataReference;
typedef NPT_List<PLT_DeviceDataReference> PLT_DeviceDataReferenceList;

/*----------------------------------------------------------------------
|   PLT_DeviceIcon class
+---------------------------------------------------------------------*/
typedef struct {
    NPT_String  mimetype;
    NPT_Int32   width;
    NPT_Int32   height;
    NPT_Int32   depth;
    NPT_String  url;
} PLT_DeviceIcon;

/*----------------------------------------------------------------------
|   PLT_DeviceData class
+---------------------------------------------------------------------*/
class PLT_DeviceData
{
public:
    PLT_DeviceData(
        NPT_HttpUrl      description_url = NPT_HttpUrl(NULL, 0, "/"), 
        const char*      uuid = "",
        NPT_TimeInterval lease_time = NPT_TimeInterval(1800, 0),
        const char*      device_type = "",
        const char*      friendly_name = "");

    /* Getters */
    virtual NPT_Result  GetDescription(NPT_String& desc);
    virtual NPT_String  GetDescriptionUrl(const char* bind_addr = NULL);
    virtual NPT_HttpUrl GetURLBase();
    virtual NPT_Result  GetDescription(NPT_XmlElementNode* parent, NPT_XmlElementNode** device = NULL);
    virtual NPT_String  GetIconUrl(const char* mimetype = NULL, NPT_Int32 maxsize = 0, NPT_Int32 maxdepth = 0);

    const NPT_TimeInterval& GetLeaseTime()    const { return m_LeaseTime;        }
    const NPT_String&   GetUUID()             const { return m_UUID;             }
    const NPT_String&   GetFriendlyName()     const { return m_FriendlyName;     }
    const NPT_String&   GetType()             const { return m_DeviceType;       }
    const NPT_String&   GetModelDescription() const { return m_ModelDescription; }
    const NPT_String&   GetParentUUID()       const { return m_ParentUUID;       }

    const NPT_Array<PLT_Service*>&            GetServices()        const { return m_Services; }
    const NPT_Array<PLT_DeviceDataReference>& GetEmbeddedDevices() const { return m_EmbeddedDevices; }

    NPT_Result FindEmbeddedDeviceByType(const char* type, PLT_DeviceDataReference& device);
    NPT_Result FindServiceById(const char* id, PLT_Service*& service);
    NPT_Result FindServiceByType(const char* type, PLT_Service*& service);
    NPT_Result FindServiceByDescriptionURI(const char* uri, PLT_Service*& service);
    NPT_Result FindServiceByControlURI(const char* uri, PLT_Service*& service);
    NPT_Result FindServiceByEventSubURI(const char* uri, PLT_Service*& service);

    /* called by PLT_Device subclasses */
    NPT_Result AddDevice(PLT_DeviceDataReference& device);
    NPT_Result AddService(PLT_Service* service);

    NPT_Result ToLog(int level = NPT_LOG_LEVEL_FINE);

protected:
    virtual NPT_Result OnAddExtraInfo(NPT_XmlElementNode* /*device_node*/) { return NPT_SUCCESS; }

private:
    /* called by PLT_CtrlPoint when new device is discovered */
    NPT_Result    SetURLBase(NPT_HttpUrl& url_base);
    NPT_TimeStamp GetLeaseTimeLastUpdate();
    NPT_Result    SetLeaseTime(NPT_TimeInterval lease_time);
    NPT_Result    SetDescription(const char*          szDescription, 
                                 const NPT_IpAddress& local_iface_ip);
    NPT_Result    SetDescriptionDevice(NPT_XmlElementNode* device_node);

public:
    NPT_String m_Manufacturer;
    NPT_String m_ManufacturerURL;
    NPT_String m_ModelDescription;
    NPT_String m_ModelName;
    NPT_String m_ModelNumber;
    NPT_String m_ModelURL;
    NPT_String m_SerialNumber;
    NPT_String m_UPC;
    NPT_String m_PresentationURL;
    NPT_String m_DlnaDoc;

protected:
    virtual ~PLT_DeviceData();

    friend class NPT_Reference<PLT_DeviceData>;
    friend class PLT_DeviceDataFinder;
    friend class PLT_DeviceDataFinderByType;
    friend class PLT_CtrlPoint;
    friend class PLT_DeviceReadyIterator;
    friend class PLT_DeviceHost;

    //members
    NPT_String                         m_ParentUUID;
    NPT_String                         m_UUID;
    NPT_HttpUrl                        m_URLDescription;
    NPT_String                         m_URLBasePath;
    NPT_String                         m_DeviceType;
    NPT_String                         m_FriendlyName;
    NPT_TimeInterval                   m_LeaseTime;
    NPT_TimeStamp                      m_LeaseTimeLastUpdate;
    NPT_Array<PLT_Service*>            m_Services;
    NPT_Array<PLT_DeviceDataReference> m_EmbeddedDevices;
    NPT_Array<PLT_DeviceIcon>          m_Icons;

    /* IP address of interface used when retrieving device description.
       We need the info for the control point subscription callback */
    NPT_IpAddress                      m_LocalIfaceIp; 
};

/*----------------------------------------------------------------------
|   PLT_DeviceDataFinder
+---------------------------------------------------------------------*/
class PLT_DeviceDataFinder
{
public:
    // methods
    PLT_DeviceDataFinder(const char* uuid) : m_UUID(uuid) {}
    virtual ~PLT_DeviceDataFinder() {}

    bool operator()(const PLT_DeviceDataReference& data) const {
        return data->m_UUID.Compare(m_UUID, true) ? false : true;
    }

private:
    // members
    NPT_String m_UUID;
};

/*----------------------------------------------------------------------
|   PLT_DeviceDataFinderByType
+---------------------------------------------------------------------*/
class PLT_DeviceDataFinderByType
{
public:
    // methods
    PLT_DeviceDataFinderByType(const char* type) : m_Type(type) {}
    virtual ~PLT_DeviceDataFinderByType() {}

    bool operator()(const PLT_DeviceDataReference& data) const {
        return data->m_DeviceType.Compare(m_Type, true) ? false : true;
    }

private:
    // members
    NPT_String m_Type;
};

#endif /* _PLT_DEVICE_DATA_H_ */

