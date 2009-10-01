/*
 * umui.c -- User Management GoForm Processing
 *
 * Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 * See the file "license.txt" for usage and redistribution license requirements
 *
 *	$Id: umui.c,v 1.2 2002/10/24 14:44:50 bporter Exp $
 */

/******************************** Description *********************************/

/*
 *	This module provides GoForm functions for User management
 */

/********************************* Includes ***********************************/

#include	"wsIntrn.h"
#include	"um.h"

/********************************* Defines ************************************/

#define		NONE_OPTION		T("<NONE>")
#define		MSG_START		T("<body><h2>")
#define		MSG_END			T("</h2></body>")

/**************************** Forward Declarations ****************************/

static void		formAddUser(webs_t wp, char_t *path, char_t *query);
static void		formDeleteUser(webs_t wp, char_t *path, char_t *query);
static void		formDisplayUser(webs_t wp, char_t *path, char_t *query);
static int		aspGenerateUserList(int eid, webs_t wp, 
									int argc, char_t **argv);

static void		formAddGroup(webs_t wp, char_t *path, char_t *query);
static void		formDeleteGroup(webs_t wp, char_t *path, char_t *query);
static int		aspGenerateGroupList(int eid, webs_t wp, 
									 int argc, char_t **argv);

static void		formAddAccessLimit(webs_t wp, char_t *path, char_t *query);
static void		formDeleteAccessLimit(webs_t wp, char_t *path, char_t *query);
static int		aspGenerateAccessLimitList(int eid, webs_t wp, 
										   int argc, char_t **argv);

static int		aspGenerateAccessMethodList(int eid, webs_t wp, 
											int argc, char_t **argv);
static int		aspGeneratePrivilegeList(int eid, webs_t wp, 
										 int argc, char_t **argv);

static void		formSaveUserManagement(webs_t wp, char_t *path, char_t *query);
static void		formLoadUserManagement(webs_t wp, char_t *path, char_t *query);

static void		websMsgStart(webs_t wp);
static void		websMsgEnd(webs_t wp);

/*********************************** Code *************************************/
/*
 *	Set up the User Management form handlers
 */

void formDefineUserMgmt(void)
{
	websAspDefine(T("MakeGroupList"), aspGenerateGroupList);
	websAspDefine(T("MakeUserList"), aspGenerateUserList);
	websAspDefine(T("MakeAccessLimitList"), aspGenerateAccessLimitList);
	websAspDefine(T("MakeAccessMethodList"), aspGenerateAccessMethodList);
	websAspDefine(T("MakePrivilegeList"), aspGeneratePrivilegeList);

	websFormDefine(T("AddUser"), formAddUser);
	websFormDefine(T("DeleteUser"), formDeleteUser);
	websFormDefine(T("DisplayUser"), formDisplayUser);
	websFormDefine(T("AddGroup"), formAddGroup);
	websFormDefine(T("DeleteGroup"), formDeleteGroup);
	websFormDefine(T("AddAccessLimit"), formAddAccessLimit);
	websFormDefine(T("DeleteAccessLimit"), formDeleteAccessLimit);

	websFormDefine(T("SaveUserManagement"), formSaveUserManagement);
	websFormDefine(T("LoadUserManagement"), formLoadUserManagement);
}

/******************************************************************************/
/*
 *  Add a user
 */

