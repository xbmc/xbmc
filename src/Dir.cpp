#include "Dir.h"

#ifdef PLATFORM_UNIX
 #include <dirent.h>
#endif

Dir::Dir(const char* path)
{
	m_path = path;

#ifdef PLATFORM_UNIX
	m_dir = opendir(path);
#else
	m_findHandle = FindFirstFile(path,&m_findData);
	m_firstEntry = true;
#endif
}

Dir::~Dir()
{
#ifdef PLATFORM_UNIX
	closedir(m_dir);
#else
	FindClose(m_findHandle);
#endif
}

bool Dir::next()
{
#ifdef PLATFORM_UNIX
	m_entry = readdir(m_dir);
	return m_entry != 0;
#else
	bool result;
	if (m_firstEntry)
	{
		m_firstEntry = false;
		return m_findHandle != INVALID_HANDLE_VALUE;
	}
	else
	{
		result = FindNextFile(m_findHandle,&m_findData);
	}
	return result;
#endif
}

std::string Dir::fileName() const
{
#ifdef PLATFORM_UNIX
	return m_entry->d_name;
#else
	return m_findData.cFileName;
#endif
}

std::string Dir::filePath() const
{
	return m_path + '/' + fileName();
}

bool Dir::isDir() const
{
#ifdef PLATFORM_UNIX
	return m_entry->d_type == DT_DIR;
#else
	return (m_findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#endif
}

