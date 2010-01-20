/*
 *	um.c -- User Management
 *
 *	Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 *	See the file "license.txt" for usage and redistribution license requirements
 *
 *	$Id: um.c,v 1.5 2002/10/24 14:44:50 bporter Exp $
 */

/******************************** Description *********************************/
/*
 *	User Management routines for adding/deleting/changing users and groups
 *  Also, routines for determining user access
 */

/********************************* Includes ***********************************/

#include	"um.h"
#include	"emfdb.h"
#include	"webs.h"

/********************************** Defines ***********************************/

#define UM_DB_FILENAME	T("um.xml")
#define UM_TXT_FILENAME	T("umconfig.txt")

/*
 *	Table names
 */
#define UM_USER_TABLENAME	T("users")
#define UM_GROUP_TABLENAME	T("groups")
#define UM_ACCESS_TABLENAME	T("access")

/*
 *	Column names
 */
#define UM_NAME			T("name")
#define UM_PASS			T("password")
#define UM_GROUP		T("group")
#define UM_PROT			T("prot")
#define UM_DISABLE		T("disable")
#define UM_METHOD		T("method")
#define UM_PRIVILEGE	T("priv")
#define UM_SECURE		T("secure")

/*
 *	XOR encryption mask
 *		Note:	This string should be modified for individual sites
 *				in order to enhance user password security.
 */
#define UM_XOR_ENCRYPT	T("*j7a(L#yZ98sSd5HfSgGjMj8;Ss;d)(*&^#@$a2s0i3g")

/******************************** Local Data **********************************/

#ifdef qHierarchicalAccess
/*
 * user-provided function to allow hierarchical access protection. See below.
 * for details.
 */
extern bool_t dmfCanAccess(const char_t* usergroup, const char_t* group);
#endif
#ifdef UEMF
/*
 *	User table definition
 */
#define NUMBER_OF_USER_COLUMNS	5

char_t	*userColumnNames[NUMBER_OF_USER_COLUMNS] = {
			UM_NAME, UM_PASS, UM_GROUP, UM_PROT, UM_DISABLE
};

int		userColumnTypes[NUMBER_OF_USER_COLUMNS] = {
			T_STRING, T_STRING, T_STRING, T_INT, T_INT
};

dbTable_t userTable = {
	UM_USER_TABLENAME,
	NUMBER_OF_USER_COLUMNS,
	userColumnNames,
	userColumnTypes,
	0,
	NULL
};

/*
 *	Group table definition
 */
#define NUMBER_OF_GROUP_COLUMNS	5

char_t	*groupColumnNames[NUMBER_OF_GROUP_COLUMNS] = {
			UM_NAME, UM_PRIVILEGE, UM_METHOD, UM_PROT, UM_DISABLE
};

int		groupColumnTypes[NUMBER_OF_GROUP_COLUMNS] = {
			T_STRING, T_INT, T_INT, T_INT, T_INT
};

dbTable_t groupTable = {
	UM_GROUP_TABLENAME,
	NUMBER_OF_GROUP_COLUMNS,
	groupColumnNames,
	groupColumnTypes,
	0,
	NULL
};

/*
 *	Access Limit table definition
 */
#define NUMBER_OF_ACCESS_COLUMNS	4

char_t	*accessColumnNames[NUMBER_OF_ACCESS_COLUMNS] = {
			UM_NAME, UM_METHOD, UM_SECURE, UM_GROUP
};

int		accessColumnTypes[NUMBER_OF_ACCESS_COLUMNS] = {
			T_STRING, T_INT, T_INT, T_STRING
};

dbTable_t accessTable = {
	UM_ACCESS_TABLENAME,
	NUMBER_OF_ACCESS_COLUMNS,
	accessColumnNames,
	accessColumnTypes,
	0,
	NULL
};
#endif	/* #ifdef UEMF */

/* 
 *	Database Identifier returned from dbOpen()
 */
static int		didUM = -1;	

/* 
 *	Configuration database persist filename
 */
static char_t	*saveFilename = NULL;

static int		umOpenCount = 0;		/* count of apps using this module */

/*************************** Forward Declarations *****************************/

static bool_t umCheckName(char_t *name);

/*********************************** Code *************************************/
/*
 *	umOpen() registers the UM tables in the fake emf-database 
 */

int umOpen()
{
	if (++umOpenCount != 1) {
		return didUM;
	}
/*
 *	Do not initialize if intialization has already taken place
 */
	if (didUM == -1) {
		didUM = dbOpen(UM_USER_TABLENAME, UM_DB_FILENAME, NULL, 0);
#ifdef UEMF
		dbRegisterDBSchema(&userTable);
		dbRegisterDBSchema(&groupTable);
		dbRegisterDBSchema(&accessTable);
#endif
	}

	if (saveFilename == NULL) {
		saveFilename = bstrdup(B_L, UM_TXT_FILENAME);
	}

	return didUM;
}

