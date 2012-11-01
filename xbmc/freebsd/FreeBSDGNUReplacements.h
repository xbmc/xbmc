#ifndef __FREEBSDGNUREPLACEMENTS_H__
#define __FREEBSDGNUREPLACEMENTS_H__

#include <sys/param.h>
#include <string.h>
#include <stdio.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#if __FreeBSD_version < 800067

size_t strnlen(const char *s, size_t n);
ssize_t getdelim(char **lineptr, size_t *n, int delimiter, FILE *stream);

#endif

int strverscmp(const char *s1, const char *s2);

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif
