#pragma once

#include <list>
#include <string>

class ProcessUtils
{
	public:
		static std::string currentProcessPath();

		static void runAsync(const std::string& executable,
		                     const std::list<std::string>& args);

		static void runElevated(const std::string& executable,
		                        const std::list<std::string>& args);

		static bool waitForProcess(long long pid);

	private:
		static void runElevatedLinux(const std::string& executable,
		                             const std::list<std::string>& args);
		static void runElevatedMac(const std::string& executable,
		                           const std::list<std::string>& args);
		static void runElevatedWindows(const std::string& executable,
		                               const std::list<std::string>& args);

		static void runAsyncUnix(const std::string& executable,
		                         const std::list<std::string>& args);
		static void runAsyncWindows(const std::string& executable,
		                            const std::list<std::string>& args);
};

