#ifndef XB_LIB_SMB_H
#define XB_LIB_SMB_H

#define uint u_int
#define mode_t int
#define ssize_t int
#define uid_t int
#define gid_t int
#define SMB_STRUCT_STAT struct __stat64
#define SMB_OFF_T UINT64

#define NT_STATUS_INVALID_COMPUTER_NAME long(0xC0000000 | 0x0122)
#define ENETUNREACH WSAENETUNREACH

/* execute permission: other */
#ifndef S_IXOTH
#define S_IXOTH 00001
#endif

/* this defines the charset types used in samba */
typedef enum {CH_UCS2=0, CH_UNIX=1, CH_DISPLAY=2, CH_DOS=3, CH_UTF8=4} charset_t;
typedef void (*smb_log_callback)(const char* logMessage);

#ifdef __cplusplus
extern "C" {
#endif

	#include "libsmbclient_win32.h"
	#undef offset_t

	const char *nt_errstr(LONG nt_code);
	const char *get_friendly_nt_error_msg(LONG nt_code);
	LONG map_nt_error_from_unix(int unix_error);
	
	BOOL lp_do_parameter(int snum, const char *pszParmName, const char *pszParmValue);
	void set_xbox_interface(char* ip, char* subnet);

	size_t convert_string(charset_t from, charset_t to,
	      void const *src, size_t srclen, 
	      void *dest, size_t destlen, BOOL allow_bad_conv);
		
	void set_log_callback(smb_log_callback fn);
	void xb_setSambaWorkgroup(char* workgroup);

  int smbc_purge();
#ifdef __cplusplus
}
#endif



#endif // XB_LIB_SMB_H
