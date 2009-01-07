/*****************************************************************
|
|      Neptune - Network :: PSP Implementation
|
|      (c) 2001-2005 Gilles Boccon-Gibod
|      Author: Sylvain Rebaud (sylvain@plutinosoft.com)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "NptNetwork.h"

/*----------------------------------------------------------------------
|       NPT_NetworkInterface::GetNetworkInterfaces
+---------------------------------------------------------------------*/
NPT_Result
NPT_NetworkInterface::GetNetworkInterfaces(NPT_List<NPT_NetworkInterface*>& interfaces)
{
    union SceNetApctlInfo info;
    int ret = sceNetApctlGetInfo(SCE_NET_APCTL_INFO_IP_ADDRESS, &info);
    if (ret < 0) {
        return NPT_FAILURE;
    }
    NPT_IpAddress primary_address;
    if (NPT_FAILED(primary_address.Parse(info.ip_address))) {
        return NPT_FAILURE;
    }

    NPT_IpAddress netmask;
    if (NPT_FAILED(netmask.Parse(info.netmask))) {
        return NPT_FAILURE;
    }

    NPT_IpAddress broadcast_address;
    NPT_Flags    flags = 0;
    flags |= NPT_NETWORK_INTERFACE_FLAG_BROADCAST;
    flags |= NPT_NETWORK_INTERFACE_FLAG_MULTICAST;

    // get mac address
    SceNetEtherAddr mac_info;
    ret = sceNetGetLocalEtherAddr(&mac_info);
    if (ret < 0) {
        return NPT_FAILURE;
    }
    NPT_MacAddress mac(TYPE_IEEE_802_11, mac_info.data, SCE_NET_ETHER_ADDR_LEN);

    // create an interface object
    char iface_name[5];
    iface_name[0] = 'i';
    iface_name[1] = 'f';
    iface_name[2] = '0';
    iface_name[3] = '0';
    iface_name[4] = '\0';
    NPT_NetworkInterface* iface = new NPT_NetworkInterface(iface_name, mac, flags);

    // set the interface address
    NPT_NetworkInterfaceAddress iface_address(
        primary_address,
        broadcast_address,
        NPT_IpAddress::Any,
        netmask);
    iface->AddAddress(iface_address);  

    // add the interface to the list
    interfaces.Add(iface);  

    return NPT_SUCCESS;
}

