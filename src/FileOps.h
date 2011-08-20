#pragma once

#include <exception>
#include <string>

#include "StringUtils.h"

class FileOps
{
	public:
		class IOException : public std::exception
		{
			public:
				IOException(const std::string& error);

				virtual ~IOException() throw ();

				virtual const char* what() const throw ()
				{
					return m_error.c_str();
				}

			private:
				std::string m_error;
				int m_errno;
		};

		static bool fileExists(const char* path) throw (IOException);
		static void setPermissions(const char* path, int permissions) throw (IOException);
		static void moveFile(const char* src, const char* dest) throw (IOException);
		static void removeFile(const char* src) throw (IOException);
		static void extractFromZip(const char* zipFile, const char* src, const char* dest) throw (IOException);
		static void mkdir(const char* dir) throw (IOException);
		static void rmdir(const char* dir) throw (IOException);
		static void createSymLink(const char* link, const char* target) throw (IOException);
		static void touch(const char* path) throw (IOException);
		static std::string dirname(const char* path);
		static void rmdirRecursive(const char* dir) throw (IOException);
};

