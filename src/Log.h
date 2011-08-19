#pragma once

#include <string>

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
		int m_fd;
};

inline void Log::write(Type type, const std::string& text)
{
	write(type,text.c_str());
}

#define LOG(type,text) \
  Log::instance()->write(Log::type,text)


