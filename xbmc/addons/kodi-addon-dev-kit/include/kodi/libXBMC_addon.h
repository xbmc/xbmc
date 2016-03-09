#pragma once
/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>
#include <vector>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdarg.h>

#if defined(BUILD_KODI_ADDON)
#include "IFileTypes.h"
#else
#include "filesystem/IFileTypes.h"
#endif

struct VFSDirEntry;

#ifdef _WIN32                   // windows
#ifndef _SSIZE_T_DEFINED
typedef intptr_t      ssize_t;
#define _SSIZE_T_DEFINED
#endif // !_SSIZE_T_DEFINED

#if defined(BUILD_KODI_ADDON)
#include "p8-platform/windows/dlfcn-win32.h"
#else
#include "dlfcn-win32.h"
#endif

#define ADDON_DLL               "\\library.xbmc.addon\\libXBMC_addon" ADDON_HELPER_EXT
#define ADDON_HELPER_EXT        ".dll"
#else // windows
// the ADDON_HELPER_ARCH is the platform dependend name which is used
// as part of the name of dynamic addon libraries. It has to match the 
// strings which are set in configure.ac for the "ARCH" variable.
#if defined(__APPLE__)          // osx
#if defined(__arm__) || defined(__aarch64__)
#define ADDON_HELPER_ARCH       "arm-osx"
#else
#define ADDON_HELPER_ARCH       "x86-osx"
#endif
#define ADDON_HELPER_EXT        ".dylib"
#else                           // linux
#if defined(__x86_64__)
#define ADDON_HELPER_ARCH       "x86_64-linux"
#elif defined(_POWERPC)
#define ADDON_HELPER_ARCH       "powerpc-linux"
#elif defined(_POWERPC64)
#define ADDON_HELPER_ARCH       "powerpc64-linux"
#elif defined(__ARMEL__)
#define ADDON_HELPER_ARCH       "arm"
#elif defined(__aarch64__)
#define ADDON_HELPER_ARCH       "aarch64"
#elif defined(__mips__)
#define ADDON_HELPER_ARCH       "mips"
#else
#define ADDON_HELPER_ARCH       "i486-linux"
#endif
#define ADDON_HELPER_EXT        ".so"
#endif
#include <dlfcn.h>              // linux+osx
#define ADDON_DLL_NAME "libXBMC_addon-" ADDON_HELPER_ARCH ADDON_HELPER_EXT
#define ADDON_DLL "/library.xbmc.addon/" ADDON_DLL_NAME
#endif
#if defined(ANDROID)
#include <sys/stat.h>
#endif

struct __stat64;

#ifdef LOG_DEBUG
#undef LOG_DEBUG
#endif
#ifdef LOG_INFO
#undef LOG_INFO
#endif
#ifdef LOG_NOTICE
#undef LOG_NOTICE
#endif
#ifdef LOG_ERROR
#undef LOG_ERROR
#endif

namespace ADDON
{
  typedef enum addon_log
  {
    LOG_DEBUG,
    LOG_INFO,
    LOG_NOTICE,
    LOG_ERROR
  } addon_log_t;

  typedef enum queue_msg
  {
    QUEUE_INFO,
    QUEUE_WARNING,
    QUEUE_ERROR
  } queue_msg_t;

  class CHelper_libXBMC_addon
  {
  public:
    CHelper_libXBMC_addon()
    {
      m_libXBMC_addon = NULL;
      m_Handle        = NULL;
    }

    ~CHelper_libXBMC_addon()
    {
      if (m_libXBMC_addon)
      {
        XBMC_unregister_me(m_Handle, m_Callbacks);
        dlclose(m_libXBMC_addon);
      }
    }

