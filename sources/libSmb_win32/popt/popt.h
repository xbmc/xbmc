/** \file popt/popt.h
 * \ingroup popt
 */

/* (C) 1998-2000 Red Hat, Inc. -- Licensing details are in the COPYING
   file accompanying popt source distributions, available from 
   ftp://ftp.rpm.org/pub/rpm/dist. */

#ifndef H_POPT
#define H_POPT

#include <stdio.h>			/* for FILE * */

#define POPT_OPTION_DEPTH	10

/** \ingroup popt
 * \name Arg type identifiers
 */
/*@{*/
#define POPT_ARG_NONE		0	/*!< no arg */
#define POPT_ARG_STRING		1	/*!< arg will be saved as string */
#define POPT_ARG_INT		2	/*!< arg will be converted to int */
#define POPT_ARG_LONG		3	/*!< arg will be converted to long */
#define POPT_ARG_INCLUDE_TABLE	4	/*!< arg points to table */
#define POPT_ARG_CALLBACK	5	/*!< table-wide callback... must be
					   set first in table; arg points 
					   to callback, descrip points to 
					   callback data to pass */
#define POPT_ARG_INTL_DOMAIN    6       /*!< set the translation domain
					   for this table and any
					   included tables; arg points
					   to the domain string */
#define POPT_ARG_VAL		7	/*!< arg should take value val */
#define	POPT_ARG_FLOAT		8	/*!< arg will be converted to float */
#define	POPT_ARG_DOUBLE		9	/*!< arg will be converted to double */

#define POPT_ARG_MASK		0x0000FFFF
/*@}*/

/** \ingroup popt
 * \name Arg modifiers
 */
/*@{*/
#define POPT_ARGFLAG_ONEDASH	0x80000000  /*!< allow -longoption */
#define POPT_ARGFLAG_DOC_HIDDEN 0x40000000  /*!< don't show in help/usage */
#define POPT_ARGFLAG_STRIP	0x20000000  /*!< strip this arg from argv(only applies to long args) */
#define	POPT_ARGFLAG_OPTIONAL	0x10000000  /*!< arg may be missing */

#define	POPT_ARGFLAG_OR		0x08000000  /*!< arg will be or'ed */
#define	POPT_ARGFLAG_NOR	0x09000000  /*!< arg will be nor'ed */
#define	POPT_ARGFLAG_AND	0x04000000  /*!< arg will be and'ed */
#define	POPT_ARGFLAG_NAND	0x05000000  /*!< arg will be nand'ed */
#define	POPT_ARGFLAG_XOR	0x02000000  /*!< arg will be xor'ed */
#define	POPT_ARGFLAG_NOT	0x01000000  /*!< arg will be negated */
#define POPT_ARGFLAG_LOGICALOPS \
        (POPT_ARGFLAG_OR|POPT_ARGFLAG_AND|POPT_ARGFLAG_XOR)

#define	POPT_BIT_SET	(POPT_ARG_VAL|POPT_ARGFLAG_OR)
					/*!< set arg bit(s) */
#define	POPT_BIT_CLR	(POPT_ARG_VAL|POPT_ARGFLAG_NAND)
					/*!< clear arg bit(s) */

#define	POPT_ARGFLAG_SHOW_DEFAULT 0x00800000 /*!< show default value in --help */

/*@}*/

/** \ingroup popt
 * \name Callback modifiers
 */
/*@{*/
#define POPT_CBFLAG_PRE		0x80000000  /*!< call the callback before parse */
#define POPT_CBFLAG_POST	0x40000000  /*!< call the callback after parse */
#define POPT_CBFLAG_INC_DATA	0x20000000  /*!< use data from the include line,
					       not the subtable */
#define POPT_CBFLAG_SKIPOPTION	0x10000000  /*!< don't callback with option */
#define POPT_CBFLAG_CONTINUE	0x08000000  /*!< continue callbacks with option */
/*@}*/

/** \ingroup popt
 * \name Error return values
 */
