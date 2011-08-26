#pragma once

#include "Platform.h"

#include <list>
#include <string>

class ProcessUtils
{
	public:
		enum Errors
		{
			/** Status code returned by runElevated() if launching
			  * the elevated process fails.
			  */
			RUN_ELEVATED_FAILED = 255
		};

		static PLATFORM_PID currentProcessId();

		static std::string currentProcessPath();

		static int runSync(const std::string& executable,
		                    const std::list<std::string>& args);

		static void runAsync(const std::string& executable,
		                     const std::list<std::string>& args);

		/** Run a process with administrative privileges and return the
		  * status code of the process, or 0 on Windows.
		  *
		  * Returns RUN_ELEVATED_FAILED if the elevated process could
		  * not be started.
		  */
		static int runElevated(const std::string& executable,
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
		static int runElevatedLinux(const std::string& executable,
		                             const std::list<std::string>& args);
		static int runElevatedMac(const std::string& executable,
		                           const std::list<std::string>& args);
		static int runElevatedWindows(const std::string& executable,
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

