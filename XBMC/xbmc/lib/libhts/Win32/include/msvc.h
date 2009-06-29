#ifdef WIN32

#define strtoll(p, e, b) _strtoi64(p, e, b)
#define snprintf _snprintf
#define strcasecmp stricmp
#define strncasecmp strnicmp
#define strdup _strdup

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#define S_IFREG  0100000
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)

#endif