/******************************************************************************/
/*
 *	umClose() frees up the UM tables in the fake emf-database 
 */

void umClose()
{
	if (--umOpenCount > 0) {
		return;
	}
/*
 *	Do not close if intialization has not taken place
 */
	if (didUM != -1) {
		dbClose(didUM);
		didUM = -1;
	}

	if (saveFilename != NULL) {
		bfree(B_L, saveFilename);
		saveFilename = NULL;
	}
}

/******************************************************************************/
/*
 *	umCommit() persists all of the UM tables
 */

int	umCommit(char_t *filename)
{
	if (filename && *filename) {
		if (saveFilename != NULL) {
			bfree(B_L, saveFilename);
		}

		saveFilename = bstrdup(B_L, filename);
	}

	a_assert (saveFilename && *saveFilename);
	trace(3, T("UM: Writing User Configuration to file <%s>\n"), 
		saveFilename);

	return dbSave(didUM, saveFilename, 0);
}

/******************************************************************************/
/*
 *	umRestore() loads up the UM tables with persisted data
 */

int umRestore(char_t *filename)
{
	if (filename && *filename) {
		if (saveFilename != NULL) {
			bfree(B_L, saveFilename);
		}

		saveFilename = bstrdup(B_L, filename);
	}

	a_assert(saveFilename && *saveFilename);

	trace(3, T("UM: Loading User Configuration from file <%s>\n"), 
		saveFilename);

/*
 *	First empty the database, otherwise we wind up with duplicates!
 */
	dbZero(didUM);
	return dbLoad(didUM, saveFilename, 0);
}

/******************************************************************************/
/*
 *	Encrypt/Decrypt a text string.  
 *		Returns the number of characters encrypted.
 */

static int umEncryptString(char_t *textString)
{
	char_t	*enMask;
	char_t	enChar;
	int		numChars;

	a_assert(textString);

	enMask = UM_XOR_ENCRYPT;
	numChars = 0;

	while (*textString) {
		enChar = *textString ^ *enMask;
/*
 *		Do not produce encrypted text with embedded linefeeds or tabs.
 *			Simply use existing character.
 */
		if (enChar && !gisspace(enChar)) 
			*textString = enChar;
/*
 *		Increment all pointers.
 */
		enMask++;
		textString++;
		numChars++;
/*
 *		Wrap encryption mask pointer if at end of length.
 */
		if (*enMask == '\0') {
			enMask = UM_XOR_ENCRYPT;
		}
	}

	return numChars;
}

/******************************************************************************/
/*
 *	umGetFirstRowData() -	return a pointer to the first non-blank key value
 *							in the given column for the given table.
 */

static char_t *umGetFirstRowData(char_t *tableName, char_t *columnName)
{
	char_t	*columnData;
	int		row;
	int		check;

	a_assert(tableName && *tableName);
	a_assert(columnName && *columnName);

	row = 0;
/*
 *	Move through table until we retrieve the first row with non-null 
 *	column data.
 */
	columnData = NULL;
	while ((check = dbReadStr(didUM, tableName, columnName, row++, 
		&columnData)) == 0 || (check == DB_ERR_ROW_DELETED)) {
		if (columnData && *columnData) {
			return columnData;
		}
	}

	return NULL;
}

/******************************************************************************/
/*
 *	umGetNextRowData() -	return a pointer to the first non-blank 
 *						key value following the given one.
 */

static char_t *umGetNextRowData(char_t *tableName, char_t *columnName, 
								char_t *keyLast)
{
	char_t	*key;
	int		row;
	int		check;

	a_assert(tableName && *tableName);
	a_assert(columnName && *columnName);
	a_assert(keyLast && *keyLast);
/*
 *	Position row counter to row where the given key value was found
 */
	row = 0;
	key = NULL;

	while ((((check = dbReadStr(didUM, tableName, columnName, row++, 
		&key)) == 0) || (check == DB_ERR_ROW_DELETED)) &&
		((key == NULL) || (gstrcmp(key, keyLast) != 0))) {
	}
/*
 *	If the last key value was not found, return NULL
 */
	if (!key || gstrcmp(key, keyLast) != 0) {
		return NULL;
	}
/*
 *	Move through table until we retrieve the next row with a non-null key
 */
	while (((check = dbReadStr(didUM, tableName, columnName, row++, &key)) 
		== 0) || (check == DB_ERR_ROW_DELETED)) {
		if (key && *key && (gstrcmp(key, keyLast) != 0)) {
			return key;
		}
	}

	return NULL;
}