static void formAddUser(webs_t wp, char_t *path, char_t *query)
{
	char_t	*userid, *pass1, *pass2, *group, *enabled, *ok;
	bool_t bDisable;
	int	nCheck;

	a_assert(wp);

	userid = websGetVar(wp, T("user"), T("")); 
	pass1 = websGetVar(wp, T("password"), T("")); 
	pass2 = websGetVar(wp, T("passconf"), T("")); 
	group = websGetVar(wp, T("group"), T("")); 
	enabled = websGetVar(wp, T("enabled"), T("")); 
	ok = websGetVar(wp, T("ok"), T("")); 

	websHeader(wp);
	websMsgStart(wp);

	if (gstricmp(ok, T("ok")) != 0) {
		websWrite(wp, T("Add User Cancelled"));
	} else if (gstrcmp(pass1, pass2) != 0) {
		websWrite(wp, T("Confirmation Password did not match."));
	} else {
		if (enabled && *enabled && (gstrcmp(enabled, T("on")) == 0)) {
			bDisable = FALSE;
		} else {
			bDisable = TRUE;
		}

		nCheck = umAddUser(userid, pass1, group, 0, bDisable);
		if (nCheck != 0) {
			char_t * strError;

			switch (nCheck) {
			case UM_ERR_DUPLICATE:
				strError = T("User already exists.");
				break;

			case UM_ERR_BAD_NAME:
				strError = T("Invalid user name.");
				break;

			case UM_ERR_BAD_PASSWORD:
				strError = T("Invalid password.");
				break;

			case UM_ERR_NOT_FOUND:
				strError = T("Invalid or unselected group.");
				break;

			default:
				strError = T("Error writing user record.");
				break;
			}

			websWrite(wp, T("Unable to add user, \"%s\".  %s"),
				userid, strError);
		} else {
			websWrite(wp, T("User, \"%s\" was successfully added."),
				userid);
		}
	}

	websMsgEnd(wp);
	websFooter(wp);
	websDone(wp, 200);
}

/******************************************************************************/
/*
 *  Delete a user
 */

static void formDeleteUser(webs_t wp, char_t *path, char_t *query)
{
	char_t	*userid, *ok;

	a_assert(wp);

	userid = websGetVar(wp, T("user"), T("")); 
	ok = websGetVar(wp, T("ok"), T("")); 

	websHeader(wp);
	websMsgStart(wp);

	if (gstricmp(ok, T("ok")) != 0) {
		websWrite(wp, T("Delete User Cancelled"));
	} else if (umUserExists(userid) == FALSE) {
		websWrite(wp, T("ERROR: User \"%s\" not found"), userid);
	} else if (umGetUserProtected(userid)) {
		websWrite(wp, T("ERROR: User, \"%s\" is delete-protected."), userid);
	} else if (umDeleteUser(userid) != 0) {
		websWrite(wp, T("ERROR: Unable to delete user, \"%s\" "), userid);
	} else {
		websWrite(wp, T("User, \"%s\" was successfully deleted."), userid);
	}

	websMsgEnd(wp);
	websFooter(wp);
	websDone(wp, 200);
}

/******************************************************************************/
/*
 *  Display the user info
 */

static void formDisplayUser(webs_t wp, char_t *path, char_t *query)
{
	char_t	*userid, *ok, *temp;
	bool_t	enabled;

	a_assert(wp);

	userid = websGetVar(wp, T("user"), T("")); 
	ok = websGetVar(wp, T("ok"), T("")); 

	websHeader(wp);
	websWrite(wp, T("<body>"));

	if (gstricmp(ok, T("ok")) != 0) {
		websWrite(wp, T("Display User Cancelled"));
	} else if (umUserExists(userid) == FALSE) {
		websWrite(wp, T("ERROR: User <b>%s</b> not found.\n"), userid);
	} else {
		websWrite(wp, T("<h2>User ID: <b>%s</b></h2>\n"), userid);
		temp = umGetUserGroup(userid);
		websWrite(wp, T("<h3>User Group: <b>%s</b></h3>\n"), temp);
		enabled = umGetUserEnabled(userid);
		websWrite(wp, T("<h3>Enabled: <b>%d</b></h3>\n"), enabled);
	}

	websWrite(wp, T("</body>\n"));
	websFooter(wp);
	websDone(wp, 200);
}


/******************************************************************************/
/*
 *  Generate HTML to create a list box containing the users
 */

static int aspGenerateUserList(int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*userid;
	int		row, nBytesSent, nBytes;

	a_assert(wp);

	nBytes = websWrite(wp, 
		T("<SELECT NAME=\"user\" SIZE=\"3\" TITLE=\"Select a User\">"));
	row = 0;
	userid = umGetFirstUser();
	nBytesSent = 0;

	while (userid && (nBytes > 0)) {
		nBytes = websWrite(wp, T("<OPTION VALUE=\"%s\">%s\n"), 
			userid, userid);
		userid = umGetNextUser(userid);
		nBytesSent += nBytes;
	}

	nBytesSent += websWrite(wp, T("</SELECT>"));

	return nBytesSent;
}

