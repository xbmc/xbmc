#ifndef SOCKETUTIL_H_
#define SOCKETUTIL_H_
xcvbxcvbcx
#include <xtl.h>
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

struct hostent* FAR gethostbyname(const char* _name);
int gethostname(char* buffer, int buffersize);
struct hostent* FAR gethostbyaddr(const char* addr, int len, int type);
struct servent* FAR getservbyname(const char* name, const char* proto);
struct servent* FAR getservbyport(int port, const char* proto);
struct protoent* FAR getprotobyname(const char* name);
char* inet_ntoa (struct in_addr in);

#ifdef __cplusplus
}
#endif

#endif // SOCKETUTIL_H_