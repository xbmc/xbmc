/*****************************************************************
|
|   Platinum - Device Data
|
|   Copyright (c) 2004-2006 Sylvain Rebaud
|   Author: Sylvain Rebaud (sylvain@rebaud.com)
|
****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "PltDeviceData.h"
#include "PltService.h"
#include "PltUPnP.h"
#include "PltXmlHelper.h"

NPT_SET_LOCAL_LOGGER("platinum.core.devicedata")

/*----------------------------------------------------------------------
|   PLT_DeviceData::PLT_DeviceData
+---------------------------------------------------------------------*/
PLT_DeviceData::PLT_DeviceData(NPT_HttpUrl      description_url,
                               const char*      uuid,
                               NPT_TimeInterval lease_time,
                               const char*      device_type,
                               const char*      friendly_name) :
    m_Root(true),
    m_UUID(uuid),
    m_URLDescription(description_url),
    m_DeviceType(device_type),
    m_FriendlyName(friendly_name)
{
    if (uuid == NULL || strlen(uuid) == 0) {
        PLT_UPnPMessageHelper::GenerateGUID(m_UUID);
    }

    SetLeaseTime(lease_time);
    SetURLBase(m_URLDescription);
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::~PLT_DeviceData
+---------------------------------------------------------------------*/
PLT_DeviceData::~PLT_DeviceData()
{
    m_Services.Apply(NPT_ObjectDeleter<PLT_Service>());
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::SetDescriptionUrl
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::SetDescriptionUrl(NPT_HttpUrl& url)
{
    NPT_CHECK_FATAL(SetURLBase(url));
    m_URLDescription = url;
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::GetDescriptionUrl
+---------------------------------------------------------------------*/
NPT_String
PLT_DeviceData::GetDescriptionUrl(const char* bind_addr)
{
    NPT_HttpUrl url = m_URLDescription;

    // override host
    if (bind_addr) url.SetHost(bind_addr);
    return url.ToString();
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::SetURLBase
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::SetURLBase(NPT_HttpUrl& url) 
{
    // we're assuming param passed to be relative (Host/port not different than description)
    m_URLBasePath = url.GetPath();

    // remove trailing file according to RFC 2396
    if (!m_URLBasePath.EndsWith("/")) {
        int index = m_URLBasePath.ReverseFind('/');
        if (index < 0) return NPT_FAILURE;
        m_URLBasePath.SetLength(index+1);
    }

    return NPT_SUCCESS;
}    

/*----------------------------------------------------------------------
|   PLT_DeviceData::GetURLBase
+---------------------------------------------------------------------*/
NPT_HttpUrl
PLT_DeviceData::GetURLBase()
{
    return NPT_HttpUrl(m_URLDescription.GetHost(),
                       m_URLDescription.GetPort(),
                       m_URLBasePath);
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::GetIconUrl
+---------------------------------------------------------------------*/
NPT_String
PLT_DeviceData::GetIconUrl(const char* mimetype, 
                           NPT_Integer maxsize, 
                           NPT_Integer maxdepth)
{
    PLT_DeviceIcon icon;
    icon.depth  = 0;
    icon.height = 0;
    icon.width  = 0;

    for (NPT_Cardinal i=0; i<m_Icons.GetItemCount(); i++) {
        if (mimetype && m_Icons[i].mimetype != mimetype ||
            maxsize  && m_Icons[i].width > maxsize      ||
            maxsize  && m_Icons[i].height > maxsize     ||
            maxdepth && m_Icons[i].depth > maxdepth)
            continue;

        // pick the biggest and better resolution we can
        if (icon.width  >= m_Icons[i].width  ||
            icon.height >= m_Icons[i].height ||
            icon.depth  >= m_Icons[i].depth  ||
            m_Icons[i].url.IsEmpty())
            continue;

        icon = m_Icons[i];
    }

    if (icon.url == "") return "";

    // make absolut path from url base if necessary
    NPT_HttpUrl url(icon.url);
    if (!url.IsValid()) {
        url = GetURLBase();
        if (icon.url.StartsWith("/")) {
            url.SetPathPlus(icon.url);
        } else {
            url.SetPathPlus(url.GetPath() + icon.url);
        }
    }

    return url.ToString();
}
/*----------------------------------------------------------------------
|   PLT_DeviceData::SetLeaseTime
+---------------------------------------------------------------------*/
NPT_Result   
PLT_DeviceData::SetLeaseTime(NPT_TimeInterval lease_time) 
{
    m_LeaseTime = lease_time;
    NPT_System::GetCurrentTimeStamp(m_LeaseTimeLastUpdate);
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::GetLeaseTimeLastUpdate
+---------------------------------------------------------------------*/
NPT_TimeStamp 
PLT_DeviceData::GetLeaseTimeLastUpdate() 
{
    return m_LeaseTimeLastUpdate;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::ToLog
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::ToLog(int level /* = NPT_LOG_LEVEL_FINE */)
{
    NPT_COMPILER_UNUSED(level);

    NPT_StringOutputStreamReference stream(new NPT_StringOutputStream);
    stream->WriteString("Device GUID: ");
    stream->WriteString((const char*)m_UUID);

    stream->WriteString("Device Type: ");
    stream->WriteString((const char*)m_DeviceType);

    stream->WriteString("Device Base Url: ");
    stream->WriteString((const char*)GetURLBase().ToString());

    stream->WriteString("Device Friendly Name: ");
    stream->WriteString((const char*)m_FriendlyName);

    NPT_LOG_1(level, "%s", (const char*)stream->GetString());
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::AddService
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::AddService(PLT_Service* service)
{
    if (service->m_ServiceType == "" ||
        service->m_ServiceID   == "" ||
        service->m_SCPDURL     == "" ||
        service->m_ControlURL  == "" ||
        service->m_EventSubURL == "") {
        return NPT_ERROR_INVALID_PARAMETERS;
    }

    return m_Services.Add(service);
}

/*----------------------------------------------------------------------
|   PLT_GetDescriptionIterator class
+---------------------------------------------------------------------*/
template <class T>
class PLT_GetDescriptionIterator
{
public:
    PLT_GetDescriptionIterator<T>(NPT_XmlElementNode* parent) :
      m_Parent(parent) {}

      NPT_Result operator()(T& data) const {
          return data->GetDescription(m_Parent);
      }

private:
    NPT_XmlElementNode* m_Parent;
};

/*----------------------------------------------------------------------
|   PLT_DeviceData::ToXML
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::GetDescription(NPT_XmlElementNode* root, NPT_XmlElementNode** device_out)
{
    NPT_XmlElementNode* device = new NPT_XmlElementNode("device");
    if (device_out) {
        *device_out = device;
    }

    NPT_CHECK_SEVERE(root->AddChild(device));

    // device properties
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(device, "deviceType", m_DeviceType));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(device, "friendlyName", m_FriendlyName));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(device, "manufacturer", m_Manufacturer));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(device, "manufacturerURL", m_ManufacturerURL));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(device, "modelDescription", m_ModelDescription));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(device, "modelName", m_ModelName));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(device, "modelURL", m_ModelURL));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(device, "modelNumber", m_ModelNumber));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(device, "serialNumber", m_SerialNumber));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(device, "UDN", "uuid:" + m_UUID));
    if (!m_PresentationURL.IsEmpty()) NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(device, "presentationURL", m_PresentationURL));

    if (!m_DlnaDoc.IsEmpty()) {
        NPT_XmlElementNode* dlnadoc = new NPT_XmlElementNode("dlna", "X_DLNADOC");
        NPT_CHECK_SEVERE(dlnadoc->SetNamespaceUri("dlna", "urn:schemas-dlna-org:device-1-0"));
        dlnadoc->AddText(m_DlnaDoc);
        device->AddChild(dlnadoc);
    }

    // icons
    if (m_Icons.GetItemCount()) {
        NPT_XmlElementNode* icons = new NPT_XmlElementNode("iconList");
        NPT_CHECK_SEVERE(device->AddChild(icons));
        for (NPT_Cardinal i=0; i<m_Icons.GetItemCount(); i++) {
            NPT_XmlElementNode* icon = new NPT_XmlElementNode("icon");
            NPT_CHECK_SEVERE(icons->AddChild(icon));
            NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(icon, "mimetype", m_Icons[i].mimetype));
            NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(icon, "width", NPT_String::FromInteger(m_Icons[i].width)));
            NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(icon, "height", NPT_String::FromInteger(m_Icons[i].height)));
            NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(icon, "depth", NPT_String::FromInteger(m_Icons[i].depth)));
            NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(icon, "url", m_Icons[i].url));
        }
    }

    // services
    NPT_XmlElementNode* services = new NPT_XmlElementNode("serviceList");
    NPT_CHECK_SEVERE(device->AddChild(services));
    NPT_CHECK_SEVERE(m_Services.ApplyUntil(PLT_GetDescriptionIterator<PLT_Service*>(services), 
                                           NPT_UntilResultNotEquals(NPT_SUCCESS)));

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::GetDescription
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::GetDescription(NPT_String& desc)
{
    NPT_XmlElementNode* root = new NPT_XmlElementNode("root");
    NPT_CHECK_SEVERE(root->SetNamespaceUri("", "urn:schemas-upnp-org:device-1-0"));

    // add spec version
    NPT_XmlElementNode* spec = new NPT_XmlElementNode("specVersion");
    NPT_CHECK_SEVERE(root->AddChild(spec));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(spec, "major", "1"));
    NPT_CHECK_SEVERE(PLT_XmlHelper::AddChildText(spec, "minor", "0"));

    // get device xml
    NPT_CHECK_SEVERE(GetDescription(root));

    // serialize node
    NPT_CHECK_SEVERE(PLT_XmlHelper::Serialize(*root, desc));
    delete root;

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::SetDescription
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::SetDescription(const char* description)
{
    NPT_XmlParser parser;
    NPT_XmlNode*  tree = NULL;
    NPT_Result    res;

    res = parser.Parse(description, tree);
    if (NPT_FAILED(res)) {
        delete tree;
        return res;
    }

    NPT_XmlElementNode* root = tree->AsElementNode();
    if (!root || root->GetTag() != "root" || !root->GetNamespace() || *root->GetNamespace() != "urn:schemas-upnp-org:device-1-0") {
        delete tree;
        return NPT_FAILURE;
    }

    // look for optional URLBase element
    NPT_String URLBase;
    if (NPT_SUCCEEDED(PLT_XmlHelper::GetChildText(root, "URLBase", URLBase))) {
        NPT_HttpUrl url(URLBase);
        if (!url.IsValid()) return NPT_FAILURE;

        SetURLBase(url);
    }

    // at least one root device child element is required
    NPT_XmlElementNode* device;
    if (!(device = PLT_XmlHelper::GetChild(root, "device"))) {
        delete tree;
        return NPT_FAILURE;
    }

    res = SetDescriptionDevice(device);

    // delete the tree
    delete tree;
    return res;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::SetDescriptionDevice
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::SetDescriptionDevice(NPT_XmlElementNode* device_node)
{
    NPT_Result res;

    NPT_CHECK_SEVERE(PLT_XmlHelper::GetChildText(device_node, "deviceType", m_DeviceType));
    NPT_CHECK_SEVERE(PLT_XmlHelper::GetChildText(device_node, "UDN", m_UUID));

    // jump uuid:, we should probably check first
    m_UUID = ((const char*)m_UUID)+5;

    // optional attributes
    PLT_XmlHelper::GetChildText(device_node, "friendlyName", m_FriendlyName);
    PLT_XmlHelper::GetChildText(device_node, "manufacturer", m_Manufacturer);
    PLT_XmlHelper::GetChildText(device_node, "manufacturerURL", m_ManufacturerURL);
    PLT_XmlHelper::GetChildText(device_node, "modelDescription", m_ModelDescription);
    PLT_XmlHelper::GetChildText(device_node, "modelName", m_ModelName);
    PLT_XmlHelper::GetChildText(device_node, "modelNumber", m_ModelNumber);
    PLT_XmlHelper::GetChildText(device_node, "serialNumber", m_SerialNumber);

    // enumerate icons
    NPT_XmlElementNode* iconList = PLT_XmlHelper::GetChild(device_node, "iconList");
    if (iconList) {
        NPT_Array<NPT_XmlElementNode*> icons;
        PLT_XmlHelper::GetChildren(iconList, icons, "icon");

        for (NPT_Cardinal k=0 ; k<icons.GetItemCount(); k++) {
            PLT_DeviceIcon icon;
            NPT_String integer, height, depth;

            PLT_XmlHelper::GetChildText(icons[k], "mimetype", icon.mimetype);
            PLT_XmlHelper::GetChildText(icons[k], "url", icon.url);

            if(NPT_SUCCEEDED(PLT_XmlHelper::GetChildText(icons[k], "width", integer)))
                NPT_ParseInteger32(integer, icon.width);
            if(NPT_SUCCEEDED(PLT_XmlHelper::GetChildText(icons[k], "height", integer)))
                NPT_ParseInteger32(integer, icon.height);
            if(NPT_SUCCEEDED(PLT_XmlHelper::GetChildText(icons[k], "depth", integer)))
                NPT_ParseInteger32(integer, icon.depth);

            m_Icons.Add(icon);
        }
    }

    // enumerate device services
    NPT_XmlElementNode* serviceList = PLT_XmlHelper::GetChild(device_node, "serviceList");
    if (serviceList) {
        NPT_Array<NPT_XmlElementNode*> services;
        PLT_XmlHelper::GetChildren(serviceList, services, "service");
        for( int k = 0 ; k < (int)services.GetItemCount(); k++) {    
            PLT_Service* service = new PLT_Service(this);
            PLT_XmlHelper::GetChildText(services[k], "serviceType", service->m_ServiceType);
            PLT_XmlHelper::GetChildText(services[k], "serviceId", service->m_ServiceID);
            PLT_XmlHelper::GetChildText(services[k], "SCPDURL", service->m_SCPDURL);
            PLT_XmlHelper::GetChildText(services[k], "controlURL", service->m_ControlURL);
            PLT_XmlHelper::GetChildText(services[k], "eventSubURL", service->m_EventSubURL);
            if (NPT_FAILED(res = AddService(service))) {
                delete service;
                return res;
            }
        }
    }

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::FindServiceById
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::FindServiceById(const char* id, PLT_Service*& service)
{
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Services, PLT_ServiceIDFinder(id), service))) {
        return NPT_SUCCESS;
    }

    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::FindServiceByType
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::FindServiceByType(const char* type, PLT_Service*& service)
{
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Services, PLT_ServiceTypeFinder(type), service))) {
        return NPT_SUCCESS;
    }

    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::FindServiceByDescriptionURI
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::FindServiceByDescriptionURI(const char* uri, PLT_Service*& service)
{
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Services, PLT_ServiceSCPDURLFinder(uri), service))) {
        return NPT_SUCCESS;
    }

    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::FindServiceByControlURI
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::FindServiceByControlURI(const char* uri, PLT_Service*& service)
{
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Services, PLT_ServiceControlURLFinder(uri), service))) {
        return NPT_SUCCESS;
    }

    return NPT_FAILURE;
}

/*----------------------------------------------------------------------
|   PLT_DeviceData::FindServiceByEventSubURI
+---------------------------------------------------------------------*/
NPT_Result
PLT_DeviceData::FindServiceByEventSubURI(const char* uri, PLT_Service*& service)
{       
    if (NPT_SUCCEEDED(NPT_ContainerFind(m_Services, PLT_ServiceEventSubURLFinder(uri), service))) {
        return NPT_SUCCESS;
    }

    return NPT_FAILURE;
}