/******************************************************************************/
/*
 *	umAddUser() - Adds a user to the "users" table.
 */

int	umAddUser(char_t *user, char_t *pass, char_t *group, 
			  bool_t prot, bool_t disabled)
{
	int		row;
	char_t	*password;

	a_assert(user && *user);
	a_assert(pass && *pass);
	a_assert(group && *group);

	trace(3, T("UM: Adding User <%s>\n"), user);

/*
 *	Do not allow duplicates
 */
	if (umUserExists(user)) {
		return UM_ERR_DUPLICATE;
	}

/*
 *	Make sure user name and password contain valid characters
 */
	if (!umCheckName(user)) {
		return UM_ERR_BAD_NAME;
	}

	if (!umCheckName(pass)) {
		return UM_ERR_BAD_PASSWORD;
	}

/*
 *	Make sure group exists
 */
	if (!umGroupExists(group)) {
		return UM_ERR_NOT_FOUND;
	}

/*
 *	Now create the user record
 */
	row = dbAddRow(didUM, UM_USER_TABLENAME);

	if (row < 0) {
		return UM_ERR_GENERAL;
	}

	if (dbWriteStr(didUM, UM_USER_TABLENAME, UM_NAME, row, user) != 0) {
		return UM_ERR_GENERAL;
	}

	password = bstrdup(B_L, pass);
	umEncryptString(password);
	dbWriteStr(didUM, UM_USER_TABLENAME, UM_PASS, row, password);
	bfree(B_L, password);
	dbWriteStr(didUM, UM_USER_TABLENAME, UM_GROUP, row, group);
	dbWriteInt(didUM, UM_USER_TABLENAME, UM_PROT, row, prot); 
	dbWriteInt(didUM, UM_USER_TABLENAME, UM_DISABLE, row, disabled);

	return 0;
}

/******************************************************************************/
/*
 *	umDeleteUser() - remove a user from the "users" table
 */

int	umDeleteUser(char_t *user)
{
	int row;

	a_assert(user && *user);
	trace(3, T("UM: Deleting User <%s>\n"), user);
/*
 *	Check to see if user is delete-protected
 */
	if (umGetUserProtected(user)) {
		return UM_ERR_PROTECTED;
	} 

/*
 *	If found, delete the user from the database
 */
	if ((row = dbSearchStr(didUM, UM_USER_TABLENAME, UM_NAME, user, 0)) >= 0) {
		return dbDeleteRow(didUM, UM_USER_TABLENAME, row);
	} 

	return UM_ERR_NOT_FOUND;
}

/******************************************************************************/
/*
 *	umGetFirstUser() -	Returns the user ID of the first user found in the
 *						"users" table.
 */

char_t *umGetFirstUser()
{
	return umGetFirstRowData(UM_USER_TABLENAME, UM_NAME);
}

/******************************************************************************/
/*
 *	umGetNextUser()	Returns the next user found in the "users" table after
 *					the given user. 	
 */

char_t *umGetNextUser(char_t *userLast)
{
	return umGetNextRowData(UM_USER_TABLENAME, UM_NAME, userLast);
}

/******************************************************************************/
/*
 *	umUserExists()	Returns TRUE if userid exists.
 */

