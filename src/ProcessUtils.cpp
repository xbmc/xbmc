#include "ProcessUtils.h"

#include "Platform.h"

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
	// TODO
#elif defined(PLATFORM_WINDOWS)
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