/******************************************************************************/
/*
 *  Add a group
 */

static void formAddGroup(webs_t wp, char_t *path, char_t *query)
{
	char_t			*group, *enabled, *privilege, *method, *ok, *pChar;
	int				nCheck;
	short			priv;
	accessMeth_t	am;
	bool_t			bDisable;

	a_assert(wp);

	group = websGetVar(wp, T("group"), T("")); 
	method = websGetVar(wp, T("method"), T("")); 
	enabled = websGetVar(wp, T("enabled"), T("")); 
	privilege = websGetVar(wp, T("privilege"), T("")); 
	ok = websGetVar(wp, T("ok"), T("")); 

	websHeader(wp);
	websMsgStart(wp);

	if (gstricmp(ok, T("ok")) != 0) {
		websWrite(wp, T("Add Group Cancelled."));
	} else if ((group == NULL) || (*group == 0)) {
		websWrite(wp, T("No Group Name was entered."));
	} else if (umGroupExists(group)) {
		websWrite(wp, T("ERROR: Group, \"%s\" already exists."), group);
	} else {
		if (privilege && *privilege) {
/*
 *			privilege is a mulitple <SELECT> var, and must be parsed.
 *			Values for these variables are space delimited.
 */
			priv = 0;
			for (pChar = privilege; *pChar; pChar++) {
				if (*pChar == ' ') {
					*pChar = '\0';
					priv |= gatoi(privilege);
					*pChar = ' ';
					privilege = pChar + 1;
				}
			}
			priv |= gatoi(privilege);
		} else {
			priv = 0;
		}

		if (method && *method) {
			am = (accessMeth_t) gatoi(method);
		} else {
			am = AM_FULL;
		}

		if (enabled && *enabled && (gstrcmp(enabled, T("on")) == 0)) {
			bDisable = FALSE;
		} else {
			bDisable = TRUE;
		}

		nCheck = umAddGroup(group, priv, am, 0, bDisable);
		if (nCheck != 0) {
			websWrite(wp, T("Unable to add group, \"%s\", code: %d "),
				group, nCheck);
		} else {
			websWrite(wp, T("Group, \"%s\" was successfully added."), 
				group);
		}
	}

	websMsgEnd(wp);
	websFooter(wp);
	websDone(wp, 200);
}

/******************************************************************************/
/*
 *  Delete a group
 */

static void formDeleteGroup(webs_t wp, char_t *path, char_t *query)
{
	char_t	*group, *ok;

	a_assert(wp);

	group = websGetVar(wp, T("group"), T("")); 
	ok = websGetVar(wp, T("ok"), T("")); 

	websHeader(wp);
	websMsgStart(wp);

	if (gstricmp(ok, T("ok")) != 0) {
		websWrite(wp, T("Delete Group Cancelled."));
	} else if ((group == NULL) || (*group == '\0')) {
		websWrite(wp, T("ERROR: No group was selected."));
	} else if (umGetGroupProtected(group)) {
		websWrite(wp, T("ERROR: Group, \"%s\" is delete-protected."), group);
	} else if (umGetGroupInUse(group)) {
		websWrite(wp, T("ERROR: Group, \"%s\" is being used."),	group);
	} else if (umDeleteGroup(group) != 0) {
		websWrite(wp, T("ERROR: Unable to delete group, \"%s\" "), group);
	} else {
		websWrite(wp, T("Group, \"%s\" was successfully deleted."), group);
	}

	websMsgEnd(wp);
	websFooter(wp);
	websDone(wp, 200);
}

/******************************************************************************/
/*
 *  Generate HTML to create a list box containing the groups
 */