/*@{*/
#define POPT_ERROR_NOARG	-10	/*!< missing argument */
#define POPT_ERROR_BADOPT	-11	/*!< unknown option */
#define POPT_ERROR_OPTSTOODEEP	-13	/*!< aliases nested too deeply */
#define POPT_ERROR_BADQUOTE	-15	/*!< error in paramter quoting */
#define POPT_ERROR_ERRNO	-16	/*!< errno set, use strerror(errno) */
#define POPT_ERROR_BADNUMBER	-17	/*!< invalid numeric value */
#define POPT_ERROR_OVERFLOW	-18	/*!< number too large or too small */
#define	POPT_ERROR_BADOPERATION	-19	/*!< mutually exclusive logical operations requested */
#define	POPT_ERROR_NULLARG	-20	/*!< opt->arg should not be NULL */
#define	POPT_ERROR_MALLOC	-21	/*!< memory allocation failed */
/*@}*/

/** \ingroup popt
 * \name poptBadOption() flags
 */
/*@{*/
#define POPT_BADOPTION_NOALIAS  (1 << 0)  /*!< don't go into an alias */
/*@}*/

/** \ingroup popt
 * \name poptGetContext() flags
 */
/*@{*/
#define POPT_CONTEXT_NO_EXEC	(1 << 0)  /*!< ignore exec expansions */
#define POPT_CONTEXT_KEEP_FIRST	(1 << 1)  /*!< pay attention to argv[0] */
#define POPT_CONTEXT_POSIXMEHARDER (1 << 2) /*!< options can't follow args */
#define POPT_CONTEXT_ARG_OPTS	(1 << 4) /*!< return args as options with value 0 */
/*@}*/

/** \ingroup popt
 */
struct poptOption {
/*@observer@*/ /*@null@*/
    const char * longName;	/*!< may be NULL */
    char shortName;		/*!< may be '\0' */
    int argInfo;
/*@shared@*/ /*@null@*/
    void * arg;			/*!< depends on argInfo */
    int val;			/*!< 0 means don't return, just update flag */
/*@observer@*/ /*@null@*/
    const char * descrip;	/*!< description for autohelp -- may be NULL */
/*@observer@*/ /*@null@*/
    const char * argDescrip;	/*!< argument description for autohelp */
};

/** \ingroup popt
 * A popt alias argument for poptAddAlias().
 */
struct poptAlias {
/*@owned@*/ /*@null@*/
    const char * longName;	/*!< may be NULL */
    char shortName;		/*!< may be '\0' */
    int argc;
/*@owned@*/
    const char ** argv;		/*!< must be free()able */
};

/** \ingroup popt
 * A popt alias or exec argument for poptAddItem().
 */
/*@-exporttype@*/
typedef struct poptItem_s {
    struct poptOption option;	/*!< alias/exec name(s) and description. */
    int argc;			/*!< (alias) no. of args. */
/*@owned@*/
    const char ** argv;		/*!< (alias) args, must be free()able. */
} * poptItem;
/*@=exporttype@*/

/** \ingroup popt
 * \name Auto-generated help/usage
 */
/*@{*/

/**
 * Empty table marker to enable displaying popt alias/exec options.
 */
/*@-exportvar@*/
/*@unchecked@*/ /*@observer@*/
extern struct poptOption poptAliasOptions[];
/*@=exportvar@*/
#define POPT_AUTOALIAS { NULL, '\0', POPT_ARG_INCLUDE_TABLE, poptAliasOptions, \
			0, "Options implemented via popt alias/exec:", NULL },

/**
 * Auto help table options.
 */
/*@-exportvar@*/
/*@unchecked@*/ /*@observer@*/
extern struct poptOption poptHelpOptions[];
/*@=exportvar@*/
#define POPT_AUTOHELP { NULL, '\0', POPT_ARG_INCLUDE_TABLE, poptHelpOptions, \
			0, "Help options:", NULL },

#define POPT_TABLEEND { NULL, '\0', 0, 0, 0, NULL, NULL }
/*@}*/

