/*****************************************************************
|
|      Neptune - HTTP Proxy :: Null Implementation
|
|      (c) 2001-2007 Gilles Boccon-Gibod
|      Author: Gilles Boccon-Gibod (bok@bok.net)
|
****************************************************************/

/*----------------------------------------------------------------------
|       includes
+---------------------------------------------------------------------*/
#include "NptResults.h"
#include "NptHttp.h"

/*----------------------------------------------------------------------
|       NPT_HttpProxySelector::GetDefault
+---------------------------------------------------------------------*/
NPT_HttpProxySelector*
NPT_HttpProxySelector::GetDefault()
{
    return NULL;
}