static int aspGenerateGroupList(int eid, webs_t wp, int argc, char_t **argv)
{
	char_t	*group;
	int		row, nBytesSent, nBytes;

	a_assert(wp);

	row = 0;
	nBytesSent = 0;
	nBytes = websWrite(wp, 
		T("<SELECT NAME=\"group\" SIZE=\"3\" TITLE=\"Select a Group\">"));
/*
 *  Add a special "<NONE>" element to allow de-selection
 */
	nBytes = websWrite(wp, T("<OPTION VALUE=\"\">[NONE]\n"));

	group = umGetFirstGroup();
	while (group && (nBytes > 0)) {
		nBytes = websWrite(wp, T("<OPTION VALUE=\"%s\">%s\n"), group, group);
		group = umGetNextGroup(group);
		nBytesSent += nBytes;
	}

	nBytesSent += websWrite(wp, T("</SELECT>"));

	return nBytesSent;
}

/******************************************************************************/
/*
 *  Add an access limit
 */

static void formAddAccessLimit(webs_t wp, char_t *path, char_t *query)
{
	char_t			*url, *method, *group, *secure, *ok;
	int				nCheck;
	accessMeth_t	am;
	short			nSecure;

	a_assert(wp);

	url = websGetVar(wp, T("url"), T("")); 
	group = websGetVar(wp, T("group"), T("")); 
	method = websGetVar(wp, T("method"), T("")); 
	secure = websGetVar(wp, T("secure"), T("")); 
	ok = websGetVar(wp, T("ok"), T("")); 

	websHeader(wp);
	websMsgStart(wp);

	if (gstricmp(ok, T("ok")) != 0) {
		websWrite(wp, T("Add Access Limit Cancelled."));
	} else if ((url == NULL) || (*url == 0)) {
		websWrite(wp, T("ERROR:  No URL was entered."));
	} else if (umAccessLimitExists(url)) {
		websWrite(wp, T("ERROR:  An Access Limit for [%s] already exists."),
			url);
	} else {
		if (method && *method) {
			am = (accessMeth_t) gatoi(method);
		} else {
			am = AM_FULL;
		}

		if (secure && *secure) {
			nSecure = (short) gatoi(secure);
		} else {
			nSecure = 0;
		}

		nCheck = umAddAccessLimit(url, am, nSecure, group);
		if (nCheck != 0) {
			websWrite(wp, T("Unable to add Access Limit for [%s]"),	url);
		} else {
			websWrite(wp, T("Access limit for [%s], was successfully added."),
				url);
		}
	}

	websMsgEnd(wp);
	websFooter(wp);
	websDone(wp, 200);
}

/******************************************************************************/
/*
 *  Delete an Access Limit
 */

static void formDeleteAccessLimit(webs_t wp, char_t *path, char_t *query)
{
	char_t	*url, *ok;

	a_assert(wp);

	url = websGetVar(wp, T("url"), T("")); 
	ok = websGetVar(wp, T("ok"), T("")); 

	websHeader(wp);
	websMsgStart(wp);

	if (gstricmp(ok, T("ok")) != 0) {
		websWrite(wp, T("Delete Access Limit Cancelled"));
	} else if (umDeleteAccessLimit(url) != 0) {
		websWrite(wp, T("ERROR: Unable to delete Access Limit for [%s]"), 
			url);
	} else {
		websWrite(wp, T("Access Limit for [%s], was successfully deleted."), 
			url);
	}

	websMsgEnd(wp);
	websFooter(wp);
	websDone(wp, 200);
}

/******************************************************************************/
/*
 *  Generate HTML to create a list box containing the access limits
 */

static int aspGenerateAccessLimitList(int eid, webs_t wp, 
									  int argc, char_t **argv)
{
	char_t	*url;
	int		row, nBytesSent, nBytes;

	a_assert(wp);

	row = nBytesSent = 0;
	url = umGetFirstAccessLimit();
	nBytes = websWrite(wp, 
		T("<SELECT NAME=\"url\" SIZE=\"3\" TITLE=\"Select a URL\">"));

	while (url && (nBytes > 0)) {
		nBytes = websWrite(wp, T("<OPTION VALUE=\"%s\">%s\n"), url, url);
		url = umGetNextAccessLimit(url);
		nBytesSent += nBytes;
	}

	nBytesSent += websWrite(wp, T("</SELECT>"));

	return nBytesSent;
}

