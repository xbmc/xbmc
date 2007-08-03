#ifndef __MD5_H__
#define __MD5_H__

#include "../portability.h"

typedef struct {
    u_int32_t buf[4];
    u_int32_t bits[2];
    unsigned char in[64];
    int apple_ver;
} MD5_CTX;

void OpenDaap_MD5Init(MD5_CTX *, int apple_ver);
void OpenDaap_MD5Update(MD5_CTX *, const unsigned char *, unsigned int);
void OpenDaap_MD5Final(MD5_CTX *, unsigned char digest[16]);

#endif /* __WINE_MD5_H__ */
