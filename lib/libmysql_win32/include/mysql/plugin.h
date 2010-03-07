/* Copyright (C) 2005 MySQL AB

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA */

#ifndef _my_plugin_h
#define _my_plugin_h


/*
  On Windows, exports from DLL need to be declared
*/
#if (defined(_WIN32) && defined(MYSQL_DYNAMIC_PLUGIN))
#define MYSQL_PLUGIN_EXPORT extern "C" __declspec(dllexport)
#else
#define MYSQL_PLUGIN_EXPORT
#endif

#ifdef __cplusplus
class THD;
class Item;
#define MYSQL_THD THD*
#else
#define MYSQL_THD void*
#endif

#ifndef _m_string_h
/* This definition must match the one given in m_string.h */
struct st_mysql_lex_string
{
  char *str;
  unsigned int length;
};
#endif /* _m_string_h */
typedef struct st_mysql_lex_string MYSQL_LEX_STRING;

#define MYSQL_XIDDATASIZE 128
/**
  struct st_mysql_xid is binary compatible with the XID structure as
  in the X/Open CAE Specification, Distributed Transaction Processing:
  The XA Specification, X/Open Company Ltd., 1991.
  http://www.opengroup.org/bookstore/catalog/c193.htm

  @see XID in sql/handler.h
*/
struct st_mysql_xid {
  long formatID;
  long gtrid_length;
  long bqual_length;
  char data[MYSQL_XIDDATASIZE];  /* Not \0-terminated */
};
typedef struct st_mysql_xid MYSQL_XID;

/*************************************************************************
  Plugin API. Common for all plugin types.
*/

#define MYSQL_PLUGIN_INTERFACE_VERSION 0x0100

/*
  The allowable types of plugins
*/
#define MYSQL_UDF_PLUGIN             0  /* User-defined function        */
#define MYSQL_STORAGE_ENGINE_PLUGIN  1  /* Storage Engine               */
#define MYSQL_FTPARSER_PLUGIN        2  /* Full-text parser plugin      */
#define MYSQL_DAEMON_PLUGIN          3  /* The daemon/raw plugin type */
#define MYSQL_INFORMATION_SCHEMA_PLUGIN  4  /* The I_S plugin type */
#define MYSQL_MAX_PLUGIN_TYPE_NUM    5  /* The number of plugin types   */

/* We use the following strings to define licenses for plugins */
#define PLUGIN_LICENSE_PROPRIETARY 0
#define PLUGIN_LICENSE_GPL 1
#define PLUGIN_LICENSE_BSD 2

#define PLUGIN_LICENSE_PROPRIETARY_STRING "PROPRIETARY"
#define PLUGIN_LICENSE_GPL_STRING "GPL"
#define PLUGIN_LICENSE_BSD_STRING "BSD"

/*
  Macros for beginning and ending plugin declarations.  Between
  mysql_declare_plugin and mysql_declare_plugin_end there should
  be a st_mysql_plugin struct for each plugin to be declared.
*/


