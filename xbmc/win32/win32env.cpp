/*-------------------------------------------------------------------------
 *
 * win32env.c
 *	  putenv() and unsetenv() for win32, that updates both process
 *	  environment and the cached versions in (potentially multiple)
 *	  MSVCRT.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/port/win32env.c
 *
 *-------------------------------------------------------------------------
 */

//#include "windows.h"

int
pgwin32_wputenv(const wchar_t *envval)
{
	wchar_t	   *envcpy;
	wchar_t	   *cp;

	/*
	 * Each version of MSVCRT has its own _putenv() call in the runtime
	 * library.
	 *
	 * mingw always uses MSVCRT.DLL, but if we are in a Visual C++
	 * environment, attempt to update the environment in all MSVCRT modules
	 * that are currently loaded, to work properly with any third party
	 * libraries linked against a different MSVCRT but still relying on
	 * environment variables.
	 *
	 * Also separately update the system environment that gets inherited by
	 * subprocesses.
	 */
#ifdef _MSC_VER
	typedef int (_cdecl * PUTENVPROC) (const wchar_t *name);
	static struct
	{
		char	   *modulename;
		HMODULE		hmodule;
		PUTENVPROC	putenvFunc;
	}			rtmodules[] =
	{
		{
			"msvcrt", 0, NULL
		},						/* Visual Studio 6.0 / mingw */
		{
			"msvcr70", 0, NULL
		},						/* Visual Studio 2002 */
		{
			"msvcr71", 0, NULL
		},						/* Visual Studio 2003 */
		{
			"msvcr80", 0, NULL
		},						/* Visual Studio 2005 */
		{
			"msvcr90", 0, NULL
		},						/* Visual Studio 2008 */
		{
			"msvcr100", 0, NULL
		},						/* Visual Studio 2010 */
		{
			NULL, 0, NULL
		}
	};
	int			i;

	for (i = 0; rtmodules[i].modulename; i++)
	{
		if (rtmodules[i].putenvFunc == NULL)
		{
			if (rtmodules[i].hmodule == 0)
			{
				/* Not attempted before, so try to find this DLL */
				rtmodules[i].hmodule = GetModuleHandle(rtmodules[i].modulename);
				if (rtmodules[i].hmodule == NULL)
				{
					/*
					 * Set to INVALID_HANDLE_VALUE so we know we have tried
					 * this one before, and won't try again.
					 */
					rtmodules[i].hmodule = ((HMODULE)(LONG_PTR)-1);
					continue;
				}
				else
				{
					rtmodules[i].putenvFunc = (PUTENVPROC) GetProcAddress(rtmodules[i].hmodule, "_wputenv");
					if (rtmodules[i].putenvFunc == NULL)
					{
						CloseHandle(rtmodules[i].hmodule);
						rtmodules[i].hmodule = ((HMODULE)(LONG_PTR)-1);
						continue;
					}
				}
			}
			else
			{
				/*
				 * Module loaded, but we did not find the function last time.
				 * We're not going to find it this time either...
				 */
				continue;
			}
		}
		/* At this point, putenvFunc is set or we have exited the loop */
		rtmodules[i].putenvFunc(envval);
	}
#endif   /* _MSC_VER */

	/*
	 * Update the process environment - to make modifications visible to child
	 * processes.
	 *
	 * Need a copy of the string so we can modify it.
	 */
	envcpy = wcsdup(envval);
	if (!envcpy)
		return -1;
	cp = wcschr(envcpy, '=');
	if (cp == NULL)
	{
		free(envcpy);
		return -1;
	}
	*cp = '\0';
	cp++;
	if (wcslen(cp))
	{
		/*
		 * Only call SetEnvironmentVariable() when we are adding a variable,
		 * not when removing it. Calling it on both crashes on at least
		 * certain versions of MingW.
		 */
		if (!SetEnvironmentVariableW(envcpy, cp))
		{
			free(envcpy);
			return -1;
		}
	}
	free(envcpy);

	/* Finally, update our "own" cache */
	return _wputenv(envval);
}

/*takes utf8 encoding*/
int pgwin32_putenv(const char *envval)
{
  int size_needed = MultiByteToWideChar(CP_UTF8, 0, envval, strlen(envval), NULL, 0);
  std::wstring strTo(size_needed, 0);
  MultiByteToWideChar(CP_UTF8, 0, envval, strlen(envval), &strTo[0], size_needed);
  return pgwin32_wputenv(strTo.c_str());
}

void
pgwin32_unsetenv(const wchar_t *name)
{
	wchar_t	   *envbuf;

	envbuf = (wchar_t *) malloc(wcslen(name) + 2);
	if (!envbuf)
		return;

	wsprintfW(envbuf, L"%s=", name);
	pgwin32_wputenv(envbuf);
	free(envbuf);
}
