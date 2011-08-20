#include "ProcessUtils.h"

#include "Platform.h"
#include "StringUtils.h"
#include "Log.h"

#include <string.h>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#endif

void ProcessUtils::runAsync(const std::string& executable,
		      const std::list<std::string>& args)
{
#ifdef PLATFORM_WINDOWS
	runAsyncWindows(executable,args);
#elif defined(PLATFORM_UNIX)
	runAsyncUnix(executable,args);
#endif
}

void ProcessUtils::runElevated(const std::string& executable,
					const std::list<std::string>& args)
{
#ifdef PLATFORM_WINDOWS
	runElevatedWindows(executable,args);
#elif defined(PLATFORM_MAC)
	runElevatedMac(executable,args);
#elif defined(PLATFORM_LINUX)
	runElevatedLinux(executable,args);
#endif
}

bool ProcessUtils::waitForProcess(long long pid)
{
#ifdef PLATFORM_UNIX
	pid_t result = ::waitpid(static_cast<pid_t>(pid), 0, 0);	
	if (result < 0)
	{
		LOG(Error,"waitpid() failed with error: " + std::string(strerror(errno)));
	}
	return result > 0;
#elif defined(PLATFORM_WINDOWS)
	HANDLE hProc;

	if (!(hProc = OpenProcess(SYNCHRONIZE, FALSE, static_cast<DWORD>(pid))))
	{
		LOG(Error,"Unable to get process handle for pid" + intToStr(pid) + "last error" + intToStr(GetLastError()));
		return false;
	}

	DWORD dwRet = WaitForSingleObject(hProc, INFINITE);
	CloseHandle(hProc);

	if (dwRet == WAIT_FAILED)
	{
		debug_logger(m_debugLog) << "WaitForSingleObject failed with error" << GetLastError();
	}

	return (dwRet == WAIT_OBJECT_0);
#endif
}

#ifdef PLATFORM_LINUX
void ProcessUtils::runElevatedLinux(const std::string& executable,
							const std::list<std::string>& args)
{
}
#endif

#ifdef PLATFORM_MAC
void ProcessUtils::runElevatedMac(const std::string& executable,
						const std::list<std::string>& args)
{
	OSStatus status;
	AuthorizationRef authorizationRef;
	
	status = AuthorizationCreate(
			NULL,
			kAuthorizationEmptyEnvironment,
			kAuthorizationFlagDefaults,
			&authorizationRef);

	AuthorizationItem right = { kAuthorizationRightExecute, 0, NULL, 0 };
	AuthorizationRights rights = { 1, &right };
	
	AuthorizationFlags flags = kAuthorizationFlagDefaults |
			kAuthorizationFlagInteractionAllowed |
			kAuthorizationFlagPreAuthorize |
			kAuthorizationFlagExtendRights;

	if (status == errAuthorizationSuccess)
	{
		status = AuthorizationCopyRights(authorizationRef, &rights, NULL, 
				flags, NULL);

		if (status == errAuthorizationSuccess)
		{
			char** args;
			args = (char**) malloc(sizeof(char*) * arguments.count() + 1);
		
			int i;
			for (i = 0; i < arguments.size(); i++)
			{
				args[i] = strdup(arguments[i].c_str());
			}
			args[i] = NULL;

			FILE* pipe = NULL;

			char* tool = strdup(executable.c_str());
			
			status = AuthorizationExecuteWithPrivileges(authorizationRef, tool,
					kAuthorizationFlagDefaults, args, &pipe);

			if (status == errAuthorizationSuccess)
			{
				// AuthorizationExecuteWithPrivileges does not provide a way to get the process ID
				// of the child process.
				//
				// Discussions on Apple development forums suggest two approaches for working around this,
				//
				// - Modify the child process to sent its process ID back to the parent via
				//   the pipe passed to AuthorizationExecuteWithPrivileges.
				//
				// - Use the generic Unix wait() call.
				//
				// This code uses wait(), which is simpler, but suffers from the problem that wait() waits
				// for any child process, not necessarily the specific process launched
				// by AuthorizationExecuteWithPrivileges.
				//
				// Apple's documentation (see 'Authorization Services Programming Guide') suggests
				// installing files in an installer as a legitimate use for 
				// AuthorizationExecuteWithPrivileges but in general strongly recommends
				// not using this call and discusses a number of other alternatives
				// for performing privileged operations,
				// which we could consider in future.

				int childStatus;
				pid_t childPid = wait(&childStatus);

				if (childStatus != 0)
				{
					LOG(Error,"elevated process failed with status " + intToStr(childStatus) + " pid "
					          + intToStr(childPid));
				}
				else
				{
					LOG(Info,"elevated process succeded with pid " + intToStr(childPid));
				}
			}
			else
			{
				LOG(Error,"failed to launch elevated process " + intToStr(status));
			}
			
			// If we want to know more information about what has happened:
			// http://developer.apple.com/mac/library/documentation/Security/Reference/authorization_ref/Reference/reference.html#//apple_ref/doc/uid/TP30000826-CH4g-CJBEABHG
			free(tool);
			for (i = 0; i < arguments.count(); i++)
			{
				free(args[i]);
			}
		}
		else
		{
			LOG(Error,"failed to get rights to launch elevated process. status: " + intToStr(status));
		}
	}
}
#endif

#ifdef PLATFORM_WINDOWS
void ProcessUtils::runElevatedWindows(const std::string& executable,
							const std::list<std::string>& args)
{
}
#endif

#ifdef PLATFORM_UNIX
void ProcessUtils::runAsyncUnix(const std::string& executable,
						const std::list<std::string>& args)
{
}
#endif

#ifdef PLATFORM_WINDOWS
void ProcessUtils::runAsyncWindows(const std::string& executable,
						const std::list<std::string>& args)
{
}
#endif