#ifndef MYSQL_DYNAMIC_PLUGIN
#define __MYSQL_DECLARE_PLUGIN(NAME, VERSION, PSIZE, DECLS)                   \
int VERSION= MYSQL_PLUGIN_INTERFACE_VERSION;                                  \
int PSIZE= sizeof(struct st_mysql_plugin);                                    \
struct st_mysql_plugin DECLS[]= {
#else
#define __MYSQL_DECLARE_PLUGIN(NAME, VERSION, PSIZE, DECLS)                   \
MYSQL_PLUGIN_EXPORT int _mysql_plugin_interface_version_= MYSQL_PLUGIN_INTERFACE_VERSION;         \
MYSQL_PLUGIN_EXPORT int _mysql_sizeof_struct_st_plugin_= sizeof(struct st_mysql_plugin);          \
MYSQL_PLUGIN_EXPORT struct st_mysql_plugin _mysql_plugin_declarations_[]= {
#endif

#define mysql_declare_plugin(NAME) \
__MYSQL_DECLARE_PLUGIN(NAME, \
                 builtin_ ## NAME ## _plugin_interface_version, \
                 builtin_ ## NAME ## _sizeof_struct_st_plugin, \
                 builtin_ ## NAME ## _plugin)

#define mysql_declare_plugin_end ,{0,0,0,0,0,0,0,0,0,0,0,0}}

/*
  declarations for SHOW STATUS support in plugins
*/
enum enum_mysql_show_type
{
  SHOW_UNDEF, SHOW_BOOL, SHOW_INT, SHOW_LONG,
  SHOW_LONGLONG, SHOW_CHAR, SHOW_CHAR_PTR,
  SHOW_ARRAY, SHOW_FUNC, SHOW_DOUBLE
};

struct st_mysql_show_var {
  const char *name;
  char *value;
  enum enum_mysql_show_type type;
};

#define SHOW_VAR_FUNC_BUFF_SIZE 1024
typedef int (*mysql_show_var_func)(MYSQL_THD, struct st_mysql_show_var*, char *);


/*
  declarations for server variables and command line options
*/


#define PLUGIN_VAR_BOOL         0x0001
#define PLUGIN_VAR_INT          0x0002
#define PLUGIN_VAR_LONG         0x0003
#define PLUGIN_VAR_LONGLONG     0x0004
#define PLUGIN_VAR_STR          0x0005
#define PLUGIN_VAR_ENUM         0x0006
#define PLUGIN_VAR_SET          0x0007
#define PLUGIN_VAR_UNSIGNED     0x0080
#define PLUGIN_VAR_THDLOCAL     0x0100 /* Variable is per-connection */
#define PLUGIN_VAR_READONLY     0x0200 /* Server variable is read only */
#define PLUGIN_VAR_NOSYSVAR     0x0400 /* Not a server variable */
#define PLUGIN_VAR_NOCMDOPT     0x0800 /* Not a command line option */
#define PLUGIN_VAR_NOCMDARG     0x1000 /* No argument for cmd line */
#define PLUGIN_VAR_RQCMDARG     0x0000 /* Argument required for cmd line */
#define PLUGIN_VAR_OPCMDARG     0x2000 /* Argument optional for cmd line */
#define PLUGIN_VAR_MEMALLOC     0x8000 /* String needs memory allocated */

struct st_mysql_sys_var;
struct st_mysql_value;

/*
  SYNOPSIS
    (*mysql_var_check_func)()
      thd               thread handle
      var               dynamic variable being altered
      save              pointer to temporary storage
      value             user provided value
  RETURN
    0   user provided value is OK and the update func may be called.
    any other value indicates error.
  
  This function should parse the user provided value and store in the
  provided temporary storage any data as required by the update func.
  There is sufficient space in the temporary storage to store a double.
  Note that the update func may not be called if any other error occurs
  so any memory allocated should be thread-local so that it may be freed
  automatically at the end of the statement.
*/

typedef int (*mysql_var_check_func)(MYSQL_THD thd,
                                    struct st_mysql_sys_var *var,
                                    void *save, struct st_mysql_value *value);

/*
  SYNOPSIS
    (*mysql_var_update_func)()
      thd               thread handle
      var               dynamic variable being altered
      var_ptr           pointer to dynamic variable
      save              pointer to temporary storage
   RETURN
     NONE
   
   This function should use the validated value stored in the temporary store
   and persist it in the provided pointer to the dynamic variable.
   For example, strings may require memory to be allocated.
*/
typedef void (*mysql_var_update_func)(MYSQL_THD thd,
                                      struct st_mysql_sys_var *var,
                                      void *var_ptr, const void *save);


/* the following declarations are for internal use only */


#define PLUGIN_VAR_MASK \
        (PLUGIN_VAR_READONLY | PLUGIN_VAR_NOSYSVAR | \
         PLUGIN_VAR_NOCMDOPT | PLUGIN_VAR_NOCMDARG | \
         PLUGIN_VAR_OPCMDARG | PLUGIN_VAR_RQCMDARG | PLUGIN_VAR_MEMALLOC)

#define MYSQL_PLUGIN_VAR_HEADER \
  int flags;                    \
  const char *name;             \
  const char *comment;          \
  mysql_var_check_func check;   \
  mysql_var_update_func update

#define MYSQL_SYSVAR_NAME(name) mysql_sysvar_ ## name
#define MYSQL_SYSVAR(name) \
  ((struct st_mysql_sys_var *)&(MYSQL_SYSVAR_NAME(name)))

/*
  for global variables, the value pointer is the first
  element after the header, the default value is the second.
  for thread variables, the value offset is the first
  element after the header, the default value is the second.
*/
   

#define DECLARE_MYSQL_SYSVAR_BASIC(name, type) struct { \
  MYSQL_PLUGIN_VAR_HEADER;      \
  type *value;                  \
  const type def_val;           \
} MYSQL_SYSVAR_NAME(name)

#define DECLARE_MYSQL_SYSVAR_SIMPLE(name, type) struct { \
  MYSQL_PLUGIN_VAR_HEADER;      \
  type *value; type def_val;    \
  type min_val; type max_val;   \
  type blk_sz;                  \
} MYSQL_SYSVAR_NAME(name)

#define DECLARE_MYSQL_SYSVAR_TYPELIB(name, type) struct { \
  MYSQL_PLUGIN_VAR_HEADER;      \
  type *value; type def_val;    \
  TYPELIB *typelib;             \
} MYSQL_SYSVAR_NAME(name)

#define DECLARE_THDVAR_FUNC(type) \
  type *(*resolve)(MYSQL_THD thd, int offset)

#define DECLARE_MYSQL_THDVAR_BASIC(name, type) struct { \
  MYSQL_PLUGIN_VAR_HEADER;      \
  int offset;                   \
  const type def_val;           \
  DECLARE_THDVAR_FUNC(type);    \
} MYSQL_SYSVAR_NAME(name)

#define DECLARE_MYSQL_THDVAR_SIMPLE(name, type) struct { \
  MYSQL_PLUGIN_VAR_HEADER;      \
  int offset;                   \
  type def_val; type min_val;   \
  type max_val; type blk_sz;    \
  DECLARE_THDVAR_FUNC(type);    \
} MYSQL_SYSVAR_NAME(name)

#define DECLARE_MYSQL_THDVAR_TYPELIB(name, type) struct { \
  MYSQL_PLUGIN_VAR_HEADER;      \
  int offset;                   \
  type def_val;                 \
  DECLARE_THDVAR_FUNC(type);    \
  TYPELIB *typelib;             \
} MYSQL_SYSVAR_NAME(name)


/*
  the following declarations are for use by plugin implementors
*/

#define MYSQL_SYSVAR_BOOL(name, varname, opt, comment, check, update, def) \
DECLARE_MYSQL_SYSVAR_BASIC(name, char) = { \
  PLUGIN_VAR_BOOL | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, &varname, def}

#define MYSQL_SYSVAR_STR(name, varname, opt, comment, check, update, def) \
DECLARE_MYSQL_SYSVAR_BASIC(name, char *) = { \
  PLUGIN_VAR_STR | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, &varname, def}

#define MYSQL_SYSVAR_INT(name, varname, opt, comment, check, update, def, min, max, blk) \
DECLARE_MYSQL_SYSVAR_SIMPLE(name, int) = { \
  PLUGIN_VAR_INT | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, &varname, def, min, max, blk }

#define MYSQL_SYSVAR_UINT(name, varname, opt, comment, check, update, def, min, max, blk) \
DECLARE_MYSQL_SYSVAR_SIMPLE(name, unsigned int) = { \
  PLUGIN_VAR_INT | PLUGIN_VAR_UNSIGNED | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, &varname, def, min, max, blk }

#define MYSQL_SYSVAR_LONG(name, varname, opt, comment, check, update, def, min, max, blk) \
DECLARE_MYSQL_SYSVAR_SIMPLE(name, long) = { \
  PLUGIN_VAR_LONG | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, &varname, def, min, max, blk }

