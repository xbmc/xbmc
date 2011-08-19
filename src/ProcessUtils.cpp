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
