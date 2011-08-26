#include "Log.h"

#include "Platform.h"
#include "StringUtils.h"

#include <string.h>
#include <iostream>

#ifdef PLATFORM_UNIX
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#endif

Log m_globalLog;

#ifdef PLATFORM_UNIX
pid_t currentProcessId = 0;
pid_t processId()
{
	if (currentProcessId == 0)
	{
		currentProcessId = getpid();
	}
	return currentProcessId;
}
#else
DWORD currentProcessId = 0;
DWORD processId()
{
	if (currentProcessId == 0)
	{
		currentProcessId = GetCurrentProcessId();
	}
	return currentProcessId;
}
#endif

Log* Log::instance()
{
	return &m_globalLog;
}

Log::Log()
{
}

Log::~Log()
{
}

void Log::open(const std::string& path)
{
	m_output.open(path.c_str(),std::ios_base::out | std::ios_base::app);
}

void Log::writeToStream(std::ostream& stream, Type type, const char* text)
{
	switch (type)
	{
		case Info:
			stream << "INFO  ";
			break;
		case Warn:
			stream << "WARN  ";
			break;
		case Error:
			stream << "ERROR ";
			break;
	}
	stream << '(' << intToStr(processId()) << ") " << text << std::endl;
}

void Log::write(Type type, const char* text)
{
	writeToStream(std::cerr,type,text);
	if (m_output.is_open())
	{
		writeToStream(m_output,type,text);
	}
}

