#include "Log.h"

#include "Platform.h"
#include "StringUtils.h"
#include "ProcessUtils.h"

#include <string.h>
#include <iostream>

Log m_globalLog;

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
	stream << '(' << intToStr(ProcessUtils::currentProcessId()) << ") " << text << std::endl;
}

void Log::write(Type type, const char* text)
{
	writeToStream(std::cerr,type,text);
	if (m_output.is_open())
	{
		writeToStream(m_output,type,text);
	}
}

