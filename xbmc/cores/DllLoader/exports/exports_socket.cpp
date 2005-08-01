
#include "..\..\..\stdafx.h"
#include "..\DllLoaderContainer.h"
#include "emu_socket.h"
#include "..\dll_tracker_socket.h"

void export_wsock32()
{
  g_dlls.wsock32.AddExport("htonl", 8, (unsigned long)htonl);
  g_dlls.wsock32.AddExport("ntohl", 14, (unsigned long)ntohl);
  g_dlls.wsock32.AddExport("recvfrom", 17, (unsigned long)dllrecvfrom);
  g_dlls.wsock32.AddExport("send", 19, (unsigned long)dllsend);
  g_dlls.wsock32.AddExport("sendto", 20, (unsigned long)dllsendto);
  g_dlls.wsock32.AddExport("shutdown", 22, (unsigned long)dllshutdown);
  g_dlls.wsock32.AddExport("socket", 23, (unsigned long)dllsocket, (void*)track_socket);
  g_dlls.wsock32.AddExport("gethostname", 57, (unsigned long)dllgethostname);
  g_dlls.wsock32.AddExport("recv", 16, (unsigned long)dllrecv);
  g_dlls.wsock32.AddExport("inet_addr", 10, (unsigned long)inet_addr);
  g_dlls.wsock32.AddExport("inet_ntoa", 11, (unsigned long)dllinet_ntoa);
  g_dlls.wsock32.AddExport("WSAStartup", 115, (unsigned long)WSAStartup);
  g_dlls.wsock32.AddExport("WSACleanup", 116, (unsigned long)WSACleanup);
  g_dlls.wsock32.AddExport("listen", 13, (unsigned long)dlllisten);
  g_dlls.wsock32.AddExport("connect", 4, (unsigned long)dllconnect);
  g_dlls.wsock32.AddExport("bind", 2, (unsigned long)dllbind);
  g_dlls.wsock32.AddExport("setsockopt", 21, (unsigned long)dllsetsockopt);
  g_dlls.wsock32.AddExport("select", 18, (unsigned long)dllselect);
  g_dlls.wsock32.AddExport("accept", 1, (unsigned long)dllaccept, (void*)track_accept);
  g_dlls.wsock32.AddExport("closesocket", 3, (unsigned long)dllclosesocket, (void*)track_closesocket);
  g_dlls.wsock32.AddExport("ioctlsocket", 12, (unsigned long)dllioctlsocket);
  g_dlls.wsock32.AddExport("ntohs", 15, (unsigned long)ntohs);
  g_dlls.wsock32.AddExport("gethostbyname", 52, (unsigned long)dllgethostbyname);
  g_dlls.wsock32.AddExport("WSAGetLastError", 111, (unsigned long)WSAGetLastError);
  g_dlls.wsock32.AddExport("htons", 9, (unsigned long)htons);
  g_dlls.wsock32.AddExport("__WSAFDIsSet", 151, (unsigned long)dll__WSAFDIsSet);
  g_dlls.wsock32.AddExport("WSASetLastError", 112, (unsigned long)WSASetLastError);
}

