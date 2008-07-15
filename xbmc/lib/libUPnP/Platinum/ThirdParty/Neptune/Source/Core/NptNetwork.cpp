/*****************************************************************
|
|   Neptune - Network
|
|   (c) 2001-2006 Gilles Boccon-Gibod
|   Author: Gilles Boccon-Gibod (bok@bok.net)
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptSockets.h"
#include "NptUtils.h"

/*----------------------------------------------------------------------
|   NPT_IpAddress::Any
+---------------------------------------------------------------------*/
NPT_IpAddress NPT_IpAddress::Any;

/*----------------------------------------------------------------------
|   NPT_IpAddress::NPT_IpAddress
+---------------------------------------------------------------------*/
NPT_IpAddress::NPT_IpAddress()
{
    m_Address[0] = m_Address[1] = m_Address[2] = m_Address[3] = 0;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::NPT_IpAddress
+---------------------------------------------------------------------*/
NPT_IpAddress::NPT_IpAddress(unsigned long address)
{
    Set(address);
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::Parse
+---------------------------------------------------------------------*/
NPT_Result
NPT_IpAddress::Parse(const char* name)
{
    // check the name
    if (name == NULL) return NPT_ERROR_INVALID_PARAMETERS;

    // clear the address
    m_Address[0] = m_Address[1] = m_Address[2] = m_Address[3] = 0;

    // parse
    unsigned int  fragment;
    bool          fragment_empty = true;
    unsigned char address[4];
    unsigned int  accumulator;
    for (fragment = 0, accumulator = 0; fragment < 4; ++name) {
        if (*name == '\0' || *name == '.') {
            // fragment terminator
            if (fragment_empty) return NPT_ERROR_INVALID_SYNTAX;
            address[fragment++] = accumulator;
            if (*name == '\0') break;
            accumulator = 0;
            fragment_empty = true;
        } else if (*name >= '0' && *name <= '9') {
            // numerical character
            accumulator = accumulator*10 + (*name - '0');
            if (accumulator > 255) return NPT_ERROR_INVALID_SYNTAX;
            fragment_empty = false; 
        } else {
            // invalid character
            return NPT_ERROR_INVALID_SYNTAX;
        }
    }

    if (fragment == 4 && *name == '\0' && !fragment_empty) {
        m_Address[0] = address[0];
        m_Address[1] = address[1];
        m_Address[2] = address[2];
        m_Address[3] = address[3];
        return NPT_SUCCESS;
    } else {
        return NPT_ERROR_INVALID_SYNTAX;
    }
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::AsLong
+---------------------------------------------------------------------*/
unsigned long
NPT_IpAddress::AsLong() const
{
    return 
        (((unsigned long)m_Address[0])<<24) |
        (((unsigned long)m_Address[1])<<16) |
        (((unsigned long)m_Address[2])<< 8) |
        (((unsigned long)m_Address[3]));
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::AsBytes
+---------------------------------------------------------------------*/
const unsigned char* 
NPT_IpAddress::AsBytes() const
{
    return m_Address;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::ToString
+---------------------------------------------------------------------*/
NPT_String
NPT_IpAddress::ToString() const
{
    NPT_String address;
    address.Reserve(16);
    address += NPT_String::FromInteger(m_Address[0]);
    address += '.';
    address += NPT_String::FromInteger(m_Address[1]);
    address += '.';
    address += NPT_String::FromInteger(m_Address[2]);
    address += '.';
    address += NPT_String::FromInteger(m_Address[3]);

    return address;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::Set
+---------------------------------------------------------------------*/
NPT_Result    
NPT_IpAddress::Set(const unsigned char bytes[4])
{
    m_Address[0] = bytes[0];
    m_Address[1] = bytes[1];
    m_Address[2] = bytes[2];
    m_Address[3] = bytes[3];

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::Set
+---------------------------------------------------------------------*/
NPT_Result    
NPT_IpAddress::Set(unsigned long address)
{
    m_Address[0] = (unsigned char)((address >> 24) & 0xFF);
    m_Address[1] = (unsigned char)((address >> 16) & 0xFF);
    m_Address[2] = (unsigned char)((address >>  8) & 0xFF);
    m_Address[3] = (unsigned char)((address      ) & 0xFF);

    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_MacAddress::NPT_MacAddress
+---------------------------------------------------------------------*/
NPT_MacAddress::NPT_MacAddress(Type                  type,
                               const unsigned char*  address, 
                               unsigned int          length)
{
    SetAddress(type, address, length);
}

/*----------------------------------------------------------------------
|   NPT_MacAddress::SetAddress
+---------------------------------------------------------------------*/
void
NPT_MacAddress::SetAddress(Type                 type,
                           const unsigned char* address, 
                           unsigned int         length)
{
    m_Type = type;
    if (length > NPT_NETWORK_MAX_MAC_ADDRESS_LENGTH) {
        length = NPT_NETWORK_MAX_MAC_ADDRESS_LENGTH;
    }
    m_Length = length;
    for (unsigned int i=0; i<length; i++) {
        m_Address[i] = address[i];
    }
}

/*----------------------------------------------------------------------
|   NPT_MacAddress::ToString
+---------------------------------------------------------------------*/
NPT_String
NPT_MacAddress::ToString() const
{
    NPT_String result;
 
    if (m_Length) {
        char s[3*NPT_NETWORK_MAX_MAC_ADDRESS_LENGTH];
        const char hex[17] = "0123456789abcdef";
        for (unsigned int i=0; i<m_Length; i++) {
            s[i*3  ] = hex[m_Address[i]>>4];
            s[i*3+1] = hex[m_Address[i]&0xf];
            s[i*3+2] = ':';
        }
        s[3*m_Length-1] = '\0';
        result = s;
    }

    return result;
}

/*----------------------------------------------------------------------
|   NPT_NetworkInterface::NPT_NetworkInterface
+---------------------------------------------------------------------*/ 
NPT_NetworkInterface::NPT_NetworkInterface(const char*           name,
                                           const NPT_MacAddress& mac,
                                           NPT_Flags             flags) :
    m_Name(name),
    m_MacAddress(mac),
    m_Flags(flags)
{
}

/*----------------------------------------------------------------------
|   NPT_NetworkInterface::AddAddress
+---------------------------------------------------------------------*/ 
NPT_Result
NPT_NetworkInterface::AddAddress(const NPT_NetworkInterfaceAddress& address)
{
    return m_Addresses.Add(address);
}


