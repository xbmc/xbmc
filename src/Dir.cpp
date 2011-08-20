#include "Dir.h"

#ifdef PLATFORM_UNIX
 #include <dirent.h>
#endif

Dir::Dir(const char* path)
: m_dir(0)
{
	m_path = path;
	m_dir = opendir(path);
}

Dir::~Dir()
{
	closedir(m_dir);
}

bool Dir::next()
{
	m_entry = readdir(m_dir);
	return m_entry != 0;
}

std::string Dir::fileName() const
{
	return m_entry->d_name;
}

std::string Dir::filePath() const
{
	return m_path + '/' + fileName();
}

bool Dir::isDir() const
{
	return m_entry->d_type == DT_DIR;
}

