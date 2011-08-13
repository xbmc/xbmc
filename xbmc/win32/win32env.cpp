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
pgwin32_putenv(const char *envval)
{
	char	   *envcpy;
	char	   *cp;

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
	typedef int (_cdecl * PUTENVPROC) (const char *name);
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
					rtmodules[i].putenvFunc = (PUTENVPROC) GetProcAddress(rtmodules[i].hmodule, "_putenv");
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
	envcpy = strdup(envval);
	if (!envcpy)
		return -1;
	cp = strchr(envcpy, '=');
	if (cp == NULL)
	{
		free(envcpy);
		return -1;
	}
	*cp = '\0';
	cp++;
	if (strlen(cp))
	{
		/*
		 * Only call SetEnvironmentVariable() when we are adding a variable,
		 * not when removing it. Calling it on both crashes on at least
		 * certain versions of MingW.
		 */
		if (!SetEnvironmentVariable(envcpy, cp))
		{
			free(envcpy);
			return -1;
		}
	}
	free(envcpy);

	/* Finally, update our "own" cache */
	return _putenv(envval);
}

void
pgwin32_unsetenv(const char *name)
{
	char	   *envbuf;

	envbuf = (char *) malloc(strlen(name) + 2);
	if (!envbuf)
		return;

	sprintf(envbuf, "%s=", name);
	pgwin32_putenv(envbuf);
	free(envbuf);
}
