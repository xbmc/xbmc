/*****************************************************************
|
|   Neptune - Network
|
| Copyright (c) 2002-2016, Axiomatic Systems, LLC.
| All rights reserved.
|
| Redistribution and use in source and binary forms, with or without
| modification, are permitted provided that the following conditions are met:
|     * Redistributions of source code must retain the above copyright
|       notice, this list of conditions and the following disclaimer.
|     * Redistributions in binary form must reproduce the above copyright
|       notice, this list of conditions and the following disclaimer in the
|       documentation and/or other materials provided with the distribution.
|     * Neither the name of Axiomatic Systems nor the
|       names of its contributors may be used to endorse or promote products
|       derived from this software without specific prior written permission.
|
| THIS SOFTWARE IS PROVIDED BY AXIOMATIC SYSTEMS ''AS IS'' AND ANY
| EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
| WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
| DISCLAIMED. IN NO EVENT SHALL AXIOMATIC SYSTEMS BE LIABLE FOR ANY
| DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
| (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
| LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
| ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
| (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
| SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
|
 ****************************************************************/

/*----------------------------------------------------------------------
|   includes
+---------------------------------------------------------------------*/
#include "NptSockets.h"
#include "NptUtils.h"

/*----------------------------------------------------------------------
|   NPT_IpAddress::NPT_IpAddress
+---------------------------------------------------------------------*/
NPT_IpAddress::NPT_IpAddress() :
    m_Type(IPV4),
    m_ScopeId(0)
{
    NPT_SetMemory(m_Address, 0, sizeof(m_Address));
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::NPT_IpAddress
+---------------------------------------------------------------------*/
NPT_IpAddress::NPT_IpAddress(Type type) :
    m_Type(type),
    m_ScopeId(0)
{
    NPT_SetMemory(m_Address, 0, sizeof(m_Address));
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::NPT_IpAddress
+---------------------------------------------------------------------*/
NPT_IpAddress::NPT_IpAddress(unsigned long address) :
    m_Type(IPV4),
    m_ScopeId(0)
{
    Set(address);
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::NPT_IpAddress
+---------------------------------------------------------------------*/
NPT_IpAddress::NPT_IpAddress(unsigned char a, 
                             unsigned char b, 
                             unsigned char c, 
                             unsigned char d) :
    m_Type(IPV4),
    m_ScopeId(0)
{
    NPT_SetMemory(&m_Address[0], 0, sizeof(m_Address));
    m_Address[0] = a;
    m_Address[1] = b;
    m_Address[2] = c;
    m_Address[3] = d;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::NPT_IpAddress
+---------------------------------------------------------------------*/
NPT_IpAddress::NPT_IpAddress(Type type, const unsigned char* address, unsigned int size, NPT_UInt32 scope_id) :
    m_Type(type),
    m_ScopeId(scope_id)
{
    if (type == IPV6 && size == 16) {
        NPT_CopyMemory(&m_Address[0], address, 16);
    } else if (type == IPV4 && size == 4) {
        NPT_CopyMemory(&m_Address[0], address, 4);
        NPT_SetMemory(&m_Address[4], 0, 12);
        m_ScopeId = 0;
    } else {
        NPT_SetMemory(&m_Address[0], 0, 16);
        m_ScopeId = 0;
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
|   NPT_IpAddress::Set
+---------------------------------------------------------------------*/
NPT_Result    
NPT_IpAddress::Set(const unsigned char bytes[4])
{
    m_Type = IPV4;
    m_Address[0] = bytes[0];
    m_Address[1] = bytes[1];
    m_Address[2] = bytes[2];
    m_Address[3] = bytes[3];
    NPT_SetMemory(&m_Address[4], 0, sizeof(m_Address)-4);
    m_ScopeId = 0; // always 0 for IPv4
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::Set
+---------------------------------------------------------------------*/
NPT_Result    
NPT_IpAddress::Set(unsigned long address)
{
    m_Type = IPV4;
    m_Address[0] = (unsigned char)((address >> 24) & 0xFF);
    m_Address[1] = (unsigned char)((address >> 16) & 0xFF);
    m_Address[2] = (unsigned char)((address >>  8) & 0xFF);
    m_Address[3] = (unsigned char)((address      ) & 0xFF);
    NPT_SetMemory(&m_Address[4], 0, sizeof(m_Address)-4);
    m_ScopeId = 0; // always 0 for IPv4
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::Set
+---------------------------------------------------------------------*/
NPT_Result    
NPT_IpAddress::Set(const unsigned char* bytes, unsigned int size, NPT_UInt32 scope_id)
{
    NPT_SetMemory(&m_Address[0], 0, sizeof(m_Address));
    if (size == 4) {
        m_Type = IPV4;
        NPT_CopyMemory(&m_Address[0], bytes, 4);
        m_ScopeId = 0; // always 0 for IPv4
    } else if (size == 16) {
        m_Type = IPV6;
        NPT_CopyMemory(&m_Address[0], bytes, 16);
        m_ScopeId = scope_id;
    } else {
        return NPT_ERROR_INVALID_PARAMETERS;
    }
    
    return NPT_SUCCESS;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::operator==
+---------------------------------------------------------------------*/
bool
NPT_IpAddress::operator==(const NPT_IpAddress& other) const
{
    unsigned int bytes_to_check = (m_Type == IPV4)?4:16;
    for (unsigned int i=0; i<bytes_to_check; i++) {
        if (m_Address[i] != other.m_Address[i]) {
            return false;
        }
    }
    return m_Type == other.m_Type;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::ToUrlHost
+---------------------------------------------------------------------*/
NPT_String
NPT_IpAddress::ToUrlHost() const
{
    if (m_Type == IPV6) {
        NPT_String result = "[";
        result += ToString();
        return result+"]";
    } else {
        return ToString();
    }
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::IsUnspecified
+---------------------------------------------------------------------*/
bool
NPT_IpAddress::IsUnspecified() const
{
    for (unsigned int i=0; i<(unsigned int)(m_Type==IPV4?4:16); i++) {
        if (m_Address[i]) return false;
    }
    return true;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::IsLooppack
+---------------------------------------------------------------------*/
bool
NPT_IpAddress::IsLooppack() const
{
    if (m_Type == IPV4) {
        return m_Address[0] == 127 &&
               m_Address[1] == 0   &&
               m_Address[2] == 0   &&
               m_Address[3] == 1;
    } else {
        return m_Address[ 0] == 0 &&
               m_Address[ 1] == 0 &&
               m_Address[ 2] == 0 &&
               m_Address[ 3] == 0 &&
               m_Address[ 4] == 0 &&
               m_Address[ 5] == 0 &&
               m_Address[ 6] == 0 &&
               m_Address[ 7] == 0 &&
               m_Address[ 8] == 0 &&
               m_Address[ 9] == 0 &&
               m_Address[10] == 0 &&
               m_Address[11] == 0 &&
               m_Address[12] == 0 &&
               m_Address[13] == 0 &&
               m_Address[14] == 0 &&
               m_Address[15] == 1;
    }
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::IsV4Compatible
+---------------------------------------------------------------------*/
bool
NPT_IpAddress::IsV4Compatible() const
{
    if (m_Type == IPV4) return true;
    return m_Address[ 0] == 0 &&
           m_Address[ 1] == 0 &&
           m_Address[ 2] == 0 &&
           m_Address[ 3] == 0 &&
           m_Address[ 4] == 0 &&
           m_Address[ 5] == 0 &&
           m_Address[ 6] == 0 &&
           m_Address[ 7] == 0 &&
           m_Address[ 8] == 0 &&
           m_Address[ 9] == 0 &&
           m_Address[10] == 0 &&
           m_Address[11] == 0 &&
           !(m_Address[12] == 0 &&
             m_Address[13] == 0 &&
             m_Address[14] == 0 &&
             m_Address[15] == 0) &&
           !(m_Address[12] == 0 &&
             m_Address[13] == 0 &&
             m_Address[14] == 0 &&
             m_Address[15] == 1);
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::IsV4Mapped
+---------------------------------------------------------------------*/
bool
NPT_IpAddress::IsV4Mapped() const
{
    if (m_Type == IPV4) return false;
    return m_Address[ 0] == 0 &&
           m_Address[ 1] == 0 &&
           m_Address[ 2] == 0 &&
           m_Address[ 3] == 0 &&
           m_Address[ 4] == 0 &&
           m_Address[ 5] == 0 &&
           m_Address[ 6] == 0 &&
           m_Address[ 7] == 0 &&
           m_Address[ 8] == 0 &&
           m_Address[ 9] == 0 &&
           m_Address[10] == 0xFF &&
           m_Address[11] == 0xFF;
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::IsLinkLocal
+---------------------------------------------------------------------*/
bool
NPT_IpAddress::IsLinkLocal() const
{
    if (m_Type == IPV4) {
        return m_Address[0] == 169 && m_Address[1] == 254;
    } else {
        return m_Address[0] == 0xFE && ((m_Address[1]&0xC0) == 0x80);
    }
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::IsSiteLocal
+---------------------------------------------------------------------*/
bool
NPT_IpAddress::IsSiteLocal() const
{
    if (m_Type == IPV4) return false;
    return m_Address[0] == 0xFE && ((m_Address[1]&0xC0) == 0xC0);
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::IsUniqueLocal
+---------------------------------------------------------------------*/
bool
NPT_IpAddress::IsUniqueLocal() const
{
    if (m_Type == IPV4) {
        return (m_Address[0] == 10) ||
               (m_Address[0] == 172 && (m_Address[1]&0xF0) == 16) ||
               (m_Address[0] == 192 && m_Address[1] == 168);
    } else {
        return ((m_Address[0] & 0xFE) == 0xFC);
    }
}

/*----------------------------------------------------------------------
|   NPT_IpAddress::IsMulticast
+---------------------------------------------------------------------*/
bool
NPT_IpAddress::IsMulticast() const
{
    if (m_Type == IPV4) {
        return (m_Address[0] & 0xF0) == 224;
    } else {
        return m_Address[0] == 0xFF;
    }
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
|   NPT_NetworkInterface::NPT_NetworkInterface
+---------------------------------------------------------------------*/ 
NPT_NetworkInterface::NPT_NetworkInterface(const char* name,
                                           NPT_Flags   flags) :
    m_Name(name),
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