/******************************************************************************/
/*
 *  Generate HTML to create a list box containing the access methods
 */

static int aspGenerateAccessMethodList(int eid, webs_t wp, 
									   int argc, char_t **argv)
{
	int		nBytes;

	a_assert(wp);

	nBytes = websWrite(wp, 
		T("<SELECT NAME=\"method\" SIZE=\"3\" TITLE=\"Select a Method\">"));
	nBytes += websWrite(wp, T("<OPTION VALUE=\"%d\">FULL ACCESS\n"), 
		AM_FULL);
	nBytes += websWrite(wp, T("<OPTION VALUE=\"%d\">BASIC ACCESS\n"), 
		AM_BASIC);
	nBytes += websWrite(wp, T("<OPTION VALUE=\"%d\" SELECTED>DIGEST ACCESS\n"), 
		AM_DIGEST);
	nBytes += websWrite(wp, T("<OPTION VALUE=\"%d\">NO ACCESS\n"), 
		AM_NONE);
	nBytes += websWrite(wp, T("</SELECT>"));

	return nBytes;
}
/******************************************************************************/
/*
 *  Generate HTML to create a list box containing privileges
 */

static int aspGeneratePrivilegeList(int eid, webs_t wp, 
									int argc, char_t **argv)
{
	int		nBytes;

	a_assert(wp);

	nBytes = websWrite(wp, T("<SELECT NAME=\"privilege\" SIZE=\"3\" "));
	nBytes += websWrite(wp, T("MULTIPLE TITLE=\"Choose Privileges\">"));
	nBytes += websWrite(wp, T("<OPTION VALUE=\"%d\">READ\n"), PRIV_READ);
	nBytes += websWrite(wp, T("<OPTION VALUE=\"%d\">EXECUTE\n"), PRIV_WRITE);
	nBytes += websWrite(wp, T("<OPTION VALUE=\"%d\">ADMINISTRATE\n"), 
		PRIV_ADMIN);
	nBytes += websWrite(wp, T("</SELECT>"));

	return nBytes;
}

/******************************************************************************/
/*
 *  Save the user management configuration to a file
 */

static void formSaveUserManagement(webs_t wp, char_t *path, char_t *query)
{
	char_t	*ok;

	a_assert(wp);

	ok = websGetVar(wp, T("ok"), T("")); 

	websHeader(wp);
	websMsgStart(wp);

	if (gstricmp(ok, T("ok")) != 0) {
		websWrite(wp, T("Save Cancelled."));
	} else if (umCommit(NULL) != 0) {
		websWrite(wp, T("ERROR: Unable to save user configuration."));
	} else {
		websWrite(wp, T("User configuration was saved successfully."));
	}

	websMsgEnd(wp);
	websFooter(wp);
	websDone(wp, 200);
}

/******************************************************************************/
/*
 *  Load the user management configuration from a file
 */

static void formLoadUserManagement(webs_t wp, char_t *path, char_t *query)
{
	char_t	*ok;

	a_assert(wp);

	ok = websGetVar(wp, T("ok"), T("")); 

	websHeader(wp);
	websMsgStart(wp);

	if (gstricmp(ok, T("ok")) != 0) {
		websWrite(wp, T("Load Cancelled."));
	} else if (umRestore(NULL) != 0) {
		websWrite(wp, T("ERROR: Unable to load user configuration."));
	} else {
		websWrite(wp, T("User configuration was re-loaded successfully."));
	}

	websMsgEnd(wp);
	websFooter(wp);
	websDone(wp, 200);
}

/******************************************************************************/
/*
 *  Message start and end convenience functions
 */

static void	websMsgStart(webs_t wp)
{
	websWrite(wp, MSG_START);
}

static void	websMsgEnd(webs_t wp)
{
	websWrite(wp, MSG_END);
}

/******************************************************************************/

