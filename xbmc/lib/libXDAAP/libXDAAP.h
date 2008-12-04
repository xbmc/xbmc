#ifdef WIN32
#include <windows.h>
#endif
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "client.h"
#include "httpClient.h"
/*
#include "daap.h"
#include "daap_contentcodes.h"
#include "dmap_generics.h"
#include "Authentication/hasher.h"
*/

#define DAAP_SOCKET_FD_TYPE          SOCKET
#ifdef _LINUX
#define DAAP_SOCKET_CLOSE			close
#else
#define DAAP_SOCKET_CLOSE            closesocket
#endif
#define DAAP_SOCKET_WRITE(s, b, l)   send((s), (b), (l), 0)
#define DAAP_SOCKET_READ(s, b, l)    recv((s), (b), (l), 0)

