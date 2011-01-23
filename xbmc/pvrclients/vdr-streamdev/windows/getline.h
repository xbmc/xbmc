#ifndef _getline_h_
#define _getline_h_

#include <stdio.h>

#define GETLINE_NO_LIMIT -1

int getline(char **_lineptr, size_t *_n, FILE *_stream);
int getline_safe(char **_lineptr, size_t *_n, FILE *_stream, int limit);
int getstr(char **_lineptr, size_t *_n, FILE *_stream, int _terminator, int _offset, int limit);

#endif /* _getline_h_ */