#define MYSQL_SYSVAR_ULONG(name, varname, opt, comment, check, update, def, min, max, blk) \
DECLARE_MYSQL_SYSVAR_SIMPLE(name, unsigned long) = { \
  PLUGIN_VAR_LONG | PLUGIN_VAR_UNSIGNED | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, &varname, def, min, max, blk }

#define MYSQL_SYSVAR_LONGLONG(name, varname, opt, comment, check, update, def, min, max, blk) \
DECLARE_MYSQL_SYSVAR_SIMPLE(name, long long) = { \
  PLUGIN_VAR_LONGLONG | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, &varname, def, min, max, blk }

#define MYSQL_SYSVAR_ULONGLONG(name, varname, opt, comment, check, update, def, min, max, blk) \
DECLARE_MYSQL_SYSVAR_SIMPLE(name, unsigned long long) = { \
  PLUGIN_VAR_LONGLONG | PLUGIN_VAR_UNSIGNED | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, &varname, def, min, max, blk }

#define MYSQL_SYSVAR_ENUM(name, varname, opt, comment, check, update, def, typelib) \
DECLARE_MYSQL_SYSVAR_TYPELIB(name, unsigned long) = { \
  PLUGIN_VAR_ENUM | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, &varname, def, typelib }

#define MYSQL_SYSVAR_SET(name, varname, opt, comment, check, update, def, typelib) \
DECLARE_MYSQL_SYSVAR_TYPELIB(name, unsigned long long) = { \
  PLUGIN_VAR_SET | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, &varname, def, typelib }

#define MYSQL_THDVAR_BOOL(name, opt, comment, check, update, def) \
DECLARE_MYSQL_THDVAR_BASIC(name, char) = { \
  PLUGIN_VAR_BOOL | PLUGIN_VAR_THDLOCAL | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, -1, def, NULL}

#define MYSQL_THDVAR_STR(name, opt, comment, check, update, def) \
DECLARE_MYSQL_THDVAR_BASIC(name, char *) = { \
  PLUGIN_VAR_STR | PLUGIN_VAR_THDLOCAL | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, -1, def, NULL}

#define MYSQL_THDVAR_INT(name, opt, comment, check, update, def, min, max, blk) \
DECLARE_MYSQL_THDVAR_SIMPLE(name, int) = { \
  PLUGIN_VAR_INT | PLUGIN_VAR_THDLOCAL | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, -1, def, min, max, blk, NULL }

#define MYSQL_THDVAR_UINT(name, opt, comment, check, update, def, min, max, blk) \
DECLARE_MYSQL_THDVAR_SIMPLE(name, unsigned int) = { \
  PLUGIN_VAR_INT | PLUGIN_VAR_THDLOCAL | PLUGIN_VAR_UNSIGNED | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, -1, def, min, max, blk, NULL }

#define MYSQL_THDVAR_LONG(name, opt, comment, check, update, def, min, max, blk) \
DECLARE_MYSQL_THDVAR_SIMPLE(name, long) = { \
  PLUGIN_VAR_LONG | PLUGIN_VAR_THDLOCAL | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, -1, def, min, max, blk, NULL }

#define MYSQL_THDVAR_ULONG(name, opt, comment, check, update, def, min, max, blk) \
DECLARE_MYSQL_THDVAR_SIMPLE(name, unsigned long) = { \
  PLUGIN_VAR_LONG | PLUGIN_VAR_THDLOCAL | PLUGIN_VAR_UNSIGNED | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, -1, def, min, max, blk, NULL }

#define MYSQL_THDVAR_LONGLONG(name, opt, comment, check, update, def, min, max, blk) \
DECLARE_MYSQL_THDVAR_SIMPLE(name, long long) = { \
  PLUGIN_VAR_LONGLONG | PLUGIN_VAR_THDLOCAL | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, -1, def, min, max, blk, NULL }

