#pragma once

#include "Platform.h"

#include <list>
#include <string>

/** A set of functions to get information about the current
  * process and launch new processes.
  */
class ProcessUtils
{
	public:
		enum Errors
		{
			/** Status code returned by runElevated() if launching
			  * the elevated process fails.
			  */
			RunElevatedFailed = 255,
			/** Status code returned by runSync() if the application
                          * cannot be started.
                          */
                        RunFailed = -8,
			/** Status code returned by runSync() if waiting for
                          * the application to exit and reading its status code fails.
                          */
                        WaitFailed = -1
		};

		static PLATFORM_PID currentProcessId();

		/** Returns the absolute path to the main binary for
		  * the current process.
		  */
		static std::string currentProcessPath();

		/** Start a process and wait for it to finish before
		  * returning its exit code.
		  *
		  * Returns -1 if the process cannot be started.
		  */
		static int runSync(const std::string& executable,
		                    const std::list<std::string>& args);

		/** Start a process and return without waiting for
		  * it to finish.
		  */
		static void runAsync(const std::string& executable,
		                     const std::list<std::string>& args);

		/** Run a process with administrative privileges and return the
		  * status code of the process, or 0 on Windows.
		  *
		  * Returns RunElevatedFailed if the elevated process could
		  * not be started.
		  */
		static int runElevated(const std::string& executable,
		                       const std::list<std::string>& args,
		                       const std::string& task);

		/** Wait for a process to exit.
		  * Returns true if the process was found and has exited or false
		  * otherwise.
		  */
		static bool waitForProcess(PLATFORM_PID pid);

#ifdef PLATFORM_WINDOWS
		/** Convert a unicode command line returned by GetCommandLineW()
		 * to a standard (argc,argv) pair.  The resulting argv array and each
		 * element of argv must be freed using free()
		 */
		static void convertWindowsCommandLine(LPCWSTR commandLine, int& argc, char**& argv);
#endif

	private:
		enum RunMode
		{
			RunSync,
			RunAsync
		};
		static int runElevatedLinux(const std::string& executable,
		                             const std::list<std::string>& args,
		                            const std::string& task);
		static int runElevatedMac(const std::string& executable,
		                           const std::list<std::string>& args);
		static int runElevatedWindows(const std::string& executable,
		                               const std::list<std::string>& args);

		static PLATFORM_PID runAsyncUnix(const std::string& executable,
		                         const std::list<std::string>& args);
		static int runWindows(const std::string& executable,
		                      const std::list<std::string>& args,
	                          RunMode runMode);
		static int runSyncUnix(const std::string& executable,
		                        const std::list<std::string>& args);
};

