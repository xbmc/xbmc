#ifndef XB_LIB_SMB_H
#define XB_LIB_SMB_H

#include "../lib/libsmb/config.h"
//#include "../lib/libsmb/includes.h"
#define uint16 unsigned short
#define int32 INT32
#include "../lib/libsmb/charset.h"

#ifdef __cplusplus
extern "C" {
#endif

	#include "../lib/libsmb/libsmbclient.h"
	#undef offset_t

	BOOL lp_do_parameter(int snum, const char *pszParmName, const char *pszParmValue);
	void set_xbox_interface(char* ip, char* subnet);

	size_t convert_string(charset_t from, charset_t to,
		void const *src, size_t srclen, 
		void *dest, size_t destlen);

#ifdef __cplusplus
}
#endif



#endif // XB_LIB_SMB_H
