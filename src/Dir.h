#pragma once

#include "Platform.h"

#include <string>

#ifdef PLATFORM_UNIX
#include <dirent.h>
#endif

class Dir
{
	public:
		Dir(const char* path);
		~Dir();

		// iterate to the next entry in the directory
		bool next();

		// methods to return information about
		// the current entry
		std::string fileName() const;
		std::string filePath() const;
		bool isDir() const;
	
	private:
		std::string m_path;

#ifdef PLATFORM_UNIX
		DIR* m_dir;
		dirent* m_entry;
#endif

#ifdef PLATFORM_WINDOWS
		HANDLE m_findHandle;
		WIN32_FIND_DATA m_findData;
		bool m_firstEntry;
#endif
};

