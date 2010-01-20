/*
 *	um.h -- GoAhead User Management public header
 *
 *	Copyright (c) GoAhead Software Inc., 1992-2000. All Rights Reserved.
 *
 *	See the file "license.txt" for information on usage and redistribution
 *
 * $Id: um.h,v 1.2 2002/10/24 14:44:50 bporter Exp $
 */

#ifndef _h_UM
#define _h_UM 1

/******************************** Description *********************************/

/*
 *	GoAhead User Management header. This defines the User Management
 *	public APIs.  Include this header for files that contain access to
 *	user inquiry or management.
 */

/********************************* Includes ***********************************/

#ifndef UEMF
	#include	"basic/basic.h"
	#include	"emf/emf.h"
#else
	#include	"uemf.h"
#endif

/********************************** Defines ***********************************/

/*
 *	Error Return Flags
 */
#define UM_OK				0
#define UM_ERR_GENERAL		-1
#define UM_ERR_NOT_FOUND	-2
#define UM_ERR_PROTECTED	-3
#define UM_ERR_DUPLICATE	-4
#define UM_ERR_IN_USE		-5
#define UM_ERR_BAD_NAME		-6
#define UM_ERR_BAD_PASSWORD -7

/*
 *	Privilege Masks
 */
#define PRIV_NONE	0x00
#define PRIV_READ	0x01
#define PRIV_WRITE	0x02
#define PRIV_ADMIN	0x04

/*
 *	User classes
 */
typedef short bool_t;

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

typedef enum {
	AM_NONE = 0,
	AM_FULL,
	AM_BASIC,
	AM_DIGEST,
	AM_INVALID
} accessMeth_t;

/********************************** Prototypes ********************************/

/*
 *	umOpen() must be called before accessing User Management functions
 */
extern int				umOpen();

/*
 *	umClose() should be called before shutdown to free memory
 */
extern void				umClose();

/*
 *	umCommit() persists the user management database
 */
extern int				umCommit(char_t *filename);

/*
 *	umRestore() loads the user management database
 */
extern int				umRestore(char_t *filename);

/*
 *	umUser functions use a user ID for a key
 */
extern int				umAddUser(char_t *user, char_t *password,
							char_t *group, bool_t protect, bool_t disabled);

extern int				umDeleteUser(char_t *user);

extern char_t			*umGetFirstUser();
extern char_t			*umGetNextUser(char_t *lastUser);

extern bool_t			umUserExists(char_t *user);

extern char_t			*umGetUserPassword(char_t *user);
extern int				umSetUserPassword(char_t *user, char_t *password);

extern char_t			*umGetUserGroup(char_t *user);
extern int				umSetUserGroup(char_t *user, char_t *password);

extern bool_t			umGetUserEnabled(char_t *user);
extern int				umSetUserEnabled(char_t *user, bool_t enabled);

extern bool_t			umGetUserProtected(char_t *user);
extern int				umSetUserProtected(char_t *user, bool_t protect);

/*
 *	umGroup functions use a group name for a key
 */
extern int				umAddGroup(char_t *group, short privilege,
							accessMeth_t am, bool_t protect, bool_t disabled);

extern int				umDeleteGroup(char_t *group);

extern char_t 			*umGetFirstGroup();
extern char_t			*umGetNextGroup(char_t *lastUser);

extern bool_t			umGroupExists(char_t *group);
extern bool_t			umGetGroupInUse(char_t *group);

extern accessMeth_t		umGetGroupAccessMethod(char_t *group);
extern int				umSetGroupAccessMethod(char_t *group, accessMeth_t am);

extern bool_t			umGetGroupEnabled(char_t *group);
extern int				umSetGroupEnabled(char_t *group, bool_t enabled);

extern short			umGetGroupPrivilege(char_t *group);
extern int				umSetGroupPrivilege(char_t *group, short privileges);

extern bool_t			umGetGroupProtected(char_t *group);
extern int				umSetGroupProtected(char_t *group, bool_t protect);

/*
 *	umAccessLimit functions use a URL as a key
 */
extern int			umAddAccessLimit(char_t *url, accessMeth_t am,
						short secure, char_t *group);

extern int			umDeleteAccessLimit(char_t *url);

extern char_t		*umGetFirstAccessLimit();
extern char_t		*umGetNextAccessLimit(char_t *lastUser);

/*
 *	Returns the name of an ancestor access limit if
 */
extern char_t		*umGetAccessLimit(char_t *url);

extern bool_t		umAccessLimitExists(char_t *url);

extern accessMeth_t	umGetAccessLimitMethod(char_t *url);
extern int			umSetAccessLimitMethod(char_t *url, accessMeth_t am);

extern short		umGetAccessLimitSecure(char_t *url);
extern int			umSetAccessLimitSecure(char_t *url, short secure);

extern char_t		*umGetAccessLimitGroup(char_t *url);
extern int			umSetAccessLimitGroup(char_t *url, char_t *group);

/*
 *	Convenience Functions
 */

extern accessMeth_t	umGetAccessMethodForURL(char_t *url);
extern bool_t		umUserCanAccessURL(char_t *user, char_t *url);

#endif /* _h_UM */

/******************************************************************************/