    bool RegisterMe(void *Handle)
    {
      m_Handle = Handle;

      std::string libBasePath;
      libBasePath  = ((cb_array*)m_Handle)->libPath;
      libBasePath += ADDON_DLL;

#if defined(ANDROID)
      struct stat st;
      if(stat(libBasePath.c_str(),&st) != 0)
      {
        std::string tempbin = getenv("XBMC_ANDROID_LIBS");
        libBasePath = tempbin + "/" + ADDON_DLL_NAME;
      }
#endif

      m_libXBMC_addon = dlopen(libBasePath.c_str(), RTLD_LAZY);
      if (m_libXBMC_addon == NULL)
      {
        fprintf(stderr, "Unable to load %s\n", dlerror());
        return false;
      }

      XBMC_register_me = (void* (*)(void *HANDLE))
        dlsym(m_libXBMC_addon, "XBMC_register_me");
      if (XBMC_register_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_unregister_me = (void (*)(void* HANDLE, void* CB))
        dlsym(m_libXBMC_addon, "XBMC_unregister_me");
      if (XBMC_unregister_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_log = (void (*)(void* HANDLE, void* CB, const addon_log_t loglevel, const char *msg))
        dlsym(m_libXBMC_addon, "XBMC_log");
      if (XBMC_log == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_get_setting = (bool (*)(void* HANDLE, void* CB, const char* settingName, void *settingValue))
        dlsym(m_libXBMC_addon, "XBMC_get_setting");
      if (XBMC_get_setting == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_queue_notification = (void (*)(void* HANDLE, void* CB, const queue_msg_t loglevel, const char *msg))
        dlsym(m_libXBMC_addon, "XBMC_queue_notification");
      if (XBMC_queue_notification == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_wake_on_lan = (bool (*)(void* HANDLE, void *CB, const char *mac))
        dlsym(m_libXBMC_addon, "XBMC_wake_on_lan");
      if (XBMC_wake_on_lan == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_unknown_to_utf8 = (char* (*)(void* HANDLE, void* CB, const char* str))
        dlsym(m_libXBMC_addon, "XBMC_unknown_to_utf8");
      if (XBMC_unknown_to_utf8 == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_get_localized_string = (char* (*)(void* HANDLE, void* CB, int dwCode))
        dlsym(m_libXBMC_addon, "XBMC_get_localized_string");
      if (XBMC_get_localized_string == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_free_string = (void (*)(void* HANDLE, void* CB, char* str))
        dlsym(m_libXBMC_addon, "XBMC_free_string");
      if (XBMC_free_string == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_get_dvd_menu_language = (char* (*)(void* HANDLE, void* CB))
        dlsym(m_libXBMC_addon, "XBMC_get_dvd_menu_language");
      if (XBMC_get_dvd_menu_language == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_open_file = (void* (*)(void* HANDLE, void* CB, const char* strFileName, unsigned int flags))
        dlsym(m_libXBMC_addon, "XBMC_open_file");
      if (XBMC_open_file == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_open_file_for_write = (void* (*)(void* HANDLE, void* CB, const char* strFileName, bool bOverWrite))
        dlsym(m_libXBMC_addon, "XBMC_open_file_for_write");
      if (XBMC_open_file_for_write == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_read_file = (ssize_t (*)(void* HANDLE, void* CB, void* file, void* lpBuf, size_t uiBufSize))
        dlsym(m_libXBMC_addon, "XBMC_read_file");
      if (XBMC_read_file == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_read_file_string = (bool (*)(void* HANDLE, void* CB, void* file, char *szLine, int iLineLength))
        dlsym(m_libXBMC_addon, "XBMC_read_file_string");
      if (XBMC_read_file_string == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_write_file = (ssize_t (*)(void* HANDLE, void* CB, void* file, const void* lpBuf, size_t uiBufSize))
        dlsym(m_libXBMC_addon, "XBMC_write_file");
      if (XBMC_write_file == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_flush_file = (void (*)(void* HANDLE, void* CB, void* file))
        dlsym(m_libXBMC_addon, "XBMC_flush_file");
      if (XBMC_flush_file == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_seek_file = (int64_t (*)(void* HANDLE, void* CB, void* file, int64_t iFilePosition, int iWhence))
        dlsym(m_libXBMC_addon, "XBMC_seek_file");
      if (XBMC_seek_file == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_truncate_file = (int (*)(void* HANDLE, void* CB, void* file, int64_t iSize))
        dlsym(m_libXBMC_addon, "XBMC_truncate_file");
      if (XBMC_truncate_file == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_get_file_position = (int64_t (*)(void* HANDLE, void* CB, void* file))
        dlsym(m_libXBMC_addon, "XBMC_get_file_position");
      if (XBMC_get_file_position == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_get_file_length = (int64_t (*)(void* HANDLE, void* CB, void* file))
        dlsym(m_libXBMC_addon, "XBMC_get_file_length");
      if (XBMC_get_file_length == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_get_file_download_speed = (double(*)(void* HANDLE, void* CB, void* file))
        dlsym(m_libXBMC_addon, "XBMC_get_file_download_speed");
      if (XBMC_get_file_download_speed == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_close_file = (void (*)(void* HANDLE, void* CB, void* file))
        dlsym(m_libXBMC_addon, "XBMC_close_file");
      if (XBMC_close_file == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_get_file_chunk_size = (int (*)(void* HANDLE, void* CB, void* file))
        dlsym(m_libXBMC_addon, "XBMC_get_file_chunk_size");
      if (XBMC_get_file_chunk_size == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_file_exists = (bool (*)(void* HANDLE, void* CB, const char *strFileName, bool bUseCache))
        dlsym(m_libXBMC_addon, "XBMC_file_exists");
      if (XBMC_file_exists == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_stat_file = (int (*)(void* HANDLE, void* CB, const char *strFileName, struct __stat64* buffer))
        dlsym(m_libXBMC_addon, "XBMC_stat_file");
      if (XBMC_stat_file == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_delete_file = (bool (*)(void* HANDLE, void* CB, const char *strFileName))
        dlsym(m_libXBMC_addon, "XBMC_delete_file");
      if (XBMC_delete_file == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_can_open_directory = (bool (*)(void* HANDLE, void* CB, const char* strURL))
        dlsym(m_libXBMC_addon, "XBMC_can_open_directory");
      if (XBMC_can_open_directory == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_create_directory = (bool (*)(void* HANDLE, void* CB, const char* strPath))
        dlsym(m_libXBMC_addon, "XBMC_create_directory");
      if (XBMC_create_directory == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_directory_exists = (bool (*)(void* HANDLE, void* CB, const char* strPath))
        dlsym(m_libXBMC_addon, "XBMC_directory_exists");
      if (XBMC_directory_exists == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_remove_directory = (bool (*)(void* HANDLE, void* CB, const char* strPath))
        dlsym(m_libXBMC_addon, "XBMC_remove_directory");
      if (XBMC_remove_directory == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_get_directory = (bool (*)(void* HANDLE, void* CB, const char* strPath, const char* mask, VFSDirEntry** items, unsigned int* num_items))
        dlsym(m_libXBMC_addon, "XBMC_get_directory");
      if (XBMC_get_directory == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_free_directory = (void (*)(void* HANDLE, void* CB, VFSDirEntry* items, unsigned int num_items))
        dlsym(m_libXBMC_addon, "XBMC_free_directory");
      if (XBMC_free_directory == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_curl_create = (void* (*)(void *HANDLE, void* CB, const char* strURL))
        dlsym(m_libXBMC_addon, "XBMC_curl_create");
      if (XBMC_curl_create == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_curl_add_option = (bool (*)(void *HANDLE, void* CB, void *file, XFILE::CURLOPTIONTYPE type, const char* name, const char *value))
        dlsym(m_libXBMC_addon, "XBMC_curl_add_option");
      if (XBMC_curl_add_option == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_curl_open = (bool (*)(void *HANDLE, void* CB, void *file, unsigned int flags))
        dlsym(m_libXBMC_addon, "XBMC_curl_open");
      if (XBMC_curl_open == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      m_Callbacks = XBMC_register_me(m_Handle);
      return m_Callbacks != NULL;
    }

    /*!
     * @brief Add a message to XBMC's log.
     * @param loglevel The log level of the message.
     * @param format The format of the message to pass to XBMC.
     */
    void Log(const addon_log_t loglevel, const char *format, ... )
    {
      char buffer[16384];
      va_list args;
      va_start (args, format);
      vsprintf (buffer, format, args);
      va_end (args);
      return XBMC_log(m_Handle, m_Callbacks, loglevel, buffer);
    }

    /*!
     * @brief Get a settings value for this add-on.
     * @param settingName The name of the setting to get.
     * @param settingValue The value.
     * @return True if the settings was fetched successfully, false otherwise.
     */
    bool GetSetting(const char* settingName, void *settingValue)
    {
      return XBMC_get_setting(m_Handle, m_Callbacks, settingName, settingValue);
    }

    /*!
     * @brief Queue a notification in the GUI.
     * @param type The message type.
     * @param format The format of the message to pass to display in XBMC.
     */
    void QueueNotification(const queue_msg_t type, const char *format, ... )
    {
      char buffer[16384];
      va_list args;
      va_start (args, format);
      vsprintf (buffer, format, args);
      va_end (args);
      return XBMC_queue_notification(m_Handle, m_Callbacks, type, buffer);
    }

    /*!
     * @brief Send WakeOnLan magic packet.
     * @param mac Network address of the host to wake.
     * @return True if the magic packet was successfully sent, false otherwise.
     */
    bool WakeOnLan(const char* mac)
    {
      return XBMC_wake_on_lan(m_Handle, m_Callbacks, mac);
    }

    /*!
     * @brief Translate a string with an unknown encoding to UTF8.
     * @param str The string to translate.
     * @return The string translated to UTF8. Must be freed by calling FreeString() when done.
     */
    char* UnknownToUTF8(const char* str)
    {
      return XBMC_unknown_to_utf8(m_Handle, m_Callbacks, str);
    }

    /*!
     * @brief Get a localised message.
     * @param dwCode The code of the message to get.
     * @return The message. Must be freed by calling FreeString() when done.
     */
    char* GetLocalizedString(int dwCode)
    {
      return XBMC_get_localized_string(m_Handle, m_Callbacks, dwCode);
    }


    /*!
     * @brief Get the DVD menu language.
     * @return The language. Must be freed by calling FreeString() when done.
     */
    char* GetDVDMenuLanguage()
    {
      return XBMC_get_dvd_menu_language(m_Handle, m_Callbacks);
    }

    /*!
     * @brief Free the memory used by str
     * @param str The string to free
     */
    void FreeString(char* str)
    {
      return XBMC_free_string(m_Handle, m_Callbacks, str);
    }

    /*!
     * @brief Open the file with filename via XBMC's CFile. Needs to be closed by calling CloseFile() when done.
     * @param strFileName The filename to open.
     * @param flags The flags to pass. Documented in XBMC's File.h
     * @return A handle for the file, or NULL if it couldn't be opened.
     */
    void* OpenFile(const char* strFileName, unsigned int flags)
    {
      return XBMC_open_file(m_Handle, m_Callbacks, strFileName, flags);
    }

    /*!
     * @brief Open the file with filename via XBMC's CFile in write mode. Needs to be closed by calling CloseFile() when done.
     * @param strFileName The filename to open.
     * @param bOverWrite True to overwrite, false otherwise.
     * @return A handle for the file, or NULL if it couldn't be opened.
     */
    void* OpenFileForWrite(const char* strFileName, bool bOverWrite)
    {
      return XBMC_open_file_for_write(m_Handle, m_Callbacks, strFileName, bOverWrite);
    }

    /*!
     * @brief Read from an open file.
     * @param file The file handle to read from.
     * @param lpBuf The buffer to store the data in.
     * @param uiBufSize The size of the buffer.
     * @return number of successfully read bytes if any bytes were read and stored in
     *         buffer, zero if no bytes are available to read (end of file was reached)
     *         or undetectable error occur, -1 in case of any explicit error
     */
    ssize_t ReadFile(void* file, void* lpBuf, size_t uiBufSize)
    {
      return XBMC_read_file(m_Handle, m_Callbacks, file, lpBuf, uiBufSize);
    }

    /*!
     * @brief Read a string from an open file.
     * @param file The file handle to read from.
     * @param szLine The buffer to store the data in.
     * @param iLineLength The size of the buffer.
     * @return True when a line was read, false otherwise.
     */
    bool ReadFileString(void* file, char *szLine, int iLineLength)
    {
      return XBMC_read_file_string(m_Handle, m_Callbacks, file, szLine, iLineLength);
    }

    /*!
     * @brief Write to a file opened in write mode.
     * @param file The file handle to write to.
     * @param lpBuf The data to write.
     * @param uiBufSize Size of the data to write.
     * @return number of successfully written bytes if any bytes were written,
     *         zero if no bytes were written and no detectable error occur,
     *         -1 in case of any explicit error
     */
    ssize_t WriteFile(void* file, const void* lpBuf, size_t uiBufSize)
    {
      return XBMC_write_file(m_Handle, m_Callbacks, file, lpBuf, uiBufSize);
    }

    /*!
     * @brief Flush buffered data.
     * @param file The file handle to flush the data for.
     */
    void FlushFile(void* file)
    {
       return XBMC_flush_file(m_Handle, m_Callbacks, file);
    }

    /*!
     * @brief Seek in an open file.
     * @param file The file handle to see in.
     * @param iFilePosition The new position.
     * @param iWhence Seek argument. See stdio.h for possible values.
     * @return The new position.
     */
    int64_t SeekFile(void* file, int64_t iFilePosition, int iWhence)
    {
      return XBMC_seek_file(m_Handle, m_Callbacks, file, iFilePosition, iWhence);
    }

    /*!
     * @brief Truncate a file to the requested size.
     * @param file The file handle to truncate.
     * @param iSize The new max size.
     * @return New size?
     */
    int TruncateFile(void* file, int64_t iSize)
    {
      return XBMC_truncate_file(m_Handle, m_Callbacks, file, iSize);
    }

    /*!
     * @brief The current position in an open file.
     * @param file The file handle to get the position for.
     * @return The requested position.
     */
    int64_t GetFilePosition(void* file)
    {
      return XBMC_get_file_position(m_Handle, m_Callbacks, file);
    }

    /*!
     * @brief Get the file size of an open file.
     * @param file The file to get the size for.
     * @return The requested size.
     */
    int64_t GetFileLength(void* file)
    {
      return XBMC_get_file_length(m_Handle, m_Callbacks, file);
    }

    /*!
    * @brief Get the download speed of an open file if available.
    * @param file The file to get the size for.
    * @return The download speed in seconds.
    */
    double GetFileDownloadSpeed(void* file)
    {
      return XBMC_get_file_download_speed(m_Handle, m_Callbacks, file);
    }

    /*!
     * @brief Close an open file.
     * @param file The file handle to close.
     */
    void CloseFile(void* file)
    {
      return XBMC_close_file(m_Handle, m_Callbacks, file);
    }

    /*!
     * @brief Get the chunk size for an open file.
     * @param file the file handle to get the size for.
     * @return The requested size.
     */
    int GetFileChunkSize(void* file)
    {
      return XBMC_get_file_chunk_size(m_Handle, m_Callbacks, file);
    }

    /*!
     * @brief Check if a file exists.
     * @param strFileName The filename to check.
     * @param bUseCache Check in file cache.
     * @return true if the file exists false otherwise.
     */
    bool FileExists(const char *strFileName, bool bUseCache)
    {
      return XBMC_file_exists(m_Handle, m_Callbacks, strFileName, bUseCache);
    }

    /*!
     * @brief Reads file status.
     * @param strFileName The filename to read the status from.
     * @param buffer The file status is written into this buffer.
     * @return The file status was successfully read.
     */
    int StatFile(const char *strFileName, struct __stat64* buffer)
    {
      return XBMC_stat_file(m_Handle, m_Callbacks, strFileName, buffer);
    }

    /*!
     * @brief Deletes a file.
     * @param strFileName The filename to delete.
     * @return The file was successfully deleted.
     */
    bool DeleteFile(const char *strFileName)
    {
      return XBMC_delete_file(m_Handle, m_Callbacks, strFileName);
    }

    /*!
     * @brief Checks whether a directory can be opened.
     * @param strUrl The URL of the directory to check.
     * @return True when it can be opened, false otherwise.
     */
    bool CanOpenDirectory(const char* strUrl)
    {
      return XBMC_can_open_directory(m_Handle, m_Callbacks, strUrl);
    }

    /*!
     * @brief Creates a directory.
     * @param strPath Path to the directory.
     * @return True when it was created, false otherwise.
     */
    bool CreateDirectory(const char *strPath)
    {
      return XBMC_create_directory(m_Handle, m_Callbacks, strPath);
    }

    /*!
     * @brief Checks if a directory exists.
     * @param strPath Path to the directory.
     * @return True when it exists, false otherwise.
     */
    bool DirectoryExists(const char *strPath)
    {
      return XBMC_directory_exists(m_Handle, m_Callbacks, strPath);
    }

    /*!
     * @brief Removes a directory.
     * @param strPath Path to the directory.
     * @return True when it was removed, false otherwise.
     */
    bool RemoveDirectory(const char *strPath)
    {
      return XBMC_remove_directory(m_Handle, m_Callbacks, strPath);
    }

    /*!
     * @brief Lists a directory.
     * @param strPath Path to the directory.
     * @param mask File mask
     * @param items The directory entries
     * @param num_items Number of entries in directory
     * @return True if listing was successful, false otherwise.
     */
    bool GetDirectory(const char *strPath, const char* mask, VFSDirEntry** items, unsigned int* num_items)
    {
      return XBMC_get_directory(m_Handle, m_Callbacks, strPath, mask, items, num_items);
    }

    /*!
     * @brief Free a directory list
     * @param items The directory entries
     * @param num_items Number of entries in directory
     */
    void FreeDirectory(VFSDirEntry* items, unsigned int num_items)
    {
      return XBMC_free_directory(m_Handle, m_Callbacks, items, num_items);
    }

    /*!
    * @brief Create a Curl representation
    * @param strURL the URL of the Type.
    */
    void* CURLCreate(const char* strURL)
    {
      return XBMC_curl_create(m_Handle, m_Callbacks, strURL);
    }

    /*!
    * @brief Adds options to the curl file created with CURLCeate
    * @param file file pointer to the file returned by CURLCeate
    * @param type option type to set
    * @param name name of the option
    * @param value value of the option
    */
    bool CURLAddOption(void* file, XFILE::CURLOPTIONTYPE type, const char* name, const char * value)
    {
      return XBMC_curl_add_option(m_Handle, m_Callbacks, file, type, name, value);
    }

    /*!
    * @brief Opens the curl file created with CURLCeate
    * @param file file pointer to the file returned by CURLCeate
    * @param flags one or more bitwise or combinded flags form XFILE
    */
    bool CURLOpen(void* file, unsigned int flags)
    {
      return XBMC_curl_open(m_Handle, m_Callbacks, file, flags);
    }

  protected:
    void* (*XBMC_register_me)(void *HANDLE);
    void (*XBMC_unregister_me)(void *HANDLE, void* CB);
    void (*XBMC_log)(void *HANDLE, void* CB, const addon_log_t loglevel, const char *msg);
    bool (*XBMC_get_setting)(void *HANDLE, void* CB, const char* settingName, void *settingValue);
    void (*XBMC_queue_notification)(void *HANDLE, void* CB, const queue_msg_t type, const char *msg);
    bool (*XBMC_wake_on_lan)(void *HANDLE, void* CB, const char* mac);
    char* (*XBMC_unknown_to_utf8)(void *HANDLE, void* CB, const char* str);
    char* (*XBMC_get_localized_string)(void *HANDLE, void* CB, int dwCode);
    char* (*XBMC_get_dvd_menu_language)(void *HANDLE, void* CB);
    void (*XBMC_free_string)(void *HANDLE, void* CB, char* str);
    void* (*XBMC_open_file)(void *HANDLE, void* CB, const char* strFileName, unsigned int flags);
    void* (*XBMC_open_file_for_write)(void *HANDLE, void* CB, const char* strFileName, bool bOverWrite);
    ssize_t (*XBMC_read_file)(void *HANDLE, void* CB, void* file, void* lpBuf, size_t uiBufSize);
    bool (*XBMC_read_file_string)(void *HANDLE, void* CB, void* file, char *szLine, int iLineLength);
    ssize_t(*XBMC_write_file)(void *HANDLE, void* CB, void* file, const void* lpBuf, size_t uiBufSize);
    void (*XBMC_flush_file)(void *HANDLE, void* CB, void* file);
    int64_t (*XBMC_seek_file)(void *HANDLE, void* CB, void* file, int64_t iFilePosition, int iWhence);
    int (*XBMC_truncate_file)(void *HANDLE, void* CB, void* file, int64_t iSize);
    int64_t (*XBMC_get_file_position)(void *HANDLE, void* CB, void* file);
    int64_t (*XBMC_get_file_length)(void *HANDLE, void* CB, void* file);
    double(*XBMC_get_file_download_speed)(void *HANDLE, void* CB, void* file);
    void (*XBMC_close_file)(void *HANDLE, void* CB, void* file);
    int (*XBMC_get_file_chunk_size)(void *HANDLE, void* CB, void* file);
    bool (*XBMC_file_exists)(void *HANDLE, void* CB, const char *strFileName, bool bUseCache);
    int (*XBMC_stat_file)(void *HANDLE, void* CB, const char *strFileName, struct __stat64* buffer);
    bool (*XBMC_delete_file)(void *HANDLE, void* CB, const char *strFileName);
    bool (*XBMC_can_open_directory)(void *HANDLE, void* CB, const char* strURL);
    bool (*XBMC_create_directory)(void *HANDLE, void* CB, const char* strPath);
    bool (*XBMC_directory_exists)(void *HANDLE, void* CB, const char* strPath);
    bool (*XBMC_remove_directory)(void *HANDLE, void* CB, const char* strPath);
    bool (*XBMC_get_directory)(void *HANDLE, void* CB, const char* strPath, const char* mask, VFSDirEntry** items, unsigned int* num_items);
    void (*XBMC_free_directory)(void *HANDLE, void* CB, VFSDirEntry* items, unsigned int num_items);
    void* (*XBMC_curl_create)(void *HANDLE, void* CB, const char* strURL);
    bool (*XBMC_curl_add_option)(void *HANDLE, void* CB, void *file, XFILE::CURLOPTIONTYPE type, const char* name, const char *value);
    bool (*XBMC_curl_open)(void *m_Handle, void *m_Callbacks, void *file, unsigned int flags);

  private:
    void *m_libXBMC_addon;
    void *m_Handle;
    void *m_Callbacks;
    struct cb_array
    {
      const char* libPath;
    };
  };
};
