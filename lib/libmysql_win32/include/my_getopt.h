/* Copyright (C) 2002-2004 MySQL AB

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

#ifndef _my_getopt_h
#define _my_getopt_h

C_MODE_START

#define GET_NO_ARG     1
#define GET_BOOL       2
#define GET_INT        3
#define GET_UINT       4
#define GET_LONG       5
#define GET_ULONG      6
#define GET_LL         7
#define GET_ULL        8
#define GET_STR        9
#define GET_STR_ALLOC 10
#define GET_DISABLED  11
#define GET_ENUM      12
#define GET_SET       13
#define GET_DOUBLE    14

#define GET_ASK_ADDR	 128
#define GET_TYPE_MASK	 127

enum get_opt_arg_type { NO_ARG, OPT_ARG, REQUIRED_ARG };

struct st_typelib;

struct my_option
{
  const char *name;                     /* Name of the option */
  int        id;                        /* unique id or short option */
  const char *comment;                  /* option comment, for autom. --help */
  uchar      **value;                   /* The variable value */
  uchar      **u_max_value;             /* The user def. max variable value */
  struct st_typelib *typelib;           /* Pointer to possible values */
  ulong     var_type;
  enum get_opt_arg_type arg_type;
  longlong   def_value;                 /* Default value */
  longlong   min_value;                 /* Min allowed value */
  longlong   max_value;                 /* Max allowed value */
  longlong   sub_size;                  /* Subtract this from given value */
  long       block_size;                /* Value should be a mult. of this */
  void       *app_type;                 /* To be used by an application */
};

typedef my_bool (* my_get_one_option) (int, const struct my_option *, char * );
typedef void (* my_error_reporter) (enum loglevel level, const char *format, ... );

extern char *disabled_my_option;
extern my_bool my_getopt_print_errors;
extern my_bool my_getopt_skip_unknown;
extern my_error_reporter my_getopt_error_reporter;

extern int handle_options (int *argc, char ***argv, 
			   const struct my_option *longopts, my_get_one_option);
extern void my_cleanup_options(const struct my_option *options);
extern void my_print_help(const struct my_option *options);
extern void my_print_variables(const struct my_option *options);
extern void my_getopt_register_get_addr(uchar ** (*func_addr)(const char *, uint,
                                        const struct my_option *, int *));

ulonglong getopt_ull_limit_value(ulonglong num, const struct my_option *optp,
                                 my_bool *fix);
longlong getopt_ll_limit_value(longlong, const struct my_option *,
                               my_bool *fix);
my_bool getopt_compare_strings(const char *s, const char *t, uint length);

C_MODE_END

#endif /* _my_getopt_h */

