#include "ProcessUtils.h"

#include "FileOps.h"
#include "Platform.h"
#include "StringUtils.h"
#include "Log.h"

#include <string.h>
#include <vector>
#include <iostream>

#ifdef PLATFORM_WINDOWS
#include <windows.h>
#else
#include <stdlib.h>
#include <sys/wait.h>
#include <errno.h>
#endif

#ifdef PLATFORM_MAC
#include <Security/Security.h>
#include <mach-o/dyld.h>
#endif

PLATFORM_PID ProcessUtils::currentProcessId()
{
#ifdef PLATFORM_UNIX
	return getpid();
#else
	return GetCurrentProcessId();
#endif
}

int ProcessUtils::runSync(const std::string& executable,
		                   const std::list<std::string>& args)
{
#ifdef PLATFORM_UNIX
	return runSyncUnix(executable,args);
#else
	return runSyncWindows(executable,args);
#endif
}

#ifdef PLATFORM_UNIX
int ProcessUtils::runSyncUnix(const std::string& executable,
		                        const std::list<std::string>& args)
{
	int pid = runAsyncUnix(executable,args);
	int status = 0;
	waitpid(pid,&status,0);
	return status;
}
#endif

#ifdef PLATFORM_WINDOWS
int ProcessUtils::runSyncWindows(const std::string& executable,
		                         const std::list<std::string>& args)
{
	// TODO - Implement me
	return 0;
}
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

bool ProcessUtils::waitForProcess(PLATFORM_PID pid)
{
#ifdef PLATFORM_UNIX
	pid_t result = ::waitpid(pid, 0, 0);	
	if (result < 0)
	{
		LOG(Error,"waitpid() failed with error: " + std::string(strerror(errno)));
	}
	return result > 0;
#elif defined(PLATFORM_WINDOWS)
	HANDLE hProc;

	if (!(hProc = OpenProcess(SYNCHRONIZE, FALSE, pid)))
	{
		LOG(Error,"Unable to get process handle for pid " + intToStr(pid) + " last error " + intToStr(GetLastError()));
		return false;
	}

	DWORD dwRet = WaitForSingleObject(hProc, INFINITE);
	CloseHandle(hProc);

	if (dwRet == WAIT_FAILED)
	{
		LOG(Error,"WaitForSingleObject failed with error " + intToStr(GetLastError()));
	}

	return (dwRet == WAIT_OBJECT_0);
#endif
}

#ifdef PLATFORM_LINUX
void ProcessUtils::runElevatedLinux(const std::string& executable,
							const std::list<std::string>& args)
{
	std::string sudoMessage = FileOps::fileName(executable.c_str()) + " needs administrative privileges.  Please enter your password.";

	std::vector<std::string> sudos;
	sudos.push_back("kdesudo");
	sudos.push_back("gksudo");
	sudos.push_back("gksu");

	for (int i=0; i < sudos.size(); i++)
	{
		const std::string& sudoBinary = sudos.at(i);

		std::list<std::string> sudoArgs;
		sudoArgs.push_back("-u");
		sudoArgs.push_back("root");

		if (sudoBinary == "kdesudo")
		{
			sudoArgs.push_back("-d");
			sudoArgs.push_back("--comment");
			sudoArgs.push_back(sudoMessage);
		}
		else
		{
			sudoArgs.push_back(sudoMessage);
		}

		sudoArgs.push_back("--");
		sudoArgs.push_back(executable);
		std::copy(args.begin(),args.end(),std::back_inserter(sudoArgs));

		// != 255: some sudo has been found and user failed to authenticate
		// or user authenticated correctly
		int result = ProcessUtils::runSync(sudoBinary,sudoArgs);
		if (result != 255)
		{
			break;
		}
	}
}
#endif

#ifdef PLATFORM_MAC
void ProcessUtils::runElevatedMac(const std::string& executable,
						const std::list<std::string>& args)
{
	// request elevation using the Security Service.
	//
	// This only works when the application is being run directly
	// from the Mac.  Attempting to run the app via a remote SSH session
	// (for example) will fail with an interaction-not-allowed error

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
			char** argv;
			argv = (char**) malloc(sizeof(char*) * args.size() + 1);
		
			int i = 0;
			for (std::list<std::string>::const_iterator iter = args.begin(); iter != args.end(); iter++)
			{
				argv[i] = strdup(iter->c_str());
				++i;
			}
			argv[i] = NULL;

			FILE* pipe = NULL;

			char* tool = strdup(executable.c_str());
			
			status = AuthorizationExecuteWithPrivileges(authorizationRef, tool,
					kAuthorizationFlagDefaults, argv, &pipe);

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
			for (i = 0; i < args.size(); i++)
			{
				free(argv[i]);
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
int ProcessUtils::runAsyncUnix(const std::string& executable,
						const std::list<std::string>& args)
{
	pid_t child = fork();
	if (child == 0)
	{
		// in child process
		char** argBuffer = new char*[args.size() + 2];
		argBuffer[0] = strdup(executable.c_str());
		int i = 1;
		for (std::list<std::string>::const_iterator iter = args.begin(); iter != args.end(); iter++)
		{
			argBuffer[i] = strdup(iter->c_str());
			++i;
		}
		argBuffer[i] = 0;

		if (execvp(executable.c_str(),argBuffer) == -1)
		{
			LOG(Error,"error starting child: " + std::string(strerror(errno)));
			exit(1);
		}
	}
	else
	{
		LOG(Info,"Started child process " + intToStr(child));
	}
}
#endif

#ifdef PLATFORM_WINDOWS
void ProcessUtils::runAsyncWindows(const std::string& executable,
						const std::list<std::string>& args)
{
	std::string commandLine = executable;
	for (std::list<std::string>::const_iterator iter = args.begin(); iter != args.end(); iter++)
	{
		if (!commandLine.empty())
		{
			commandLine.append(" ");
		}
		commandLine.append(*iter);
	}

	STARTUPINFO startupInfo;
	ZeroMemory(&startupInfo,sizeof(startupInfo));
	startupInfo.cb = sizeof(startupInfo);

	PROCESS_INFORMATION processInfo;
	ZeroMemory(&processInfo,sizeof(processInfo));

	char* commandLineStr = strdup(commandLine.c_str());
	bool result = CreateProcess(
	    executable.c_str(),
		commandLineStr,
		0 /* process attributes */,
		0 /* thread attributes */,
		false /* inherit handles */,
		NORMAL_PRIORITY_CLASS /* creation flags */,
		0 /* environment */,
		0 /* current directory */,
		&startupInfo /* startup info */,
		&processInfo /* process information */
	);
	if (!result)
	{
		LOG(Error,"Failed to start child process. " + executable + " Last error: " + intToStr(GetLastError()));
	}
}
#endif
		
std::string ProcessUtils::currentProcessPath()
{
#ifdef PLATFORM_LINUX
	std::string path = FileOps::canonicalPath("/proc/self/exe");
	LOG(Info,"Current process path " + path);
	return path;
#elif defined(PLATFORM_MAC)
	uint32_t bufferSize = PATH_MAX;
	char buffer[bufferSize];
	_NSGetExecutablePath(buffer,&bufferSize);
	return buffer;
#else
	char fileName[MAX_PATH];
	GetModuleFileName(0 /* get path of current process */,fileName,MAX_PATH);
	return fileName;
#endif
}

