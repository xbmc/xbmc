#ifndef XB_LIB_SMB_H
#define XB_LIB_SMB_H

#define uint u_int
#define mode_t int
#define ssize_t int
#define uid_t int
#define gid_t int
#define SMB_STRUCT_STAT struct __stat64
#define SMB_OFF_T UINT64

/* this defines the charset types used in samba */
typedef enum {CH_UCS2=0, CH_UNIX=1, CH_DISPLAY=2, CH_DOS=3, CH_UTF8=4} charset_t;

#ifdef __cplusplus
extern "C" {
#endif

	#include "libsmbclient.h"
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
