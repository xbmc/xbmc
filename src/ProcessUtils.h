#pragma once

#include "Platform.h"

#include <list>
#include <string>

class ProcessUtils
{
	public:
		static PLATFORM_PID currentProcessId();

		static std::string currentProcessPath();

		static int runSync(const std::string& executable,
		                    const std::list<std::string>& args);

		static void runAsync(const std::string& executable,
		                     const std::list<std::string>& args);

		static void runElevated(const std::string& executable,
		                        const std::list<std::string>& args);

		static bool waitForProcess(PLATFORM_PID pid);

#ifdef PLATFORM_WINDOWS
		/** Convert a unicode command line returned by GetCommandLineW()
		 * to a standard (argc,argv) pair.  The resulting argv array and each
		 * element of argv must be freed using free()
		 */
		static void convertWindowsCommandLine(LPCWSTR commandLine, int& argc, char**& argv);
#endif

	private:
		static void runElevatedLinux(const std::string& executable,
		                             const std::list<std::string>& args);
		static void runElevatedMac(const std::string& executable,
		                           const std::list<std::string>& args);
		static void runElevatedWindows(const std::string& executable,
		                               const std::list<std::string>& args);

		static PLATFORM_PID runAsyncUnix(const std::string& executable,
		                         const std::list<std::string>& args);
		static void runAsyncWindows(const std::string& executable,
		                            const std::list<std::string>& args);
		static int runSyncUnix(const std::string& executable,
		                        const std::list<std::string>& args);
		static int runSyncWindows(const std::string& executable,
		                        const std::list<std::string>& args);
};