/** \ingroup popt
 */
/*@-exporttype@*/
typedef /*@abstract@*/ struct poptContext_s * poptContext;
/*@=exporttype@*/

/** \ingroup popt
 */
#ifndef __cplusplus
/*@-exporttype -typeuse@*/
typedef struct poptOption * poptOption;
/*@=exporttype =typeuse@*/
#endif

/*@-exportconst@*/
enum poptCallbackReason {
    POPT_CALLBACK_REASON_PRE	= 0,
    POPT_CALLBACK_REASON_POST	= 1,
    POPT_CALLBACK_REASON_OPTION = 2
};
/*@=exportconst@*/

#ifdef __cplusplus
extern "C" {
#endif
/*@-type@*/

/** \ingroup popt
 * Table callback prototype.
 * @param con		context
 * @param reason	reason for callback
 * @param opt		option that triggered callback
 * @param arg		@todo Document.
 * @param data		@todo Document.
 */
typedef void (*poptCallbackType) (poptContext con,
		enum poptCallbackReason reason,
		/*@null@*/ const struct poptOption * opt,
		/*@null@*/ const char * arg,
		/*@null@*/ const void * data)
	/*@*/;

/** \ingroup popt
 * Initialize popt context.
 * @param name		context name (usually argv[0] program name)
 * @param argc		no. of arguments
 * @param argv		argument array
 * @param options	address of popt option table
 * @param flags		or'd POPT_CONTEXT_* bits
 * @return		initialized popt context
 */
/*@only@*/ /*@null@*/ poptContext poptGetContext(
		/*@dependent@*/ /*@keep@*/ const char * name,
		int argc, /*@dependent@*/ /*@keep@*/ const char ** argv,
		/*@dependent@*/ /*@keep@*/ const struct poptOption * options,
		int flags)
	/*@*/;

/** \ingroup popt
 * Reinitialize popt context.
 * @param con		context
 */
/*@unused@*/
void poptResetContext(/*@null@*/poptContext con)
	/*@modifies con @*/;

/** \ingroup popt
 * Return value of next option found.
 * @param con		context
 * @return		next option val, -1 on last item, POPT_ERROR_* on error
 */
int poptGetNextOpt(/*@null@*/poptContext con)
	/*@globals fileSystem, internalState @*/
	/*@modifies con, fileSystem, internalState @*/;

/** \ingroup popt
 * Return next option argument (if any).
 * @param con		context
 * @return		option argument, NULL if no argument is available
 */
/*@observer@*/ /*@null@*/ const char * poptGetOptArg(/*@null@*/poptContext con)
	/*@modifies con @*/;

/** \ingroup popt
 * Return next argument.
 * @param con		context
 * @return		next argument, NULL if no argument is available
 */
/*@observer@*/ /*@null@*/ const char * poptGetArg(/*@null@*/poptContext con)
	/*@modifies con @*/;

/** \ingroup popt
 * Peek at current argument.
 * @param con		context
 * @return		current argument, NULL if no argument is available
 */
/*@observer@*/ /*@null@*/ const char * poptPeekArg(/*@null@*/poptContext con)
	/*@*/;

/** \ingroup popt
 * Return remaining arguments.
 * @param con		context
 * @return		argument array, NULL terminated
 */
/*@observer@*/ /*@null@*/ const char ** poptGetArgs(/*@null@*/poptContext con)
	/*@modifies con @*/;

/** \ingroup popt
 * Return the option which caused the most recent error.
 * @param con		context
 * @param flags
 * @return		offending option
 */
/*@observer@*/ const char * poptBadOption(/*@null@*/poptContext con, int flags)
	/*@*/;

/** \ingroup popt
 * Destroy context.
 * @param con		context
 * @return		NULL always
 */
/*@null@*/ poptContext poptFreeContext( /*@only@*/ /*@null@*/ poptContext con)
	/*@modifies con @*/;

/** \ingroup popt
 * Add arguments to context.
 * @param con		context
 * @param argv		argument array, NULL terminated
 * @return		0 on success, POPT_ERROR_OPTSTOODEEP on failure
 */
int poptStuffArgs(poptContext con, /*@keep@*/ const char ** argv)
	/*@modifies con @*/;

/** \ingroup popt
 * Add alias to context.
 * @todo Pass alias by reference, not value.
 * @deprecated Use poptAddItem instead.
 * @param con		context
 * @param alias		alias to add
 * @param flags		(unused)
 * @return		0 on success
 */
/*@unused@*/
int poptAddAlias(poptContext con, struct poptAlias alias, int flags)
	/*@modifies con @*/;

/** \ingroup popt
 * Add alias/exec item to context.
 * @param con		context
 * @param newItem	alias/exec item to add
 * @param flags		0 for alias, 1 for exec
 * @return		0 on success
 */
int poptAddItem(poptContext con, poptItem newItem, int flags)
	/*@modifies con @*/;

/** \ingroup popt
 * Read configuration file.
 * @param con		context
 * @param fn		file name to read
 * @return		0 on success, POPT_ERROR_ERRNO on failure
 */
int poptReadConfigFile(poptContext con, const char * fn)
	/*@globals fileSystem, internalState @*/
	/*@modifies con->execs, con->numExecs,
		fileSystem, internalState @*/;

/** \ingroup popt
 * Read default configuration from /etc/popt and $HOME/.popt.
 * @param con		context
 * @param useEnv	(unused)
 * @return		0 on success, POPT_ERROR_ERRNO on failure
 */
int poptReadDefaultConfig(poptContext con, /*@unused@*/ int useEnv)
	/*@globals fileSystem, internalState @*/
	/*@modifies con->execs, con->numExecs,
		fileSystem, internalState @*/;

/** \ingroup popt
 * Duplicate an argument array.
 * @note: The argument array is malloc'd as a single area, so only argv must
 * be free'd.
 *
 * @param argc		no. of arguments
 * @param argv		argument array
 * @retval argcPtr	address of returned no. of arguments
 * @retval argvPtr	address of returned argument array
 * @return		0 on success, POPT_ERROR_NOARG on failure
 */
int poptDupArgv(int argc, /*@null@*/ const char **argv,
		/*@null@*/ /*@out@*/ int * argcPtr,
		/*@null@*/ /*@out@*/ const char *** argvPtr)
	/*@modifies *argcPtr, *argvPtr @*/;

/** \ingroup popt
 * Parse a string into an argument array.
 * The parse allows ', ", and \ quoting, but ' is treated the same as " and
 * both may include \ quotes.
 * @note: The argument array is malloc'd as a single area, so only argv must
 * be free'd.
 *
 * @param s		string to parse
 * @retval argcPtr	address of returned no. of arguments
 * @retval argvPtr	address of returned argument array
 */
int poptParseArgvString(const char * s,
		/*@out@*/ int * argcPtr, /*@out@*/ const char *** argvPtr)
	/*@modifies *argcPtr, *argvPtr @*/;

/** \ingroup popt
 * Parses an input configuration file and returns an string that is a
 * command line.  For use with popt.  You must free the return value when done.
 *
 * Given the file:
\verbatim
# this line is ignored
    #   this one too
aaa
  bbb
    ccc
bla=bla

this_is   =   fdsafdas
     bad_line=
  reall bad line
  reall bad line  = again
5555=   55555
  test = with lots of spaces
\endverbatim
*
* The result is:
\verbatim
--aaa --bbb --ccc --bla="bla" --this_is="fdsafdas" --5555="55555" --test="with lots of spaces"
\endverbatim
*
* Passing this to poptParseArgvString() yields an argv of:
\verbatim
'--aaa'
'--bbb'
'--ccc'
'--bla=bla'
'--this_is=fdsafdas'
'--5555=55555'
'--test=with lots of spaces'
\endverbatim
 *
 * @bug NULL is returned if file line is too long.
 * @bug Silently ignores invalid lines.
 *
 * @param fp		file handle to read
 * @param *argstrp	return string of options (malloc'd)
 * @param flags		unused
 * @return		0 on success
 * @see			poptParseArgvString
 */
/*@-fcnuse@*/
int poptConfigFileToString(FILE *fp, /*@out@*/ char ** argstrp, int flags)
	/*@globals fileSystem @*/
	/*@modifies *fp, *argstrp, fileSystem @*/;
/*@=fcnuse@*/

/** \ingroup popt
 * Return formatted error string for popt failure.
 * @param error		popt error
 * @return		error string
 */
/*@observer@*/ const char* poptStrerror(const int error)
	/*@*/;

/** \ingroup popt
 * Limit search for executables.
 * @param con		context
 * @param path		single path to search for executables
 * @param allowAbsolute	absolute paths only?
 */
void poptSetExecPath(poptContext con, const char * path, int allowAbsolute)
	/*@modifies con @*/;

/** \ingroup popt
 * Print detailed description of options.
 * @param con		context
 * @param fp		ouput file handle
 * @param flags		(unused)
 */
void poptPrintHelp(poptContext con, FILE * fp, /*@unused@*/ int flags)
	/*@globals fileSystem @*/
	/*@modifies *fp, fileSystem @*/;

/** \ingroup popt
 * Print terse description of options.
 * @param con		context
 * @param fp		ouput file handle
 * @param flags		(unused)
 */
void poptPrintUsage(poptContext con, FILE * fp, /*@unused@*/ int flags)
	/*@globals fileSystem @*/
	/*@modifies *fp, fileSystem @*/;

/** \ingroup popt
 * Provide text to replace default "[OPTION...]" in help/usage output.
 * @param con		context
 * @param text		replacement text
 */
/*@-fcnuse@*/
void poptSetOtherOptionHelp(poptContext con, const char * text)
	/*@modifies con @*/;
/*@=fcnuse@*/

/** \ingroup popt
 * Return argv[0] from context.
 * @param con		context
 * @return		argv[0]
 */
/*@-fcnuse@*/
/*@observer@*/ const char * poptGetInvocationName(poptContext con)
	/*@*/;
/*@=fcnuse@*/

/** \ingroup popt
 * Shuffle argv pointers to remove stripped args, returns new argc.
 * @param con		context
 * @param argc		no. of args
 * @param argv		arg vector
 * @return		new argc
 */
/*@-fcnuse@*/
int poptStrippedArgv(poptContext con, int argc, char ** argv)
	/*@modifies *argv @*/;
/*@=fcnuse@*/

/**
 * Save a long, performing logical operation with value.
 * @warning Alignment check may be too strict on certain platorms.
 * @param arg		integer pointer, aligned on int boundary.
 * @param argInfo	logical operation (see POPT_ARGFLAG_*)
 * @param aLong		value to use
 * @return		0 on success, POPT_ERROR_NULLARG/POPT_ERROR_BADOPERATION
 */
/*@-incondefs@*/
/*@unused@*/
int poptSaveLong(/*@null@*/ long * arg, int argInfo, long aLong)
	/*@modifies *arg @*/
	/*@requires maxSet(arg) >= 0 /\ maxRead(arg) == 0 @*/;
/*@=incondefs@*/

/**
 * Save an integer, performing logical operation with value.
 * @warning Alignment check may be too strict on certain platorms.
 * @param arg		integer pointer, aligned on int boundary.
 * @param argInfo	logical operation (see POPT_ARGFLAG_*)
 * @param aLong		value to use
 * @return		0 on success, POPT_ERROR_NULLARG/POPT_ERROR_BADOPERATION
 */
/*@-incondefs@*/
/*@unused@*/
int poptSaveInt(/*@null@*/ int * arg, int argInfo, long aLong)
	/*@modifies *arg @*/
	/*@requires maxSet(arg) >= 0 /\ maxRead(arg) == 0 @*/;
/*@=incondefs@*/

/*@=type@*/
#ifdef  __cplusplus
}
#endif

#endif