bool_t umUserExists(char_t *user)
{
	a_assert(user && *user);

	if (dbSearchStr(didUM, UM_USER_TABLENAME, UM_NAME, user, 0) >= 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}

/******************************************************************************/
/*
 *	umGetUserPassword() returns a de-crypted copy of the user password
 */

char_t *umGetUserPassword(char_t *user)
{
	char_t	*password;
	int		row;

	a_assert(user && *user);

	password = NULL;
	row = dbSearchStr(didUM, UM_USER_TABLENAME, UM_NAME, user, 0);

	if (row >= 0) {
		char_t *pass = NULL;
		dbReadStr(didUM, UM_USER_TABLENAME, UM_PASS, row, &pass);
/*
 *		Decrypt	password
 *		Note, this function returns a copy of the password, which must
 *		be deleted at some time in the future.
 */
		password = bstrdup(B_L, pass);
		umEncryptString(password);
	}

	return password;
}

/******************************************************************************/
/*
 *	umSetUserPassword() updates the user password in the user "table" after
 *						encrypting the given password
 */

int	umSetUserPassword(char_t *user, char_t *pass)
{
	int		row, nRet;
	char_t	*password;

	a_assert(user && *user);
	a_assert(pass && *pass);
	trace(3, T("UM: Attempting to change the password for user <%s>\n"), user);
/*
 *	Find the row of the user
 */
	if ((row = dbSearchStr(didUM, UM_USER_TABLENAME, UM_NAME, user, 0)) < 0) {
		return UM_ERR_NOT_FOUND;
	}

	password = bstrdup(B_L, pass);
	umEncryptString(password);
	nRet = dbWriteStr(didUM, UM_USER_TABLENAME, UM_PASS, row, password);
	bfree(B_L, password);

	return nRet;
}

/******************************************************************************/
/*
 *	umGetUserGroup() returns the name of the user group
 */

char_t *umGetUserGroup(char_t *user)
{
	char_t	*group;
	int		row;

	a_assert(user && *user);
	group = NULL;
/*
 *	Find the row of the user
 */
	row = dbSearchStr(didUM, UM_USER_TABLENAME, UM_NAME, user, 0);

	if (row >= 0) {
		dbReadStr(didUM, UM_USER_TABLENAME, UM_GROUP, row, &group);
	}

	return group;
}

/******************************************************************************/
/*
 *	umSetUserGroup() Sets the name of the user group for the user
 */

int	umSetUserGroup(char_t *user, char_t *group)
{
	int row;

	a_assert(user && *user);
	a_assert(group && *group);
/*
 *	Find the row of the user
 */
	row = dbSearchStr(didUM, UM_USER_TABLENAME, UM_NAME, user, 0);

	if (row >= 0) {
		return dbWriteStr(didUM, UM_USER_TABLENAME, UM_GROUP, row, group);
	} else {
		return UM_ERR_NOT_FOUND;
	}
}

/******************************************************************************/
/*
 *	umGetUserEnabled() - returns if the user is enabled
 *	Returns FALSE if the user is not found.
 */

bool_t	umGetUserEnabled(char_t *user)
{
	int disabled, row;

	a_assert(user && *user);

	disabled = 1;
/*
 *	Find the row of the user
 */
	row = dbSearchStr(didUM, UM_USER_TABLENAME, UM_NAME, user, 0);

	if (row >= 0) {
		dbReadInt(didUM, UM_USER_TABLENAME, UM_DISABLE, row, &disabled);
	}

	return (bool_t)!disabled;
}

/******************************************************************************/
/*
 *	umSetUserEnabled() Enables/disables the user
 */
int	umSetUserEnabled(char_t *user, bool_t enabled)
{
	int row;

	a_assert(user && *user);
/*
 *	Find the row of the user
 */
	row = dbSearchStr(didUM, UM_USER_TABLENAME, UM_NAME, user, 0);
	if (row >= 0) {
		return dbWriteInt(didUM, UM_USER_TABLENAME, UM_DISABLE, row, !enabled);
	} else {
		return UM_ERR_NOT_FOUND;
	}
}

/******************************************************************************/
/*
 *	umGetUserProtected() - determine deletability of user
 */

bool_t umGetUserProtected(char_t *user)
{
	int protect, row;

	a_assert(user && *user);
/*
 *	Find the row of the user
 */
	row = dbSearchStr(didUM, UM_USER_TABLENAME, UM_NAME, user, 0);
	protect = FALSE;

	if (row >= 0) {
		dbReadInt(didUM, UM_USER_TABLENAME, UM_PROT, row, &protect);
	}

	return (bool_t)protect;
}

/******************************************************************************/
/*
 *	umSetUserProtected() sets the delete protection for the user
 */
int	umSetUserProtected(char_t *user, bool_t protect)
{
	int row;

	a_assert(user && *user);
/*
 *	Find the row of the user
 */
	row = dbSearchStr(didUM, UM_USER_TABLENAME, UM_NAME, user, 0);

	if (row >= 0) {
		return dbWriteInt(didUM, UM_USER_TABLENAME, UM_PROT, row, protect);
	} else {
		return UM_ERR_NOT_FOUND;
	}
}


/******************************************************************************/
/*
 *	umAddGroup() adds a group to the "Group" table
 */

int	umAddGroup(char_t *group, short priv, accessMeth_t am, 
			   bool_t prot, bool_t disabled)
{
	int row;

	a_assert(group && *group);
	trace(3, T("UM: Adding group <%s>\n"), group);
	
/*
 *	Do not allow duplicates
 */
	if (umGroupExists(group)) {
		return UM_ERR_DUPLICATE;
	}

/*
 *	Only allow valid characters in key field
 */
	if (!umCheckName(group)) {
		return UM_ERR_BAD_NAME;
	}

/*
 *	Add a new row to the table
 */
	if ((row = dbAddRow(didUM, UM_GROUP_TABLENAME)) < 0) {
		return UM_ERR_GENERAL;
	}

/*
 *	Write the key field
 */
	if (dbWriteStr(didUM, UM_GROUP_TABLENAME, UM_NAME, row, group) != 0) {
		return UM_ERR_GENERAL;
	}

/*
 *	Write the remaining fields
 */
	dbWriteInt(didUM, UM_GROUP_TABLENAME, UM_PRIVILEGE, row, priv);
	dbWriteInt(didUM, UM_GROUP_TABLENAME, UM_METHOD, row, (int) am);
	dbWriteInt(didUM, UM_GROUP_TABLENAME, UM_PROT, row, prot);
	dbWriteInt(didUM, UM_GROUP_TABLENAME, UM_DISABLE, row, disabled);

	return 0;
}

/******************************************************************************/
/*
 *	umDeleteGroup() - Delete a user group, if not protected
 */

int	umDeleteGroup(char_t *group)
{
	int row;

	a_assert(group && *group);
	trace(3, T("UM: Deleting Group <%s>\n"), group);

/*
 *	Check to see if the group is in use
 */
	if (umGetGroupInUse(group)) {
		return UM_ERR_IN_USE;
	} 

/*
 *	Check to see if the group is delete-protected
 */
	if (umGetGroupProtected(group)) {
		return UM_ERR_PROTECTED;
	} 

/*
 *	Find the row of the group to delete
 */
	if ((row = dbSearchStr(didUM, UM_GROUP_TABLENAME, UM_NAME, group, 0)) < 0) {
		return UM_ERR_NOT_FOUND;
	}

	return dbDeleteRow(didUM, UM_GROUP_TABLENAME, row);
}

/******************************************************************************/
/*
 *	umGroupExists() returns TRUE if group exists, FALSE otherwise
 */

bool_t umGroupExists(char_t *group)
{
	a_assert(group && *group);

	if (dbSearchStr(didUM, UM_GROUP_TABLENAME, UM_NAME, group, 0) >= 0) {
		return TRUE;
	} else {
		return FALSE;
	}
}


/******************************************************************************/
/*
 *	umGetGroupInUse() returns TRUE if the group is referenced by a user or by
 *  an access limit.
 */

bool_t umGetGroupInUse(char_t *group)
{
	a_assert(group && *group);

/*
 *	First, check the user table
 */
	if (dbSearchStr(didUM, UM_USER_TABLENAME, UM_GROUP, group, 0) >= 0) {
		return TRUE;
	} 

/*
 *	Second, check the access limit table
 */
	if (dbSearchStr(didUM, UM_ACCESS_TABLENAME, UM_GROUP, group, 0) >= 0) {
		return TRUE;
	} 

	return FALSE;
}


/******************************************************************************/
/*
 *	umGetFirstGroup() - return a pointer to the first non-blank group name
 */

char_t *umGetFirstGroup()
{
	return umGetFirstRowData(UM_GROUP_TABLENAME, UM_NAME);
}

/******************************************************************************/
/*
 *	umGetNextGroup() -	return a pointer to the first non-blank group name
 *						following the given group name
 */

char_t *umGetNextGroup(char_t *groupLast)
{
	return umGetNextRowData(UM_GROUP_TABLENAME, UM_NAME, groupLast);
}

/******************************************************************************/
/*
 *	Returns the default access method to use for a given group
 */

accessMeth_t umGetGroupAccessMethod(char_t *group)
{
	int am, row;

	a_assert(group && *group);
	row = dbSearchStr(didUM, UM_GROUP_TABLENAME, UM_NAME, group, 0);

	if (row >= 0) {
		dbReadInt(didUM, UM_GROUP_TABLENAME, UM_METHOD, row, (int *)&am);
	} else {
		am = AM_INVALID;
	}

	return (accessMeth_t) am;
}

/******************************************************************************/
/*
 *	Set the default access method to use for a given group
 */

int	umSetGroupAccessMethod(char_t *group, accessMeth_t am)
{
	int row;

	a_assert(group && *group);
	row = dbSearchStr(didUM, UM_GROUP_TABLENAME, UM_NAME, group, 0);

	if (row >= 0) {
		return dbWriteInt(didUM, UM_GROUP_TABLENAME, UM_METHOD, row, (int) am);
	} else {
		return UM_ERR_NOT_FOUND;
	}
}

/******************************************************************************/
/*
 *	Returns the privilege mask for a given group
 */

short umGetGroupPrivilege(char_t *group)
{
	int privilege, row;

	a_assert(group && *group);
	privilege = -1;
	row = dbSearchStr(didUM, UM_GROUP_TABLENAME, UM_NAME, group, 0);

	if (row >= 0) {
		dbReadInt(didUM, UM_GROUP_TABLENAME, UM_PRIVILEGE, row, &privilege);
	}

	return (short) privilege;
}

/******************************************************************************/
/*
 *	Set the privilege mask for a given group
 */

int	umSetGroupPrivilege(char_t *group, short privilege)
{
	int row;

	a_assert(group && *group);
	row = dbSearchStr(didUM, UM_GROUP_TABLENAME, UM_NAME, group, 0);

	if (row >= 0) {
		return dbWriteInt(didUM, UM_GROUP_TABLENAME, UM_PRIVILEGE, row, 
			(int)privilege);
	} else {
		return UM_ERR_NOT_FOUND;
	}
}

/******************************************************************************/
/*
 *	Returns the enabled setting for a given group.
 *	Returns FALSE if group is not found.
 */

bool_t umGetGroupEnabled(char_t *group)
{
	int disabled, row;

	a_assert(group && *group);
	row = dbSearchStr(didUM, UM_GROUP_TABLENAME, UM_NAME, group, 0);
	disabled = 1;

	if (row >= 0) {
		dbReadInt(didUM, UM_GROUP_TABLENAME, UM_DISABLE, row, &disabled);
	}

	return (bool_t) !disabled;
}

/******************************************************************************/
/*
 *	Sets the enabled setting for a given group.
 */

int umSetGroupEnabled(char_t *group, bool_t enabled)
{
	int row;

	a_assert(group && *group);
	row = dbSearchStr(didUM, UM_GROUP_TABLENAME, UM_NAME, group, 0);

	if (row >= 0) {
		return dbWriteInt(didUM, UM_GROUP_TABLENAME, UM_DISABLE, row, 
			(int) !enabled);
	} else {
		return UM_ERR_NOT_FOUND;
	}
}

/******************************************************************************/
/*
 *	Returns the protected setting for a given group
 *  Returns FALSE if user is not found
 */

bool_t umGetGroupProtected(char_t *group)
{
	int protect, row;

	a_assert(group && *group);

	protect = 0;
	row = dbSearchStr(didUM, UM_GROUP_TABLENAME, UM_NAME, group, 0);
	if (row >= 0) {
		dbReadInt(didUM, UM_GROUP_TABLENAME, UM_PROT, row, &protect);
	}

	return (bool_t) protect;
}

/******************************************************************************/
/*
 *	Sets the protected setting for a given group
 */

int	umSetGroupProtected(char_t *group, bool_t protect)
{
	int row;

	a_assert(group && *group);
	row = dbSearchStr(didUM, UM_GROUP_TABLENAME, UM_NAME, group, 0);

	if (row >= 0) {
		return dbWriteInt(didUM, UM_GROUP_TABLENAME, UM_PROT, row, 
			(int) protect);
	} else {
		return UM_ERR_NOT_FOUND;
	}
}


/******************************************************************************/
/*
 *	umAddAccessLimit() adds an access limit to the "access" table
 */

int	umAddAccessLimit(char_t *url, accessMeth_t am, short secure, char_t *group)
{
	int row;

	a_assert(url && *url);
	trace(3, T("UM: Adding Access Limit for <%s>\n"), url);

/*
 *	Do not allow duplicates
 */
	if (umAccessLimitExists(url)) {
		return UM_ERR_DUPLICATE;
	}

/*
 *	Add a new row to the table
 */
	if ((row = dbAddRow(didUM, UM_ACCESS_TABLENAME)) < 0) {
		return UM_ERR_GENERAL;
	}

/*
 *	Write the key field
 */
	if(dbWriteStr(didUM, UM_ACCESS_TABLENAME, UM_NAME, row, url) < 0) {
		return UM_ERR_GENERAL;
	}

/*
 *	Write the remaining fields
 */
	dbWriteInt(didUM, UM_ACCESS_TABLENAME, UM_METHOD, row, (int)am);
	dbWriteInt(didUM, UM_ACCESS_TABLENAME, UM_SECURE, row, (int)secure);
	dbWriteStr(didUM, UM_ACCESS_TABLENAME, UM_GROUP, row, group);

	return 0;
}

/******************************************************************************/
/*
 *	umDeleteAccessLimit()
 */

int	umDeleteAccessLimit(char_t *url)
{
	int row;

	a_assert(url && *url);
	trace(3, T("UM: Deleting Access Limit for <%s>\n"), url);
/*
 *	Find the row of the access limit to delete
 */
	if ((row = dbSearchStr(didUM, UM_ACCESS_TABLENAME, UM_NAME, url, 0)) < 0) {
		return UM_ERR_NOT_FOUND;
	}

	return dbDeleteRow(didUM, UM_ACCESS_TABLENAME, row);
}

/******************************************************************************/
/*
 *	umGetFirstGroup() - return a pointer to the first non-blank access limit
 */

char_t *umGetFirstAccessLimit()
{
	return umGetFirstRowData(UM_ACCESS_TABLENAME, UM_NAME);
}

/******************************************************************************/
/*
 *	umGetNextAccessLimit() -	return a pointer to the first non-blank 
 *								access limit following the given one
 */

char_t *umGetNextAccessLimit(char_t *urlLast)
{
	return umGetNextRowData(UM_ACCESS_TABLENAME, UM_NAME, urlLast);
}

/******************************************************************************/
/*
 *	umAccessLimitExists() returns TRUE if this access limit exists
 */

bool_t	umAccessLimitExists(char_t *url)
{
	a_assert(url && *url);

	if (dbSearchStr(didUM, UM_ACCESS_TABLENAME, UM_NAME, url, 0) < 0) {
		return FALSE;
	} else {
		return TRUE;
	}
}

/******************************************************************************/
/*
 *	umGetAccessLimit() returns the Access Method for the URL
 */

accessMeth_t umGetAccessLimitMethod(char_t *url)
{
	int am, row;

	am = (int) AM_INVALID;
	row = dbSearchStr(didUM, UM_ACCESS_TABLENAME, UM_NAME, url, 0);

	if (row >= 0) {
		dbReadInt(didUM, UM_ACCESS_TABLENAME, UM_METHOD, row, &am);
	} 

	return (accessMeth_t) am;
}

/******************************************************************************/
/*
 *	umSetAccessLimitMethod() - set Access Method for Access Limit
 */

int	umSetAccessLimitMethod(char_t *url, accessMeth_t am)
{
	int row;

	a_assert(url && *url);
	row = dbSearchStr(didUM, UM_ACCESS_TABLENAME, UM_NAME, url, 0);

	if (row >= 0) {
		return dbWriteInt(didUM, UM_ACCESS_TABLENAME, UM_METHOD, row, (int) am);
	} else {
		return UM_ERR_NOT_FOUND;
	}
}

/******************************************************************************/
/*
 *	umGetAccessLimitSecure() - returns secure switch for access limit
 */

short umGetAccessLimitSecure(char_t *url)
{
	int secure, row;

	a_assert(url && *url);
	secure = -1;
	row = dbSearchStr(didUM, UM_ACCESS_TABLENAME, UM_NAME, url, 0);

	if (row >= 0) {
		dbReadInt(didUM, UM_ACCESS_TABLENAME, UM_SECURE, row, &secure);
	}

	return (short)secure;
}

/******************************************************************************/
/*
 *	umSetAccessLimitSecure() - sets the secure flag for the URL
 */

int	umSetAccessLimitSecure(char_t *url, short secure)
{
	int row;

	a_assert(url && *url);
	row = dbSearchStr(didUM, UM_ACCESS_TABLENAME, UM_NAME, url, 0);

	if (row >= 0) {
		return dbWriteInt(didUM, UM_ACCESS_TABLENAME, UM_SECURE, row, 
			(int)secure);
	} else {
		return UM_ERR_NOT_FOUND;
	}
}

/******************************************************************************/
/*
 *	umGetAccessLimitGroup() - returns the user group of the access limit
 */

char_t *umGetAccessLimitGroup(char_t *url)
{
	char_t	*group;
	int		row;

	a_assert(url && *url);
	group = NULL;
	row = dbSearchStr(didUM, UM_ACCESS_TABLENAME, UM_NAME, url, 0);

	if (row >= 0) {
		dbReadStr(didUM, UM_ACCESS_TABLENAME, UM_GROUP, row, &group);
	}

	return group;
}

/******************************************************************************/
/*
 *	umSetAccessLimitGroup() - sets the user group for the access limit.
 */

int	umSetAccessLimitGroup(char_t *url, char_t *group)
{
	int row;

	a_assert(url && *url);
	row = dbSearchStr(didUM, UM_ACCESS_TABLENAME, UM_NAME, url, 0);

	if (row >= 0) {
		return dbWriteStr(didUM, UM_ACCESS_TABLENAME, UM_GROUP, row, group);
	} else {
		return UM_ERR_NOT_FOUND;
	}
}

/******************************************************************************/
/*
 *	Returns the access limit to use for a given URL, by checking for URLs up
 *	the directory tree.  Creates a new string that must be deleted.
 */

char_t *umGetAccessLimit(char_t *url)
{
	char_t	*urlRet, *urlCheck, *lastChar;
	int		len;
	
	a_assert(url && *url);
	urlRet = NULL;
	urlCheck = bstrdup(B_L, url);
	a_assert(urlCheck);
	len = gstrlen(urlCheck);
/*
 *	Scan back through URL to see if there is a "parent" access limit
 */
	while (len && !urlRet) {
		if (umAccessLimitExists(urlCheck)) {
			urlRet = bstrdup(B_L, urlCheck);
		} else {
/*
 *	Trim the end portion of the URL to the previous directory marker
 */
			lastChar = urlCheck + len;
			lastChar--;

			while ((lastChar >= urlCheck) && ((*lastChar == '/') || 
				(*lastChar == '\\'))) {
				*lastChar = 0;
				lastChar--;
			}

			while ((lastChar >= urlCheck) && (*lastChar != '/') && 
				(*lastChar != '\\')) {
				*lastChar = 0;
				lastChar--;
			}

			len = gstrlen(urlCheck);
		}
	}
	bfree (B_L, urlCheck);

	return urlRet;
}

/******************************************************************************/
/*
 *	Returns the access method to use for a given URL
 */

accessMeth_t umGetAccessMethodForURL(char_t *url)
{
	accessMeth_t	amRet;
	char_t			*urlHavingLimit, *group;
	
	urlHavingLimit = umGetAccessLimit(url);
	if (urlHavingLimit) {
		group = umGetAccessLimitGroup(urlHavingLimit);

		if (group && *group) {
			amRet = umGetGroupAccessMethod(group);
		} else {
			amRet = umGetAccessLimitMethod(urlHavingLimit);
		}

		bfree(B_L, urlHavingLimit);
	} else {
		amRet = AM_FULL;
	}

	return amRet;
}

/******************************************************************************/
/*
 *	Returns TRUE if user can access URL
 */

bool_t umUserCanAccessURL(char_t *user, char_t *url)
{
	accessMeth_t	amURL;
	char_t			*group, *usergroup, *urlHavingLimit;
	short			priv;
	
	a_assert(user && *user);
	a_assert(url && *url);

/*
 *	Make sure user exists
 */
	if (!umUserExists(user)) {
		return FALSE;
	}

/*
 *	Make sure user is enabled
 */
	if (!umGetUserEnabled(user)) {
		return FALSE;
	}

/*
 *	Make sure user has sufficient privileges (any will do)
 */
	usergroup = umGetUserGroup(user);
	priv = umGetGroupPrivilege(usergroup);
	if (priv == 0) {
		return FALSE;
	}

/*
 *	Make sure user's group is enabled
 */
	if (!umGetGroupEnabled(usergroup)) {
		return FALSE;
	}

/*
 *	The access method of the user group must not be AM_NONE
 */
	if (umGetGroupAccessMethod(usergroup) == AM_NONE) {
		return FALSE;
	}

/*
 *	Check to see if there is an Access Limit for this URL
 */
	urlHavingLimit = umGetAccessLimit(url);
	if (urlHavingLimit) {
		amURL = umGetAccessLimitMethod(urlHavingLimit);
		group = umGetAccessLimitGroup(urlHavingLimit);
		bfree(B_L, urlHavingLimit);
	} else {
/*
 *		If there isn't an access limit for the URL, user has full access
 */
		return TRUE;
	}

/*
 *	If the access method for the URL is AM_NONE then 
 *	the file "doesn't exist".
 */
	if (amURL == AM_NONE) {
		return FALSE;
	} 
	
/*
 *	If Access Limit has a group specified, then the user must be a 
 *	member of that group
 */
	if (group && *group) {
#ifdef qHierarchicalAccess
      /*
       * If we are compiling with the hierarchical access extensions, we
       * instead call the user-provided function that checks to see whether
       * the current user's access level is greater than or equal to the
       * access level required for this URL.
       */
      return dmfCanAccess(usergroup, group);

#else
		if (usergroup && (gstrcmp(group, usergroup) != 0)) {
			return FALSE;

		}
#endif
	} 

/*
 *	Otherwise, user can access the URL 
 */
	return TRUE;

}

/******************************************************************************/
/*
 *	Returns TRUE if given name has only valid chars
 */

static bool_t umCheckName(char_t *name)
{
	a_assert(name && *name);

	if (name && *name) {
		while (*name) {
			if (gisspace(*name)) {
				return FALSE;
			}

			name++;
		}

		return TRUE;
	}

	return FALSE;
}

/******************************************************************************/

