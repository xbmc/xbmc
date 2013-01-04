/*
 * Adopted from Apache DSO code.
 * Portions copyright Apache Software Foundation
 *
 * Structures and types used to implement dlopen, dlsym, etc.
 * on Windows 95/NT.
 */
#include <windows.h>
#include <string.h>
#include <stdio.h>

#include "dlfcn.h"
#include "os_types.h"

void *dlopen(const char *module_name, int mode)
{
    UINT em;
    HINSTANCE dsoh;
    char path[MAX_PATH], *p;
    /* Load the module...
     * per PR2555, the LoadLibraryEx function is very picky about slashes.
     * Debugging on NT 4 SP 6a reveals First Chance Exception within NTDLL.
     * LoadLibrary in the MS PSDK also reveals that it -explicitly- states
     * that backslashes must be used.
     *
     * Transpose '\' for '/' in the filename.
     */
    (void)strncpy(path, module_name, MAX_PATH);
    p = path;
    while (p = strchr(p, '/'))
        *p = '\\';

    /* First assume the dso/dll's required by -this- dso are sitting in the
     * same path or can be found in the usual places.  Failing that, let's
     * let that dso look in the apache root.
     */
    em = SetErrorMode(SEM_FAILCRITICALERRORS);
    dsoh = LoadLibraryEx(path, NULL, LOAD_WITH_ALTERED_SEARCH_PATH);
    if (!dsoh)
    {
        SetLastError(0); // clear the last error
        dsoh = LoadLibraryEx(path, NULL, 0);
    }
    SetErrorMode(em);
    SetLastError(0); // clear the last error
    return (void *)dsoh;
}

char *dlerror(void)
{
    int len, nErrorCode;
    static char errstr[120];
    /* This is -not- threadsafe code, but it's about the best we can do.
     * mostly a potential problem for isapi modules, since LoadModule
     * errors are handled within a single config thread.
     */

    if((nErrorCode = GetLastError()) == 0)
      return((char *)0);

    SetLastError(0); // clear the last error
    len = snprintf(errstr, sizeof(errstr), "(%d) ", nErrorCode);

    len += FormatMessage(
            FORMAT_MESSAGE_FROM_SYSTEM,
            NULL,
            nErrorCode,
            MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), /* Default language */
            (LPTSTR) errstr + len,
            sizeof(errstr) - len,
            NULL
        );
        /* FormatMessage may have appended a newline (\r\n). So remove it
         * and use ": " instead like the Unix errors. The error may also
         * end with a . before the return - if so, trash it.
         */
    if (len > 1 && errstr[len-2] == '\r' && errstr[len-1] == '\n') {
        if (len > 2 && errstr[len-3] == '.')
            len--;
        errstr[len-2] = ':';
        errstr[len-1] = ' ';
    }
    return errstr;
}

int dlclose(void *handle)
{
  return  FreeLibrary(handle);
}

void *dlsym(void *handle, const char *name)
{
  return GetProcAddress(handle, name);
}