/*
    WSOCK32.DLL
    ordinal hint RVA      name
 
       1141    0          AcceptEx (forwarded to MSWSOCK.AcceptEx)
       1111    1          EnumProtocolsA (forwarded to MSWSOCK.EnumProtocolsA)
       1112    2          EnumProtocolsW (forwarded to MSWSOCK.EnumProtocolsW)
       1142    3          GetAcceptExSockaddrs (forwarded to MSWSOCK.GetAcceptExSockaddrs)
       1109    4          GetAddressByNameA (forwarded to MSWSOCK.GetAddressByNameA)
       1110    5          GetAddressByNameW (forwarded to MSWSOCK.GetAddressByNameW)
       1115    6          GetNameByTypeA (forwarded to MSWSOCK.GetNameByTypeA)
       1116    7          GetNameByTypeW (forwarded to MSWSOCK.GetNameByTypeW)
       1119    8          GetServiceA (forwarded to MSWSOCK.GetServiceA)
       1120    9          GetServiceW (forwarded to MSWSOCK.GetServiceW)
       1113    A          GetTypeByNameA (forwarded to MSWSOCK.GetTypeByNameA)
       1114    B          GetTypeByNameW (forwarded to MSWSOCK.GetTypeByNameW)
         24    C          MigrateWinsockConfiguration (forwarded to MSWSOCK.MigrateWinsockConfiguration)
       1130    D          NPLoadNameSpaces (forwarded to MSWSOCK.NPLoadNameSpaces)
       1117    E          SetServiceA (forwarded to MSWSOCK.SetServiceA)
       1118    F          SetServiceW (forwarded to MSWSOCK.SetServiceW)
       1140   10          TransmitFile (forwarded to MSWSOCK.TransmitFile)
        500   11          WEP (forwarded to ws2_32.WEP)
        102   12          WSAAsyncGetHostByAddr (forwarded to ws2_32.WSAAsyncGetHostByAddr)
        103   13          WSAAsyncGetHostByName (forwarded to ws2_32.WSAAsyncGetHostByName)
        105   14          WSAAsyncGetProtoByName (forwarded to ws2_32.WSAAsyncGetProtoByName)
        104   15          WSAAsyncGetProtoByNumber (forwarded to ws2_32.WSAAsyncGetProtoByNumber)
        107   16          WSAAsyncGetServByName (forwarded to ws2_32.WSAAsyncGetServByName)
        106   17          WSAAsyncGetServByPort (forwarded to ws2_32.WSAAsyncGetServByPort)
        101   18          WSAAsyncSelect (forwarded to ws2_32.WSAAsyncSelect)
        108   19          WSACancelAsyncRequest (forwarded to ws2_32.WSACancelAsyncRequest)
        113   1A          WSACancelBlockingCall (forwarded to ws2_32.WSACancelBlockingCall)
        116   1B          WSACleanup (forwarded to ws2_32.WSACleanup)
        111   1C          WSAGetLastError (forwarded to ws2_32.WSAGetLastError)
        114   1D          WSAIsBlocking (forwarded to ws2_32.WSAIsBlocking)
       1107   1E          WSARecvEx (forwarded to MSWSOCK.WSARecvEx)
        109   1F          WSASetBlockingHook (forwarded to ws2_32.WSASetBlockingHook)
        112   20          WSASetLastError (forwarded to ws2_32.WSASetLastError)
        115   21          WSAStartup (forwarded to ws2_32.WSAStartup)
        110   22          WSAUnhookBlockingHook (forwarded to ws2_32.WSAUnhookBlockingHook)
       1000   23          WSApSetPostRoutine (forwarded to ws2_32.WSApSetPostRoutine)
        151   24          __WSAFDIsSet (forwarded to ws2_32.__WSAFDIsSet)
          1   25          accept (forwarded to ws2_32.accept)
          2   26          bind (forwarded to ws2_32.bind)
          3   27          closesocket (forwarded to ws2_32.closesocket)
          4   28          connect (forwarded to ws2_32.connect)
       1106   29          dn_expand (forwarded to MSWSOCK.dn_expand)
         51   2A          gethostbyaddr (forwarded to ws2_32.gethostbyaddr)
         52   2B          gethostbyname (forwarded to ws2_32.gethostbyname)
         57   2C          gethostname (forwarded to ws2_32.gethostname)
       1101   2D          getnetbyname (forwarded to MSWSOCK.getnetbyname)
          5   2E          getpeername (forwarded to ws2_32.getpeername)
         53   2F          getprotobyname (forwarded to ws2_32.getprotobyname)
         54   30          getprotobynumber (forwarded to ws2_32.getprotobynumber)
         55   31          getservbyname (forwarded to ws2_32.getservbyname)
         56   32          getservbyport (forwarded to ws2_32.getservbyport)
          6   33          getsockname (forwarded to ws2_32.getsockname)
          7   34 00002E1D getsockopt
          8   35          htonl (forwarded to ws2_32.htonl)
          9   36          htons (forwarded to ws2_32.htons)
         10   37          inet_addr (forwarded to ws2_32.inet_addr)
       1100   38          inet_network (forwarded to MSWSOCK.inet_network)
         11   39          inet_ntoa (forwarded to ws2_32.inet_ntoa)
         12   3A          ioctlsocket (forwarded to ws2_32.ioctlsocket)
         13   3B          listen (forwarded to ws2_32.listen)
         14   3C          ntohl (forwarded to ws2_32.ntohl)
         15   3D          ntohs (forwarded to ws2_32.ntohs)
       1102   3E          rcmd (forwarded to MSWSOCK.rcmd)
         16   3F 00001020 recv
         17   40 00002E5F recvfrom
       1103   41          rexec (forwarded to MSWSOCK.rexec)
       1104   42          rresvport (forwarded to MSWSOCK.rresvport)
       1108   43          s_perror (forwarded to MSWSOCK.s_perror)
         18   44          select (forwarded to ws2_32.select)
         19   45          send (forwarded to ws2_32.send)
         20   46          sendto (forwarded to ws2_32.sendto)
       1105   47          sethostname (forwarded to MSWSOCK.sethostname)
         21   48 00001072 setsockopt
         22   49          shutdown (forwarded to ws2_32.shutdown)
         23   4A          socket (forwarded to ws2_32.socket)
*/

