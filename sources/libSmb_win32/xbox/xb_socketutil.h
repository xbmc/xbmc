#ifndef SMB_SOCKETUTIL_H_
#define SMB_SOCKETUTIL_H_

#include "xb_defines.h"

typedef struct {
	struct hostent server;
	char name[128];
	char addr[16];
	char* addr_list[4];
} HostEnt;

#ifdef __cplusplus
extern "C" {
#endif

// #define inet_ntoa(in) smb_inet_ntoa(in)
#define gethostbyname(name) smb_gethostbyname(name)
#define gethostname(buffer, buffersize) smb_gethostname(buffer, buffersize)
#define gethostbyaddr(addr, len, type) smb_gethostbyaddr(addr, len, type)

char* smb_inet_ntoa (struct in_addr in);
struct hostent* FAR smb_gethostbyname(const char* _name);
int smb_gethostname(char* buffer, int buffersize);
struct hostent* FAR smb_gethostbyaddr(const char* addr, int len, int type);
/*
struct servent* FAR getservbyname(const char* name, const char* proto);
struct servent* FAR getservbyport(int port, const char* proto);
struct protoent* FAR getprotobyname(const char* name);
*/

int socketerrno(int isconnect);

#ifdef __cplusplus
}
#endif

#endif // SMB_SOCKETUTIL_H_