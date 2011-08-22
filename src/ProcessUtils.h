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

	private:
		static void runElevatedLinux(const std::string& executable,
		                             const std::list<std::string>& args);
		static void runElevatedMac(const std::string& executable,
		                           const std::list<std::string>& args);
		static void runElevatedWindows(const std::string& executable,
		                               const std::list<std::string>& args);

		static int runAsyncUnix(const std::string& executable,
		                         const std::list<std::string>& args);
		static void runAsyncWindows(const std::string& executable,
		                            const std::list<std::string>& args);
		static int runSyncUnix(const std::string& executable,
		                        const std::list<std::string>& args);
		static int runSyncWindows(const std::string& executable,
		                        const std::list<std::string>& args);
};