void export_ws2_32()
{
  g_dlls.ws2_32.AddExport("WSACleanup", 116, (unsigned long)WSACleanup);
  g_dlls.ws2_32.AddExport("WSAGetLastError", 111, (unsigned long)WSAGetLastError);
  g_dlls.ws2_32.AddExport("WSAStartup", 115, (unsigned long)WSAStartup);
  g_dlls.ws2_32.AddExport("bind", 2, (unsigned long)dllbind);
  g_dlls.ws2_32.AddExport("closesocket", 3, (unsigned long)dllclosesocket, (void*)track_closesocket);
  g_dlls.ws2_32.AddExport("connect", 4, (unsigned long)dllconnect);
  g_dlls.ws2_32.AddExport("gethostbyname", 52, (unsigned long)dllgethostbyname);
  g_dlls.ws2_32.AddExport("getsockopt", 7, (unsigned long)dllgetsockopt);
  g_dlls.ws2_32.AddExport("htonl", 8, (unsigned long)htonl);
  g_dlls.ws2_32.AddExport("htons", 9, (unsigned long)htons);
  g_dlls.ws2_32.AddExport("inet_addr", 11, (unsigned long)inet_addr);
  g_dlls.ws2_32.AddExport("inet_ntoa", 12, (unsigned long)dllinet_ntoa);
  g_dlls.ws2_32.AddExport("ioctlsocket", 10, (unsigned long)dllioctlsocket);
  g_dlls.ws2_32.AddExport("ntohl", 14, (unsigned long)ntohl);
  g_dlls.ws2_32.AddExport("recv", 16, (unsigned long)dllrecv);
  g_dlls.ws2_32.AddExport("select", 18, (unsigned long)dllselect);
  g_dlls.ws2_32.AddExport("send", 19, (unsigned long)dllsend);
  g_dlls.ws2_32.AddExport("sendto", 20, (unsigned long)dllsendto);
  g_dlls.ws2_32.AddExport("setsockopt", 21, (unsigned long)dllsetsockopt);
  g_dlls.ws2_32.AddExport("socket", 23, (unsigned long)dllsocket, (void*)track_socket);
  g_dlls.ws2_32.AddExport("accept", 1, (unsigned long)dllaccept, (void*)track_accept);
  g_dlls.ws2_32.AddExport("gethostname", 57, (unsigned long)dllgethostname);
  g_dlls.ws2_32.AddExport("getsockname", 6, (unsigned long)dllgetsockname);
  g_dlls.ws2_32.AddExport("listen", 13, (unsigned long)dlllisten);
  g_dlls.ws2_32.AddExport("ntohs", 15, (unsigned long)ntohs);
  g_dlls.ws2_32.AddExport("recvfrom", 17, (unsigned long)dllrecvfrom);
  g_dlls.ws2_32.AddExport("__WSAFDIsSet", 151, (unsigned long)dll__WSAFDIsSet);
  g_dlls.ws2_32.AddExport("WSASetLastError", 112, (unsigned long)WSASetLastError);
  g_dlls.ws2_32.AddExport("shutdown", 22, (unsigned long)dllshutdown);
  g_dlls.ws2_32.AddExport("getservbyname", 55, (unsigned long)dllgetservbyname);
  g_dlls.ws2_32.AddExport("getprotobyname", 53, (unsigned long)dllgetprotobyname);
  g_dlls.ws2_32.AddExport("getpeername", 5, (unsigned long)dllgetpeername);
  g_dlls.ws2_32.AddExport("getservbyport", 56, (unsigned long)dllgetservbyport);
  g_dlls.ws2_32.AddExport("gethostbyaddr", 51, (unsigned long)dllgethostbyaddr);
}

