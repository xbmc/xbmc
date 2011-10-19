#pragma once

#include <exception>
#include <string>

#include "Platform.h"
#include "StringUtils.h"


/** A set of functions for performing common operations
  * on files, throwing exceptions if an operation fails.
  *
  * Path arguments to FileUtils functions should use Unix-style path
  * separators.
  */
class FileUtils
{
	public:
		/** Base class for exceptions reported by
		  * FileUtils methods if an operation fails.
		  */
		class IOException : public std::exception
		{
			public:
				IOException(const std::string& error);
				IOException(int errorCode, const std::string& error);

				virtual ~IOException() throw ();

				enum Type
				{
					NoError,
					/** Unknown error type.  Call what() to get the description
					  * provided by the OS.
					  */
					Unknown,
					ReadOnlyFileSystem,
					DiskFull
				};

				virtual const char* what() const throw ()
				{
					return m_error.c_str();
				}

				Type type() const;

			private:
				void init(int errorCode, const std::string& error);

				std::string m_error;
				int m_errorCode;
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

		/** Set the permissions of a file.  @p permissions uses the standard
		  * Unix mode_t values.
		  */
		static void chmod(const char* path, int permissions) throw (IOException);
		static bool fileExists(const char* path) throw (IOException);
		static int fileMode(const char* path) throw (IOException);
		static void moveFile(const char* src, const char* dest) throw (IOException);
		static void mkdir(const char* dir) throw (IOException);
		static void rmdir(const char* dir) throw (IOException);
		static void createSymLink(const char* link, const char* target) throw (IOException);
		static void touch(const char* path) throw (IOException);
		static void copyFile(const char* src, const char* dest) throw (IOException);

		/** Create all the directories in @p path which do not yet exist.
		  * @p path may be relative or absolute.
		  */
		static void mkpath(const char* path) throw (IOException);

		/** Returns the file name part of a file path, including the extension. */
		static std::string fileName(const char* path);

		/** Returns the directory part of a file path.
		 * On Windows this includes the drive letter, if present in @p path.
		 */
		static std::string dirname(const char* path);

		/** Remove a directory and all of its contents. */
		static void rmdirRecursive(const char* dir) throw (IOException);

		/** Return the full, absolute path to a file, resolving any
		  * symlinks and removing redundant sections.
		  */
		static std::string canonicalPath(const char* path);

		/** Returns the path to a directory for storing temporary files. */
		static std::string tempPath();

		/** Extract the file @p src from the zip archive @p zipFile and
		  * write it to @p dest.
		  */
		static void extractFromZip(const char* zipFile, const char* src, const char* dest) throw (IOException);

		/** Returns a copy of the path 'str' with Windows-style '\'
		 * dir separators converted to Unix-style '/' separators
		 */
		static std::string toUnixPathSeparators(const std::string& str);

		static std::string toWindowsPathSeparators(const std::string& str);

		/** Returns true if the provided path is relative.
		  * Or false if absolute.
		  */
		static bool isRelative(const char* path);

		/** Converts @p path to an absolute path.  If @p path is already absolute,
		  * just returns @p path, otherwise prefixes it with @p basePath to make it absolute.
		  *
		  * @p basePath should be absolute.
		  */
		static std::string makeAbsolute(const char* path, const char* basePath);

		static void writeFile(const char* path, const char* data, int length) throw (IOException);

		/** Changes the current working directory to @p path */
		static void chdir(const char* path) throw (IOException);

		/** Returns the current working directory of the application. */
		static std::string getcwd() throw (IOException);
};

