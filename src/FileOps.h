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

		enum QtFilePermission
		{
			ReadOwner  = 0x4000,
			WriteOwner = 0x2000,
			ExecOwner  = 0x1000,
			ReadUser   = 0x0400,
			WriteUser  = 0x0200,
			ExecUser   = 0x0100,
			ReadGroup  = 0x0040,
			WriteGroup = 0x0020,
			ExecGroup  = 0x0010,
			ReadOther  = 0x0004,
			WriteOther = 0x0002,
			ExecOther  = 0x0001
		};

		/** Remove a file.  Throws an exception if the file
		 * could not be removed.
		 *
		 * On Unix, a file can be removed even if it is in use if the user
		 * has the necessary permissions.  removeFile() tries to simulate
		 * this behavior on Windows.  If a file cannot be removed on Windows
		 * because it is in use it will be moved to a temporary directory and
		 * scheduled for deletion on the next restart.
		 */
		static void removeFile(const char* src) throw (IOException);

		static void setQtPermissions(const char* path, int permissions) throw (IOException);
		static bool fileExists(const char* path) throw (IOException);
		static void moveFile(const char* src, const char* dest) throw (IOException);
		static void extractFromZip(const char* zipFile, const char* src, const char* dest) throw (IOException);
		static void mkdir(const char* dir) throw (IOException);
		static void rmdir(const char* dir) throw (IOException);
		static void createSymLink(const char* link, const char* target) throw (IOException);
		static void touch(const char* path) throw (IOException);
		static std::string fileName(const char* path);
		static std::string dirname(const char* path);
		static void rmdirRecursive(const char* dir) throw (IOException);
		static std::string canonicalPath(const char* path);
		static std::string tempPath();

	private:
		static int toUnixPermissions(int qtPermissions);

		// returns a copy of the path 'str' with Windows-style '\'
		// dir separators converted to Unix-style '/' separators
		static std::string toUnixPathSeparators(const std::string& str);
};