#define MYSQL_THDVAR_ULONGLONG(name, opt, comment, check, update, def, min, max, blk) \
DECLARE_MYSQL_THDVAR_SIMPLE(name, unsigned long long) = { \
  PLUGIN_VAR_LONGLONG | PLUGIN_VAR_THDLOCAL | PLUGIN_VAR_UNSIGNED | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, -1, def, min, max, blk, NULL }

#define MYSQL_THDVAR_ENUM(name, opt, comment, check, update, def, typelib) \
DECLARE_MYSQL_THDVAR_TYPELIB(name, unsigned long) = { \
  PLUGIN_VAR_ENUM | PLUGIN_VAR_THDLOCAL | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, -1, def, NULL, typelib }

#define MYSQL_THDVAR_SET(name, opt, comment, check, update, def, typelib) \
DECLARE_MYSQL_THDVAR_TYPELIB(name, unsigned long long) = { \
  PLUGIN_VAR_SET | PLUGIN_VAR_THDLOCAL | ((opt) & PLUGIN_VAR_MASK), \
  #name, comment, check, update, -1, def, NULL, typelib }

/* accessor macros */

#define SYSVAR(name) \
  (*(MYSQL_SYSVAR_NAME(name).value))

/* when thd == null, result points to global value */
#define THDVAR(thd, name) \
  (*(MYSQL_SYSVAR_NAME(name).resolve(thd, MYSQL_SYSVAR_NAME(name).offset)))


/*
  Plugin description structure.
*/

struct st_mysql_plugin
{
  int type;             /* the plugin type (a MYSQL_XXX_PLUGIN value)   */
  void *info;           /* pointer to type-specific plugin descriptor   */
  const char *name;     /* plugin name                                  */
  const char *author;   /* plugin author (for SHOW PLUGINS)             */
  const char *descr;    /* general descriptive text (for SHOW PLUGINS ) */
  int license;          /* the plugin license (PLUGIN_LICENSE_XXX)      */
  int (*init)(void *);  /* the function to invoke when plugin is loaded */
  int (*deinit)(void *);/* the function to invoke when plugin is unloaded */
  unsigned int version; /* plugin version (for SHOW PLUGINS)            */
  struct st_mysql_show_var *status_vars;
  struct st_mysql_sys_var **system_vars;
  void * __reserved1;   /* reserved for dependency checking             */
};

/*************************************************************************
  API for Full-text parser plugin. (MYSQL_FTPARSER_PLUGIN)
*/

#define MYSQL_FTPARSER_INTERFACE_VERSION 0x0100

/* Parsing modes. Set in  MYSQL_FTPARSER_PARAM::mode */
enum enum_ftparser_mode
{
/*
  Fast and simple mode.  This mode is used for indexing, and natural
  language queries.

  The parser is expected to return only those words that go into the
  index. Stopwords or too short/long words should not be returned. The
  'boolean_info' argument of mysql_add_word() does not have to be set.
*/
  MYSQL_FTPARSER_SIMPLE_MODE= 0,

/*
  Parse with stopwords mode.  This mode is used in boolean searches for
  "phrase matching."

  The parser is not allowed to ignore words in this mode.  Every word
  should be returned, including stopwords and words that are too short
  or long.  The 'boolean_info' argument of mysql_add_word() does not
  have to be set.
*/
  MYSQL_FTPARSER_WITH_STOPWORDS= 1,

/*
  Parse in boolean mode.  This mode is used to parse a boolean query string.

  The parser should provide a valid MYSQL_FTPARSER_BOOLEAN_INFO
  structure in the 'boolean_info' argument to mysql_add_word().
  Usually that means that the parser should recognize boolean operators
  in the parsing stream and set appropriate fields in
  MYSQL_FTPARSER_BOOLEAN_INFO structure accordingly.  As for
  MYSQL_FTPARSER_WITH_STOPWORDS mode, no word should be ignored.
  Instead, use FT_TOKEN_STOPWORD for the token type of such a word.
*/
  MYSQL_FTPARSER_FULL_BOOLEAN_INFO= 2
};

/*
  Token types for boolean mode searching (used for the type member of
  MYSQL_FTPARSER_BOOLEAN_INFO struct)

  FT_TOKEN_EOF: End of data.
  FT_TOKEN_WORD: Regular word.
  FT_TOKEN_LEFT_PAREN: Left parenthesis (start of group/sub-expression).
  FT_TOKEN_RIGHT_PAREN: Right parenthesis (end of group/sub-expression).
  FT_TOKEN_STOPWORD: Stopword.
*/

enum enum_ft_token_type
{
  FT_TOKEN_EOF= 0,
  FT_TOKEN_WORD= 1,
  FT_TOKEN_LEFT_PAREN= 2,
  FT_TOKEN_RIGHT_PAREN= 3,
  FT_TOKEN_STOPWORD= 4
};

/*
  This structure is used in boolean search mode only. It conveys
  boolean-mode metadata to the MySQL search engine for every word in
  the search query. A valid instance of this structure must be filled
  in by the plugin parser and passed as an argument in the call to
  mysql_add_word (the callback function in the MYSQL_FTPARSER_PARAM
  structure) when a query is parsed in boolean mode.

  type: The token type.  Should be one of the enum_ft_token_type values.

  yesno: Whether the word must be present for a match to occur:
    >0 Must be present
    <0 Must not be present
    0  Neither; the word is optional but its presence increases the relevance
  With the default settings of the ft_boolean_syntax system variable,
  >0 corresponds to the '+' operator, <0 corrresponds to the '-' operator,
  and 0 means neither operator was used.

  weight_adjust: A weighting factor that determines how much a match
  for the word counts.  Positive values increase, negative - decrease the
  relative word's importance in the query.

  wasign: The sign of the word's weight in the query. If it's non-negative
  the match for the word will increase document relevance, if it's
  negative - decrease (the word becomes a "noise word", the less of it the
  better).

  trunc: Corresponds to the '*' operator in the default setting of the
  ft_boolean_syntax system variable.
*/

typedef struct st_mysql_ftparser_boolean_info
{
  enum enum_ft_token_type type;
  int yesno;
  int weight_adjust;
  char wasign;
  char trunc;
  /* These are parser state and must be removed. */
  char prev;
  char *quot;
} MYSQL_FTPARSER_BOOLEAN_INFO;

/*
  The following flag means that buffer with a string (document, word)
  may be overwritten by the caller before the end of the parsing (that is
  before st_mysql_ftparser::deinit() call). If one needs the string
  to survive between two successive calls of the parsing function, she
  needs to save a copy of it. The flag may be set by MySQL before calling
  st_mysql_ftparser::parse(), or it may be set by a plugin before calling
  st_mysql_ftparser_param::mysql_parse() or
  st_mysql_ftparser_param::mysql_add_word().
*/
#define MYSQL_FTFLAGS_NEED_COPY 1

/*
  An argument of the full-text parser plugin. This structure is
  filled in by MySQL server and passed to the parsing function of the
  plugin as an in/out parameter.

  mysql_parse: A pointer to the built-in parser implementation of the
  server. It's set by the server and can be used by the parser plugin
  to invoke the MySQL default parser.  If plugin's role is to extract
  textual data from .doc, .pdf or .xml content, it might extract
  plaintext from the content, and then pass the text to the default
  MySQL parser to be parsed.

  mysql_add_word: A server callback to add a new word.  When parsing
  a document, the server sets this to point at a function that adds
  the word to MySQL full-text index.  When parsing a search query,
  this function will add the new word to the list of words to search
  for.  The boolean_info argument can be NULL for all cases except
  when mode is MYSQL_FTPARSER_FULL_BOOLEAN_INFO.

  ftparser_state: A generic pointer. The plugin can set it to point
  to information to be used internally for its own purposes.

  mysql_ftparam: This is set by the server.  It is used by MySQL functions
  called via mysql_parse() and mysql_add_word() callback.  The plugin
  should not modify it.

  cs: Information about the character set of the document or query string.

  doc: A pointer to the document or query string to be parsed.

  length: Length of the document or query string, in bytes.

  flags: See MYSQL_FTFLAGS_* constants above.

  mode: The parsing mode.  With boolean operators, with stopwords, or
  nothing.  See  enum_ftparser_mode above.
*/

typedef struct st_mysql_ftparser_param
{
  int (*mysql_parse)(struct st_mysql_ftparser_param *,
                     char *doc, int doc_len);
  int (*mysql_add_word)(struct st_mysql_ftparser_param *,
                        char *word, int word_len,
                        MYSQL_FTPARSER_BOOLEAN_INFO *boolean_info);
  void *ftparser_state;
  void *mysql_ftparam;
  struct charset_info_st *cs;
  char *doc;
  int length;
  int flags;
  enum enum_ftparser_mode mode;
} MYSQL_FTPARSER_PARAM;

/*
  Full-text parser descriptor.

  interface_version is, e.g., MYSQL_FTPARSER_INTERFACE_VERSION.
  The parsing, initialization, and deinitialization functions are
  invoked per SQL statement for which the parser is used.
*/

struct st_mysql_ftparser
{
  int interface_version;
  int (*parse)(MYSQL_FTPARSER_PARAM *param);
  int (*init)(MYSQL_FTPARSER_PARAM *param);
  int (*deinit)(MYSQL_FTPARSER_PARAM *param);
};

/*************************************************************************
  API for Storage Engine plugin. (MYSQL_DAEMON_PLUGIN)
*/

/* handlertons of different MySQL releases are incompatible */
#define MYSQL_DAEMON_INTERFACE_VERSION (MYSQL_VERSION_ID << 8)

/*************************************************************************
  API for I_S plugin. (MYSQL_INFORMATION_SCHEMA_PLUGIN)
*/

/* handlertons of different MySQL releases are incompatible */
#define MYSQL_INFORMATION_SCHEMA_INTERFACE_VERSION (MYSQL_VERSION_ID << 8)

/*************************************************************************
  API for Storage Engine plugin. (MYSQL_STORAGE_ENGINE_PLUGIN)
*/

/* handlertons of different MySQL releases are incompatible */
#define MYSQL_HANDLERTON_INTERFACE_VERSION (MYSQL_VERSION_ID << 8)

/*
  The real API is in the sql/handler.h
  Here we define only the descriptor structure, that is referred from
  st_mysql_plugin.
*/

struct st_mysql_storage_engine
{
  int interface_version;
};

struct handlerton;

/*
  Here we define only the descriptor structure, that is referred from
  st_mysql_plugin.
*/

struct st_mysql_daemon
{
  int interface_version;
};

/*
  Here we define only the descriptor structure, that is referred from
  st_mysql_plugin.
*/

struct st_mysql_information_schema
{
  int interface_version;
};


/*
  st_mysql_value struct for reading values from mysqld.
  Used by server variables framework to parse user-provided values.
  Will be used for arguments when implementing UDFs.

  Note that val_str() returns a string in temporary memory
  that will be freed at the end of statement. Copy the string
  if you need it to persist.
*/

#define MYSQL_VALUE_TYPE_STRING 0
#define MYSQL_VALUE_TYPE_REAL   1
#define MYSQL_VALUE_TYPE_INT    2

struct st_mysql_value
{
  int (*value_type)(struct st_mysql_value *);
  const char *(*val_str)(struct st_mysql_value *, char *buffer, int *length);
  int (*val_real)(struct st_mysql_value *, double *realbuf);
  int (*val_int)(struct st_mysql_value *, long long *intbuf);
};


/*************************************************************************
  Miscellaneous functions for plugin implementors
*/

#ifdef __cplusplus
extern "C" {
#endif

int thd_in_lock_tables(const MYSQL_THD thd);
int thd_tablespace_op(const MYSQL_THD thd);
long long thd_test_options(const MYSQL_THD thd, long long test_options);
int thd_sql_command(const MYSQL_THD thd);
const char *thd_proc_info(MYSQL_THD thd, const char *info);
void **thd_ha_data(const MYSQL_THD thd, const struct handlerton *hton);
int thd_tx_isolation(const MYSQL_THD thd);
char *thd_security_context(MYSQL_THD thd, char *buffer, unsigned int length,
                           unsigned int max_query_len);
/* Increments the row counter, see THD::row_count */
void thd_inc_row_count(MYSQL_THD thd);

/**
  Create a temporary file.

  @details
  The temporary file is created in a location specified by the mysql
  server configuration (--tmpdir option).  The caller does not need to
  delete the file, it will be deleted automatically.

  @param prefix  prefix for temporary file name
  @retval -1    error
  @retval >= 0  a file handle that can be passed to dup or my_close
*/
int mysql_tmpfile(const char *prefix);

/**
  Check the killed state of a connection

  @details
  In MySQL support for the KILL statement is cooperative. The KILL
  statement only sets a "killed" flag. This function returns the value
  of that flag.  A thread should check it often, especially inside
  time-consuming loops, and gracefully abort the operation if it is
  non-zero.

  @param thd  user thread connection handle
  @retval 0  the connection is active
  @retval 1  the connection has been killed
*/
int thd_killed(const MYSQL_THD thd);


/**
  Return the thread id of a user thread

  @param thd  user thread connection handle
  @return  thread id
*/
unsigned long thd_get_thread_id(const MYSQL_THD thd);


/**
  Allocate memory in the connection's local memory pool

  @details
  When properly used in place of @c my_malloc(), this can significantly
  improve concurrency. Don't use this or related functions to allocate
  large chunks of memory. Use for temporary storage only. The memory
  will be freed automatically at the end of the statement; no explicit
  code is required to prevent memory leaks.

  @see alloc_root()
*/
void *thd_alloc(MYSQL_THD thd, unsigned int size);
/**
  @see thd_alloc()
*/
void *thd_calloc(MYSQL_THD thd, unsigned int size);
/**
  @see thd_alloc()
*/
char *thd_strdup(MYSQL_THD thd, const char *str);
/**
  @see thd_alloc()
*/
char *thd_strmake(MYSQL_THD thd, const char *str, unsigned int size);
/**
  @see thd_alloc()
*/
void *thd_memdup(MYSQL_THD thd, const void* str, unsigned int size);

/**
  Create a LEX_STRING in this connection's local memory pool

  @param thd      user thread connection handle
  @param lex_str  pointer to LEX_STRING object to be initialized
  @param str      initializer to be copied into lex_str
  @param size     length of str, in bytes
  @param allocate_lex_string  flag: if TRUE, allocate new LEX_STRING object,
                              instead of using lex_str value
  @return  NULL on failure, or pointer to the LEX_STRING object

  @see thd_alloc()
*/
MYSQL_LEX_STRING *thd_make_lex_string(MYSQL_THD thd, MYSQL_LEX_STRING *lex_str,
                                      const char *str, unsigned int size,
                                      int allocate_lex_string);

/**
  Get the XID for this connection's transaction

  @param thd  user thread connection handle
  @param xid  location where identifier is stored
*/
void thd_get_xid(const MYSQL_THD thd, MYSQL_XID *xid);

/**
  Invalidate the query cache for a given table.

  @param thd         user thread connection handle
  @param key         databasename\\0tablename\\0
  @param key_length  length of key in bytes, including the NUL bytes
  @param using_trx   flag: TRUE if using transactions, FALSE otherwise
*/
void mysql_query_cache_invalidate4(MYSQL_THD thd,
                                   const char *key, unsigned int key_length,
                                   int using_trx);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
/**
  Provide a handler data getter to simplify coding
*/
inline
void *
thd_get_ha_data(const MYSQL_THD thd, const struct handlerton *hton)
{
  return *thd_ha_data(thd, hton);
}

/**
  Provide a handler data setter to simplify coding
*/
inline
void
thd_set_ha_data(const MYSQL_THD thd, const struct handlerton *hton,
                const void *ha_data)
{
  *thd_ha_data(thd, hton)= (void*) ha_data;
}
#endif

#endif

