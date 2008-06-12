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
