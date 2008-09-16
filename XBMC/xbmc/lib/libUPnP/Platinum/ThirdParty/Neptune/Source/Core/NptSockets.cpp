/*****************************************************************
|
|   Neptune - Sockets
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
|   NPT_SocketAddress::ToString
+---------------------------------------------------------------------*/
NPT_String
NPT_SocketAddress::ToString() const
{
    NPT_String s = m_IpAddress.ToString();
    s += ':';
    s += NPT_String::FromInteger(m_Port);
    return s;
}

/*----------------------------------------------------------------------
|   NPT_SocketAddress::operator==
+---------------------------------------------------------------------*/
bool
NPT_SocketAddress::operator==(const NPT_SocketAddress& other) const
{
    return (other.GetIpAddress().AsLong() == m_IpAddress.AsLong() && 
            other.GetPort() == m_Port);
}

