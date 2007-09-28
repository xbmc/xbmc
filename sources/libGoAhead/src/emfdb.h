/*
 *	emfdb.h -- EMF database compatability functions for GoAhead WebServer.
 *
 *	Copyright (c) GoAhead Software Inc., 1995-2000. All Rights Reserved.
 *
 *	See the file "license.txt" for usage and redistribution license requirements
 *
 *	$Id: emfdb.h,v 1.3 2003/09/29 19:48:34 bporter Exp $
 */

/******************************** Description *********************************/
/*
 *	Emf-like textfile database support for WebServer 2.1.
 */

/********************************* Includes ***********************************/

#ifndef _h_EMFDB
#define _h_EMFDB 1

#ifndef UEMF
	#include	"basic/basic.h"
	#include	"emf/emf.h"
#else
	#include	"uemf.h"
#endif


/********************************* Defines ************************************/

#define		T_INT					0
#define		T_STRING				1

#define		DB_OK					0
#define		DB_ERR_GENERAL			-1
#define		DB_ERR_COL_NOT_FOUND	-2
#define		DB_ERR_COL_DELETED		-3
#define		DB_ERR_ROW_NOT_FOUND	-4
#define		DB_ERR_ROW_DELETED		-5
#define		DB_ERR_TABLE_NOT_FOUND	-6
#define		DB_ERR_TABLE_DELETED	-7
#define		DB_ERR_BAD_FORMAT		-8

/*
 * 30 Jun 03 BgP -- pass DB_CASE_INSENSITIVE as the "flags" argument to
 * dbSearchString() to force a case-insensitive search.
 */
#define     DB_CASE_INSENSITIVE  1

typedef struct dbTable_s {
	char_t	*name;
	int		nColumns;
	char_t	**columnNames;
	int		*columnTypes;
	int		nRows;
	int		**rows;
} dbTable_t;

/********************************** Prototypes ********************************/

/*
 *	Add a schema to the module-internal schema database
 */
extern int		dbRegisterDBSchema(dbTable_t *sTable);

extern int		dbOpen(char_t *databasename, char_t *filename,
					int (*gettime)(int did), int flags);
extern void		dbClose(int did);
extern int		dbGetTableId(int did, char_t *tname);
extern char_t	*dbGetTableName(int did, int tid);
extern int		dbReadInt(int did, char_t *table, char_t *column, int row,
					int *returnValue);
extern int		dbReadStr(int did, char_t *table, char_t *column, int row,
					char_t **returnValue);
extern int		dbWriteInt(int did, char_t *table, char_t *column, int row,
					int idata);
extern int		dbWriteStr(int did, char_t *table, char_t *column, int row,
					char_t *s);
extern int		dbAddRow(int did, char_t *table);
extern int		dbDeleteRow(int did, char_t *table, int rid);
extern int		dbSetTableNrow(int did, char_t *table, int nNewRows);
extern int		dbGetTableNrow(int did, char_t *table);

/*
 *	Dump the contents of a database to file
 */
extern int		dbSave(int did, char_t *filename, int flags);

/*
 *	Load the contents of a database to file
 */
extern int		dbLoad(int did, char_t *filename, int flags);

/*
 *	Search for a data in a given column
 *	30 Jun 03 BgP: If the value of 'flags' is DB_CASE_INSENSITIVE, use a
 *	case-insensitive string compare when searching.
 */
extern int		dbSearchStr(int did, char_t *table, char_t *column,
					char_t *value, int flags);

extern void		dbZero(int did);

extern char_t	*basicGetProductDir();
extern void		basicSetProductDir(char_t *proddir);

#endif /* _h_EMFDB */

/******************************************************************************/


