#pragma once

#include <string>
#include <fstream>

class Log
{
	public:
		enum Type
		{
			Info,
			Warn,
			Error
		};

		Log();
		~Log();

		void open(const std::string& path);

		void write(Type type, const std::string& text);
		void write(Type type, const char* text);

		static Log* instance();
	
	private:
		static void writeToStream(std::ostream& stream, Type type, const char* text);

		std::ofstream m_output;
};

inline void Log::write(Type type, const std::string& text)
{
	write(type,text.c_str());
}

#define LOG(type,text) \
  Log::instance()->write(Log::type,text)


