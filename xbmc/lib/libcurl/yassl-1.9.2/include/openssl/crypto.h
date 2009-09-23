/* crypto.h for openSSL */

#ifndef ysSSL_crypto_h__
#define yaSSL_crypto_h__

#ifdef YASSL_PREFIX
#include "prefix_crypto.h"
#endif

const char* SSLeay_version(int type);

#define SSLEAY_NUMBER_DEFINED
#define SSLEAY_VERSION 0x0900L
#define SSLEAY_VERSION_NUMBER SSLEAY_VERSION


#endif /* yaSSL_crypto_h__ */

