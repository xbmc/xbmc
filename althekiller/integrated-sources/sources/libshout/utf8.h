
/*
 * Convert a string between UTF-8 and the locale's charset.
 * Invalid bytes are replaced by '#', and characters that are
 * not available in the target encoding are replaced by '?'.
 *
 * If the locale's charset is not set explicitly then it is
 * obtained using nl_langinfo(CODESET), where available, the
 * environment variable CHARSET, or assumed to be US-ASCII.
 *
 * Return value of conversion functions:
 *
 *  -1 : memory allocation failed
 *   0 : data was converted exactly
 *   1 : valid data was converted approximately (using '?')
 *   2 : input was invalid (but still converted, using '#')
 *   3 : unknown encoding (but still converted, using '?')
 */

#ifndef __UTF8_H
#define __UTF8_H

#ifdef	__cplusplus
extern "C" {
#endif

void convert_set_charset(const char *charset);

int utf8_encode(const char *from, char **to);
int utf8_decode(const char *from, char **to);

#ifdef	__cplusplus
}
#endif

#endif /* __UTF8_H */