/*
    WS2_32.DLL
    
    ordinal hint RVA      name
 
        500    0 0001068D WEP
         25    1 00010336 WPUCompleteOverlappedRequest
         26    2 000086A5 WSAAccept
         27    3 000058E9 WSAAddressToStringA
         28    4 00002045 WSAAddressToStringW
        102    5 0000D4E1 WSAAsyncGetHostByAddr
        103    6 0000896B WSAAsyncGetHostByName
        105    7 0000D584 WSAAsyncGetProtoByName
        104    8 0000D623 WSAAsyncGetProtoByNumber
        107    9 0000D3B9 WSAAsyncGetServByName
        106    A 0000D45E WSAAsyncGetServByPort
        101    B 000060C9 WSAAsyncSelect
        108    C 0000D6A0 WSACancelAsyncRequest
        113    D 0000C811 WSACancelBlockingCall
        116    E 00001836 WSACleanup
         29    F 00005F97 WSACloseEvent
         30   10 0000F6AF WSAConnect
         31   11 00005ED3 WSACreateEvent
         32   12 0000CD4F WSADuplicateSocketA
         33   13 0000CCC8 WSADuplicateSocketW
         34   14 0000E63C WSAEnumNameSpaceProvidersA
         35   15 0000E695 WSAEnumNameSpaceProvidersW
         36   16 00005EFC WSAEnumNetworkEvents
         37   17 0000CDF9 WSAEnumProtocolsA
         38   18 000064B5 WSAEnumProtocolsW
         39   19 00005E6F WSAEventSelect
        111   1A 00001740 WSAGetLastError
         40   1B 00008FE5 WSAGetOverlappedResult
         41   1C 0000E279 WSAGetQOSByName
         42   1D 0000EE52 WSAGetServiceClassInfoA
         43   1E 0000E7B9 WSAGetServiceClassInfoW
         44   1F 0000EACC WSAGetServiceClassNameByClassIdA
         45   20 0000EC95 WSAGetServiceClassNameByClassIdW
         46   21 0000B6CE WSAHtonl
         47   22 0000B620 WSAHtons
         48   23 0000F0C4 WSAInstallServiceClassA
         49   24 0000E8F6 WSAInstallServiceClassW
         50   25 000014DC WSAIoctl
        114   26 0000C859 WSAIsBlocking
         58   27 0000F828 WSAJoinLeaf
         59   28 00002E18 WSALookupServiceBeginA
         60   29 000023F3 WSALookupServiceBeginW
         61   2A 000022E2 WSALookupServiceEnd
         62   2B 00002B24 WSALookupServiceNextA
         63   2C 0000214D WSALookupServiceNextW
         64   2D 00006130 WSANSPIoctl
         65   2E 0000B6CE WSANtohl
         66   2F 0000B620 WSANtohs
         67   30 00006653 WSAProviderConfigChange
         68   31 000019A0 WSARecv
         69   32 0000E3DF WSARecvDisconnect
         70   33 00008F4A WSARecvFrom
         71   34 0000E9E1 WSARemoveServiceClass
         72   35 0000CF09 WSAResetEvent
         73   36 00005722 WSASend
         74   37 0000F5A4 WSASendDisconnect
         75   38 00009047 WSASendTo
        109   39 000088FF WSASetBlockingHook
         76   3A 00009114 WSASetEvent
        112   3B 0000350D WSASetLastError
         77   3C 0000F13C WSASetServiceA
         78   3D 0000EF9F WSASetServiceW
         79   3E 00005A01 WSASocketA
         80   3F 00003A72 WSASocketW
        115   40 000041DA WSAStartup
         81   41 00003538 WSAStringToAddressA
         82   42 0000EEDD WSAStringToAddressW
        110   43 0000887B WSAUnhookBlockingHook
         83   44 000044AB WSAWaitForMultipleEvents
         24   45 00010453 WSApSetPostRoutine
         84   46 000100BF WSCDeinstallProvider
         85   47 0000DD44 WSCEnableNSProvider
         86   48 00006738 WSCEnumProtocols
         87   49 0000CDF4 WSCGetProviderPath
         88   4A 0000DFE6 WSCInstallNameSpace
         89   4B 0000FE48 WSCInstallProvider
         90   4C 0000E157 WSCUnInstallNameSpace
         91   4D 0000FAD4 WSCUpdateProvider
         92   4E 0000DEB9 WSCWriteNameSpaceOrder
         93   4F 0000FD1B WSCWriteProviderOrder
        151   50 00001B7B __WSAFDIsSet
          1   51 0000868D accept
          2   52 00003ECE bind
          3   53 00001A6D closesocket
          4   54 00003E5D connect
         94   55 00003A2C freeaddrinfo
         95   56 000033DF getaddrinfo
         51   57 0000D755 gethostbyaddr
         52   58 00002BBF gethostbyname
         57   59 000032CA gethostname
         96   5A 0000C076 getnameinfo
          5   5B 0000F628 getpeername
         53   5C 0000D24E getprotobyname
         54   5D 0000D1A2 getprotobynumber
         55   5E 0000D969 getservbyname
         56   5F 0000D850 getservbyport
          6   60 0000157E getsockname
          7   61 00004122 getsockopt
          8   62 000012A7 htonl
          9   63 00001746 htons
         11   64 000012F8 inet_addr
         12   65 0000401C inet_ntoa
         10   66 0000155A ioctlsocket
         13   67 00005DE2 listen
         14   68 000012A7 ntohl
         15   69 00001746 ntohs
         16   6A 00005690 recv
         17   6B 00001444 recvfrom
         18   6C 00001890 select
         19   6D 00001AF4 send
         20   6E 00001ED3 sendto
         21   6F 00003F8D setsockopt
         22   70 00008629 shutdown
         23   71 00003C22 socket
         */
