/* __eprintf shouldn't have been made visible from libstdc++, or
+   anywhere, but on Mac OS X 10.4 it was defined in
+   libstdc++.6.0.3.dylib; so on that platform we have to keep defining
+   it to keep binary compatibility.  We can't just put the libgcc
+   version in the export list, because that doesn't work; once a
+   symbol is marked as hidden, it stays that way.  */
#if defined(__APPLE__) && (MAC_OS_X_VERSION_MAX_ALLOWED < 1050)
#include <cstdio>
#include <cstdlib>

using namespace std;

extern "C" void __attribute__ ((weak))
__eprintf(const char *string, const char *expression,
          unsigned int line, const char *filename)
  {
    fprintf(stderr, string, expression, line, filename);
    fflush(stderr);
    abort();
  }

#endif
