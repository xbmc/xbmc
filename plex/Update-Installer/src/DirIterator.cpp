#include "DirIterator.h"

#include "Log.h"
#include "StringUtils.h"

#ifdef PLATFORM_UNIX
 #include <dirent.h>
#endif

#include <string.h>

DirIterator::DirIterator(const char* path)
{
	m_path = path;

#ifdef PLATFORM_UNIX
	m_dir = opendir(path);
	m_entry = 0;
#else
	// to list the contents of a directory, the first
	// argument to FindFirstFile needs to be a wildcard
	// of the form: C:\path\to\dir\*
	std::string searchPath = m_path;
	if (!endsWith(searchPath,"/"))
	{
		searchPath.append("/");
	}
	searchPath.append("*");
	m_findHandle = FindFirstFile(searchPath.c_str(),&m_findData);
	m_firstEntry = true;
#endif
}

DirIterator::~DirIterator()
{
#ifdef PLATFORM_UNIX
	closedir(m_dir);
#else
	FindClose(m_findHandle);
#endif
}

bool DirIterator::next()
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

std::string DirIterator::fileName() const
{
#ifdef PLATFORM_UNIX
	return m_entry->d_name;
#else
	return m_findData.cFileName;
#endif
}

std::string DirIterator::filePath() const
{
	return m_path + '/' + fileName();
}

bool DirIterator::isDir() const
{
#ifdef PLATFORM_UNIX
	return m_entry->d_type == DT_DIR;
#else
	return (m_findData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0;
#endif
}

