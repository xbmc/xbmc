#ifndef LINUX
/* This is only needed by modules in the Sun implementation. */
#include <security/pam_appl.h>
#endif  /* LINUX */

#include <security/pam_modules.h>

#ifndef PAM_AUTHTOK_RECOVER_ERR  
#define PAM_AUTHTOK_RECOVER_ERR PAM_AUTHTOK_RECOVERY_ERR
#endif

#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>

/*
 * here is the string to inform the user that the new passwords they
 * typed were not the same.
 */

#define MISTYPED_PASS "Sorry, passwords do not match"

/* type definition for the control options */

typedef struct {
     const char *token;
     unsigned int mask;            /* shall assume 32 bits of flags */
     unsigned int flag;
} SMB_Ctrls;

#ifndef False
#define False (0)
#endif

#ifndef True
#define True (1)
#endif

/* macro to determine if a given flag is on */
#define on(x,ctrl)  (smb_args[x].flag & ctrl)

/* macro to determine that a given flag is NOT on */
#define off(x,ctrl) (!on(x,ctrl))

/* macro to turn on/off a ctrl flag manually */
#define set(x,ctrl)   (ctrl = ((ctrl)&smb_args[x].mask)|smb_args[x].flag)
#define unset(x,ctrl) (ctrl &= ~(smb_args[x].flag))

/* the generic mask */
#define _ALL_ON_  (~0U)

/* end of macro definitions definitions for the control flags */

/*
 * These are the options supported by the smb password module, very
 * similar to the pwdb options
 */

#define SMB__OLD_PASSWD		 0	/* internal */
#define SMB__VERIFY_PASSWD	 1	/* internal */

#define SMB_AUDIT		 2	/* print more things than debug..
					   some information may be sensitive */
#define SMB_USE_FIRST_PASS	 3
#define SMB_TRY_FIRST_PASS	 4
#define SMB_NOT_SET_PASS	 5	/* don't set the AUTHTOK items */

#define SMB__NONULL		 6	/* internal */
#define SMB__QUIET		 7	/* internal */
#define SMB_USE_AUTHTOK		 8	/* insist on reading PAM_AUTHTOK */
#define SMB__NULLOK		 9	/* Null token ok */
#define SMB_DEBUG		10	/* send more info to syslog(3) */
#define SMB_NODELAY		11	/* admin does not want a fail-delay */
#define SMB_MIGRATE		12	/* Does no authentication, just
					   updates the smb database. */
#define SMB_CONF_FILE		13	/* Alternate location of smb.conf */

#define SMB_CTRLS_		14	/* number of ctrl arguments defined */

static const SMB_Ctrls smb_args[SMB_CTRLS_] = {
/* symbol                 token name          ctrl mask      ctrl       *
 * ------------------     ------------------  -------------- ---------- */

/* SMB__OLD_PASSWD */	 {  NULL,            _ALL_ON_,              01 },
/* SMB__VERIFY_PASSWD */ {  NULL,            _ALL_ON_,              02 },
/* SMB_AUDIT */		 { "audit",          _ALL_ON_,              04 },
/* SMB_USE_FIRST_PASS */ { "use_first_pass", _ALL_ON_^(030),       010 },
/* SMB_TRY_FIRST_PASS */ { "try_first_pass", _ALL_ON_^(030),       020 },
/* SMB_NOT_SET_PASS */	 { "not_set_pass",   _ALL_ON_,             040 },
/* SMB__NONULL */	 {  "nonull",        _ALL_ON_,            0100 },
/* SMB__QUIET */	 {  NULL,            _ALL_ON_,            0200 },
/* SMB_USE_AUTHTOK */	 { "use_authtok",    _ALL_ON_,            0400 },
/* SMB__NULLOK */	 { "nullok",         _ALL_ON_^(0100),        0 },
/* SMB_DEBUG */		 { "debug",          _ALL_ON_,           01000 },
/* SMB_NODELAY */	 { "nodelay",        _ALL_ON_,           02000 },
/* SMB_MIGRATE */	 { "migrate",        _ALL_ON_^(0100),	 04000 },
/* SMB_CONF_FILE */	 { "smbconf=",       _ALL_ON_,		     0 },
};

#define SMB_DEFAULTS  (smb_args[SMB__NONULL].flag)

/*
 * the following is used to keep track of the number of times a user fails
 * to authenticate themself.
 */

#define FAIL_PREFIX			"-SMB-FAIL-"
#define SMB_MAX_RETRIES			3

struct _pam_failed_auth {
    char *user;                 /* user that's failed to be authenticated */
    int id;                     /* uid of requested user */
    char *agent;                /* attempt from user with name */
    int count;                  /* number of failures so far */
};

/*
 * General use functions go here 
 */

/* from support.c */
int make_remark(pam_handle_t *, unsigned int, int, const char *);
