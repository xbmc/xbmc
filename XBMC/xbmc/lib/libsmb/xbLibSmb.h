#ifndef XB_LIB_SMB_H
#define XB_LIB_SMB_H

#include "../lib/libsmb/config.h"

#ifdef __cplusplus
extern "C" {
#endif

	#include "../lib/libsmb/libsmbclient.h"
	#undef offset_t

	BOOL lp_do_parameter(int snum, const char *pszParmName, const char *pszParmValue);
	void set_xbox_interface(char* ip, char* subnet);

#ifdef __cplusplus
}
#endif



#endif // XB_LIB_SMB_H
