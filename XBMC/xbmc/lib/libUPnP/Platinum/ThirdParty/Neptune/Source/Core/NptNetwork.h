/*****************************************************************
|
|   Neptune - Network
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

#ifndef _NPT_NETWORK_H_
#define _NPT_NETWORK_H_

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptTypes.h"
#include "NptConstants.h"
#include "NptStrings.h"
#include "NptList.h"

/*----------------------------------------------------------------------
|   constants
+---------------------------------------------------------------------*/
const unsigned int NPT_NETWORK_MAX_MAC_ADDRESS_LENGTH  = 8;

/*----------------------------------------------------------------------
|   flags
+---------------------------------------------------------------------*/
#define NPT_NETWORK_INTERFACE_FLAG_LOOPBACK       0x01
#define NPT_NETWORK_INTERFACE_FLAG_PROMISCUOUS    0x02
#define NPT_NETWORK_INTERFACE_FLAG_BROADCAST      0x04
#define NPT_NETWORK_INTERFACE_FLAG_MULTICAST      0x08
#define NPT_NETWORK_INTERFACE_FLAG_POINT_TO_POINT 0x10

/*----------------------------------------------------------------------
|   workarounds
+---------------------------------------------------------------------*/
#if defined(_WIN32)
#if defined(SetPort)
#undef SetPort
#endif
#endif

/*----------------------------------------------------------------------
|   types
+---------------------------------------------------------------------*/
typedef unsigned int NPT_IpPort;

/*----------------------------------------------------------------------
|   NPT_IpAddress
+---------------------------------------------------------------------*/
class NPT_IpAddress
{
public:
    // class members
    static NPT_IpAddress Any;

    // constructors and destructor
    NPT_IpAddress();
    NPT_IpAddress(unsigned long address);

    // methods
    NPT_Result       ResolveName(const char* name, 
                                 NPT_Timeout timeout = NPT_TIMEOUT_INFINITE);
    NPT_Result       Parse(const char* name);
    NPT_Result       Set(unsigned long address);
    NPT_Result       Set(const unsigned char bytes[4]);
    const unsigned char* AsBytes() const;
    unsigned long    AsLong() const;
    NPT_String       ToString() const;
    
private:
    // members
    unsigned char m_Address[4];
};

/*----------------------------------------------------------------------
|   NPT_MacAddress
+---------------------------------------------------------------------*/
class NPT_MacAddress
{
public:
    // typedef enum
    typedef enum {
        TYPE_UNKNOWN,
        TYPE_LOOPBACK,
        TYPE_ETHERNET,
        TYPE_PPP,
        TYPE_IEEE_802_11
    } Type;
    
    // constructors and destructor
    NPT_MacAddress() : m_Type(TYPE_UNKNOWN), m_Length(0) {}
    NPT_MacAddress(Type           type,
                   const unsigned char* addr, 
                   unsigned int   length);
    
    // methods
    void                 SetAddress(Type type, const unsigned char* addr,
                                    unsigned int length);
    Type                 GetType() const    { return m_Type; }
    const unsigned char* GetAddress() const { return m_Address; }
    unsigned int         GetLength() const  { return m_Length; }
    NPT_String           ToString() const;
    
private:
    // members
    Type          m_Type;
    unsigned char m_Address[NPT_NETWORK_MAX_MAC_ADDRESS_LENGTH];
    unsigned int  m_Length;
};

/*----------------------------------------------------------------------
|   NPT_NetworkInterfaceAddress
+---------------------------------------------------------------------*/
class NPT_NetworkInterfaceAddress
{
public:
    // constructors and destructor
    NPT_NetworkInterfaceAddress(const NPT_IpAddress& primary,
                                const NPT_IpAddress& broadcast,
                                const NPT_IpAddress& destination,
                                const NPT_IpAddress& netmask) :
        m_PrimaryAddress(primary),
        m_BroadcastAddress(broadcast),
        m_DestinationAddress(destination),
        m_NetMask(netmask) {}

    // methods
    const NPT_IpAddress& GetPrimaryAddress() const {
        return m_PrimaryAddress;
    }
    const NPT_IpAddress& GetBroadcastAddress() const {
        return m_BroadcastAddress;
    }
    const NPT_IpAddress& GetDestinationAddress() const {
        return m_DestinationAddress;
    }
    const NPT_IpAddress& GetNetMask() const {
        return m_NetMask;
    }
    
private:
    // members
    NPT_IpAddress m_PrimaryAddress;
    NPT_IpAddress m_BroadcastAddress;
    NPT_IpAddress m_DestinationAddress;
    NPT_IpAddress m_NetMask;
};

/*----------------------------------------------------------------------
|   NPT_NetworkInterface
+---------------------------------------------------------------------*/
class NPT_NetworkInterface
{
public:
    // class methods
    static NPT_Result GetNetworkInterfaces(NPT_List<NPT_NetworkInterface*>& interfaces);

    // constructors and destructor
    NPT_NetworkInterface(const char*           name,
                         const NPT_MacAddress& mac,
                         NPT_Flags             flags);
   ~NPT_NetworkInterface() {}

    // methods
    NPT_Result AddAddress(const NPT_NetworkInterfaceAddress& address);
    const NPT_String& GetName() const {
        return m_Name;
    }
    const NPT_MacAddress& GetMacAddress() const {
        return m_MacAddress;
    }
    NPT_Flags GetFlags() const { return m_Flags; }
    const NPT_List<NPT_NetworkInterfaceAddress>& GetAddresses() const {
        return m_Addresses;
    }    
    
private:
    // members
    NPT_String                            m_Name;
    NPT_MacAddress                        m_MacAddress;
    NPT_Flags                             m_Flags;
    NPT_List<NPT_NetworkInterfaceAddress> m_Addresses;
};


#endif // _NPT_NETWORK_H_
