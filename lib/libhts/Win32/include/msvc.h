#ifdef WIN32

#define strtoll(p, e, b) _strtoi64(p, e, b)
#define snprintf _snprintf
#define strcasecmp stricmp
#define strncasecmp strnicmp
#define strdup _strdup
#if _MSC_VER < 1500
#define vsnprintf _vsnprintf
#endif

static char * strndup(const char* str, size_t len)
{
  size_t i = 0;
  char*  p = (char*)str;
  while(*p != 0 && i < len)
  {
    p++;
    i++;
  }
  p = malloc(len+1);
  memcpy(p, str, len);
  p[len] = 0;
  return p;
}

#ifndef M_PI
#define M_PI 3.1415926535897932384626433832795
#endif

#define S_IFREG  0100000
#define S_ISREG(m)      (((m) & S_IFMT) == S_IFREG)

#endif
