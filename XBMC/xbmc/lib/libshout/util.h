#ifndef __UTIL_H__
#define __UTIL_H__

#include "types.h"


extern char		*escape_string_alloc(const char *str);
extern char		*left_str(char *str, int len);
extern char		*strip_last_word(char *str);
extern int			word_count(char *str);
extern char		*subnstr_until(const char *str, char *until, char *newstr, int maxlen);
extern char		*strip_invalid_chars(char *str);
extern char		*format_byte_size(char *str, long size);
extern char		*add_trailing_slash(char *str);
extern void		trim(char *str);
extern void 		null_printf(char *s, ...);


#endif //__UTIL_H__
