/*****************************************************************
|
|   Platinum - Device Data
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
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
class PLT_DeviceHost;
class PLT_CtrlPoint;
class PLT_DeviceDataFinder;
class PLT_DeviceData;

typedef NPT_Reference<PLT_DeviceData> PLT_DeviceDataReference;
typedef NPT_List<PLT_DeviceDataReference> PLT_DeviceDataReferenceList;

/*----------------------------------------------------------------------
|   PLT_DeviceData class
+---------------------------------------------------------------------*/
class PLT_DeviceData
{
public:
    PLT_DeviceData(
        NPT_HttpUrl      description_url = NPT_HttpUrl(NULL, 0, "/"), 
        const char*      uuid = "",
        NPT_TimeInterval lease_time = NPT_TimeInterval(40, 0),
        const char*      device_type = "",
        const char*      friendly_name = "");

    virtual NPT_Result  GetDescription(NPT_String& desc);
    virtual NPT_String  GetDescriptionUrl(const char* bind_addr = NULL);
    virtual NPT_HttpUrl GetURLBase();
    virtual NPT_Result  GetDescription(NPT_XmlElementNode* parent, NPT_XmlElementNode** device = NULL);

    const NPT_TimeInterval& GetLeaseTime()    const { return m_LeaseTime;    }
    const NPT_String&       GetUUID()         const { return m_UUID;         }
    const NPT_String&       GetFriendlyName() const { return m_FriendlyName; }
    const NPT_String&       GetType()         const { return m_DeviceType;   }


    NPT_Result FindServiceByType(const char* type, PLT_Service*& service);
    NPT_Result FindServiceById(const char* id, PLT_Service*& service);
    NPT_Result FindServiceByDescriptionURI(const char* uri, PLT_Service*& service);
    NPT_Result FindServiceByControlURI(const char* uri, PLT_Service*& service);
    NPT_Result FindServiceByEventSubURI(const char* uri, PLT_Service*& service);

    NPT_Result ToLog(int level = NPT_LOG_LEVEL_FINE);

protected:
    NPT_TimeStamp GetLeaseTimeLastUpdate();
    NPT_Result    SetLeaseTime(NPT_TimeInterval lease_time);
    NPT_Result    SetURLBase(const char* url_base);
    NPT_Result    AddDevice(PLT_DeviceDataReference& device);
    NPT_Result    AddService(PLT_Service* service);
    NPT_Result    SetDescription(const char* szDescription);
    NPT_Result    SetDescriptionUrl(NPT_HttpUrl description_url);


private:
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

protected:
    virtual ~PLT_DeviceData();
    friend class NPT_Reference<PLT_DeviceData>;
    friend class PLT_DeviceDataFinder;
    friend class PLT_CtrlPoint;
    friend class PLT_DeviceReadyIterator;
    friend class PLT_DeviceHost;

    //members
    NPT_AtomicVariable  m_ReferenceCount;
    bool                m_Root;
    NPT_String          m_UUID;
    NPT_HttpUrl         m_URLDescription;
    NPT_String          m_URLBasePath;
    NPT_String          m_DeviceType;
    NPT_String          m_FriendlyName;

    NPT_TimeInterval    m_LeaseTime;
    NPT_TimeStamp       m_LeaseTimeLastUpdate;

    NPT_Array<PLT_Service*>             m_Services;
    NPT_Array<PLT_DeviceDataReference>  m_EmbeddedDevices;
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
    NPT_String   m_UUID;
};

#endif /* _PLT_DEVICE_DATA_H_ */

