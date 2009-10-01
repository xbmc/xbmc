#ifndef MAC_UTILS_H
#define MAC_UTILS_H

#ifndef HAVE_WCSCASECMP
#include <wchar.h>

int mac_wcscasecmp(const wchar_t *s1, const wchar_t *s2);
int mac_wcsncasecmp(const wchar_t *s1, const wchar_t *s2, size_t n);

#endif

#endif
