/*
 * Convert a string between UTF-8 and unicode.
 */

#ifndef __UTF8_H
#define __UTF8_H

#ifdef	__cplusplus
extern "C"
{
#endif

unsigned char *make_utf8_string(const wchar_t *unicode);
wchar_t *make_unicode_string(const unsigned char *utf8);

#ifdef	__cplusplus
}
#endif

#endif /* __UTF8_H */
