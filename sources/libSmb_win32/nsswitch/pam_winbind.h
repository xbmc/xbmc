/* pam_winbind header file 
   (Solaris needs some macros from Linux for common PAM code)

   Shirish Kalele 2000
*/

#ifdef HAVE_FEATURES_H
#include <features.h>
#endif

#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <string.h>
#include <syslog.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include <config.h>

#define MODULE_NAME "pam_winbind"
#define PAM_SM_AUTH
#define PAM_SM_ACCOUNT
#define PAM_SM_PASSWORD

#ifndef PAM_WINBIND_CONFIG_FILE
#define PAM_WINBIND_CONFIG_FILE "/etc/security/pam_winbind.conf"
#endif

#include <iniparser.h>

#if defined(SUNOS5) || defined(SUNOS4) || defined(HPUX) || defined(FREEBSD) || defined(AIX)

/* Solaris always uses dynamic pam modules */
#define PAM_EXTERN extern
#include <security/pam_appl.h> 

#ifndef PAM_AUTHTOK_RECOVER_ERR
#define PAM_AUTHTOK_RECOVER_ERR PAM_AUTHTOK_RECOVERY_ERR
#endif

#endif

#ifdef HAVE_SECURITY_PAM_MODULES_H
#include <security/pam_modules.h>
#endif

#ifdef HAVE_SECURITY__PAM_MACROS_H
#include <security/_pam_macros.h>
#else
/* Define required macros from (Linux PAM 0.68) security/_pam_macros.h */
#define _pam_drop_reply(/* struct pam_response * */ reply, /* int */ replies) \
do {                                              \
    int reply_i;                                  \
                                                  \
    for (reply_i=0; reply_i<replies; ++reply_i) { \
        if (reply[reply_i].resp) {                \
            _pam_overwrite(reply[reply_i].resp);  \
            free(reply[reply_i].resp);            \
        }                                         \
    }                                             \
    if (reply)                                    \
        free(reply);                              \
} while (0)

#define _pam_overwrite(x)        \
do {                             \
     register char *__xx__;      \
     if ((__xx__=(x)))           \
          while (*__xx__)        \
               *__xx__++ = '\0'; \
} while (0)

/*
 * Don't just free it, forget it too.
 */

#define _pam_drop(X) SAFE_FREE(X)

#define  x_strdup(s)  ( (s) ? strdup(s):NULL )     
#endif

#define WINBIND_DEBUG_ARG (1<<0)
#define WINBIND_USE_AUTHTOK_ARG (1<<1)
#define WINBIND_UNKNOWN_OK_ARG (1<<2)
#define WINBIND_TRY_FIRST_PASS_ARG (1<<3)
#define WINBIND_USE_FIRST_PASS_ARG (1<<4)
#define WINBIND__OLD_PASSWORD (1<<5)
#define WINBIND_REQUIRED_MEMBERSHIP (1<<6)
#define WINBIND_KRB5_AUTH (1<<7)
#define WINBIND_KRB5_CCACHE_TYPE (1<<8)
#define WINBIND_CACHED_LOGIN (1<<9)
#define WINBIND_CONFIG_FILE (1<<10)

/*
 * here is the string to inform the user that the new passwords they
 * typed were not the same.
 */

#define MISTYPED_PASS "Sorry, passwords do not match"

#define on(x, y) (x & y)
#define off(x, y) (!(x & y))

#define PAM_WINBIND_NEW_AUTHTOK_REQD "PAM_WINBIND_NEW_AUTHTOK_REQD"
#define PAM_WINBIND_HOMEDIR "PAM_WINBIND_HOMEDIR"
#define PAM_WINBIND_LOGONSCRIPT "PAM_WINBIND_LOGONSCRIPT"
#define PAM_WINBIND_PROFILEPATH "PAM_WINBIND_PROFILEPATH"
#define PAM_WINBIND_PWD_LAST_SET "PAM_WINBIND_PWD_LAST_SET"

#define SECONDS_PER_DAY 86400

#define DAYS_TO_WARN_BEFORE_PWD_EXPIRES 5

#include "winbind_client.h"

#define PAM_WB_REMARK_DIRECT(h,x)\
{\
	const char *error_string = NULL; \
	error_string = _get_ntstatus_error_string(x);\
	if (error_string != NULL) {\
		_make_remark(h, PAM_ERROR_MSG, error_string);\
	} else {\
		_make_remark(h, PAM_ERROR_MSG, x);\
	};\
};

#define PAM_WB_REMARK_DIRECT_RET(h,x)\
{\
	const char *error_string = NULL; \
	error_string = _get_ntstatus_error_string(x);\
	if (error_string != NULL) {\
		_make_remark(h, PAM_ERROR_MSG, error_string);\
		return ret;\
	};\
	_make_remark(h, PAM_ERROR_MSG, x);\
	return ret;\
};
	
#define PAM_WB_REMARK_CHECK_RESPONSE_RET(h,x,y)\
{\
	const char *ntstatus = x.data.auth.nt_status_string; \
	const char *error_string = NULL; \
	if (!strcasecmp(ntstatus,y)) {\
		error_string = _get_ntstatus_error_string(y);\
		if (error_string != NULL) {\
			_make_remark(h, PAM_ERROR_MSG, error_string);\
			return ret;\
		};\
		if (x.data.auth.error_string[0] != '\0') {\
			_make_remark(h, PAM_ERROR_MSG, x.data.auth.error_string);\
			return ret;\
		};\
		_make_remark(h, PAM_ERROR_MSG, y);\
		return ret;\
	};\
};

/* from include/rpc_samr.h */
#define DOMAIN_PASSWORD_COMPLEX            0x00000001

#define REJECT_REASON_OTHER		0x00000000
#define REJECT_REASON_TOO_SHORT		0x00000001
#define REJECT_REASON_IN_HISTORY	0x00000002
#define REJECT_REASON_NOT_COMPLEX	0x00000005

/* from include/smb.h */
#define ACB_PWNOEXP			0x00000200

/* from include/rpc_netlogon.h */
#define LOGON_CACHED_ACCOUNT		0x00000004

