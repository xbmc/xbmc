#pragma once
/*
 *      Copyright (C) 2005-2010 Team XBMC
 *      http://www.xbmc.org
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

#ifdef _WIN32                   // windows
#include "dlfcn-win32.h"
#define ADDON_DLL               "\\library.xbmc.addon\\libXBMC_addon" ADDON_HELPER_EXT
#define ADDON_HELPER_EXT        ".dll"
#else
#if defined(__APPLE__)          // osx
#if defined(__POWERPC__)
#define ADDON_HELPER_ARCH       "powerpc-osx"
#elif defined(__arm__)
#define ADDON_HELPER_ARCH       "arm-osx"
#elif defined(__x86_64__)
#define ADDON_HELPER_ARCH       "x86-osx"
#else
#define ADDON_HELPER_ARCH       "x86-osx"
#endif
#else                           // linux
#if defined(__x86_64__)
#define ADDON_HELPER_ARCH       "x86_64-linux"
#elif defined(_POWERPC)
#define ADDON_HELPER_ARCH       "powerpc-linux"
#elif defined(_POWERPC64)
#define ADDON_HELPER_ARCH       "powerpc64-linux"
#elif defined(__ARMEL__)
#define ADDON_HELPER_ARCH       "arm"
#elif defined(_MIPSEL)
#define ADDON_HELPER_ARCH       "mipsel-linux"
#else
#define ADDON_HELPER_ARCH       "i486-linux"
#endif
#endif
#include <dlfcn.h>              // linux+osx
#define ADDON_HELPER_EXT        ".so"
#define ADDON_DLL "/library.xbmc.addon/libXBMC_addon-" ADDON_HELPER_ARCH ADDON_HELPER_EXT
#endif

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
        XBMC_unregister_me();
        dlclose(m_libXBMC_addon);
      }
    }

    bool RegisterMe(void *Handle)
    {
      m_Handle = Handle;

      std::string libBasePath;
      libBasePath  = ((cb_array*)m_Handle)->libPath;
      libBasePath += ADDON_DLL;

      m_libXBMC_addon = dlopen(libBasePath.c_str(), RTLD_LAZY);
      if (m_libXBMC_addon == NULL)
      {
        fprintf(stderr, "Unable to load %s\n", dlerror());
        return false;
      }

      XBMC_register_me   = (int (*)(void *HANDLE))
        dlsym(m_libXBMC_addon, "XBMC_register_me");
      if (XBMC_register_me == NULL)   { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      XBMC_unregister_me = (void (*)())
        dlsym(m_libXBMC_addon, "XBMC_unregister_me");
      if (XBMC_unregister_me == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      Log                = (void (*)(const addon_log_t loglevel, const char *format, ... ))
        dlsym(m_libXBMC_addon, "XBMC_log");
      if (Log == NULL)                { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      GetSetting         = (bool (*)(const char* settingName, void *settingValue))
        dlsym(m_libXBMC_addon, "XBMC_get_setting");
      if (GetSetting == NULL)         { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      QueueNotification  = (void (*)(const queue_msg_t loglevel, const char *format, ... ))
        dlsym(m_libXBMC_addon, "XBMC_queue_notification");
      if (QueueNotification == NULL)  { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      UnknownToUTF8      = (void (*)(std::string &str))
        dlsym(m_libXBMC_addon, "XBMC_unknown_to_utf8");
      if (UnknownToUTF8 == NULL)      { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      GetLocalizedString = (const char* (*)(int dwCode))
        dlsym(m_libXBMC_addon, "XBMC_get_localized_string");
      if (GetLocalizedString == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      GetDVDMenuLanguage = (const char* (*)())
        dlsym(m_libXBMC_addon, "XBMC_get_dvd_menu_language");
      if (GetDVDMenuLanguage == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      OpenFile = (void* (*)(const char* strFileName, unsigned int flags))
        dlsym(m_libXBMC_addon, "XBMC_open_file");
      if (OpenFile == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      OpenFileForWrite = (void* (*)(const char* strFileName, bool bOverWrite))
        dlsym(m_libXBMC_addon, "XBMC_open_file_for_write");
      if (OpenFileForWrite == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      ReadFile = (unsigned int (*)(void* file, void* lpBuf, int64_t uiBufSize))
        dlsym(m_libXBMC_addon, "XBMC_read_file");
      if (ReadFile == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      ReadFileString = (bool (*)(void* file, char *szLine, int iLineLength))
        dlsym(m_libXBMC_addon, "XBMC_read_file_string");
      if (ReadFileString == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      WriteFile = (int (*)(void* file, const void* lpBuf, int64_t uiBufSize))
        dlsym(m_libXBMC_addon, "XBMC_write_file");
      if (WriteFile == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      FlushFile = (void (*)(void* file))
        dlsym(m_libXBMC_addon, "XBMC_flush_file");
      if (FlushFile == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      SeekFile = (int64_t (*)(void* file, int64_t iFilePosition, int iWhence))
        dlsym(m_libXBMC_addon, "XBMC_seek_file");
      if (SeekFile == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      TruncateFile = (int (*)(void* file, int64_t iSize))
        dlsym(m_libXBMC_addon, "XBMC_truncate_file");
      if (TruncateFile == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      GetFilePosition = (int64_t (*)(void* file))
        dlsym(m_libXBMC_addon, "XBMC_get_file_position");
      if (GetFilePosition == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      GetFileLength = (int64_t (*)(void* file))
        dlsym(m_libXBMC_addon, "XBMC_get_file_length");
      if (GetFileLength == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      CloseFile = (void (*)(void* file))
        dlsym(m_libXBMC_addon, "XBMC_close_file");
      if (CloseFile == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      GetFileChunkSize = (int (*)(void* file))
        dlsym(m_libXBMC_addon, "XBMC_get_file_chunk_size");
      if (GetFileChunkSize == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      CanOpenDirectory = (bool (*)(const char* strURL))
        dlsym(m_libXBMC_addon, "XBMC_can_open_directory");
      if (CanOpenDirectory == NULL) { fprintf(stderr, "Unable to assign function %s\n", dlerror()); return false; }

      return XBMC_register_me(m_Handle) > 0;
    }

    /*!
     * @brief Add a message to XBMC's log.
     * @param loglevel The log level of the message.
     * @param format The format of the message to pass to XBMC.
     */
    void (*Log)(const addon_log_t loglevel, const char *format, ... );

    /*!
     * @brief Get a settings value for this add-on.
     * @param settingName The name of the setting to get.
     * @param settingValue The value.
     * @return True if the settings was fetched successfully, false otherwise.
     */
    bool (*GetSetting)(const char* settingName, void *settingValue);

    /*!
     * @brief Queue a notification in the GUI.
     * @param type The message type.
     * @param format The format of the message to pass to display in XBMC.
     */
    void (*QueueNotification)(const queue_msg_t type, const char *format, ... );

    /*!
     * @brief Translate a string with an unknown encoding to UTF8.
     * @param sourceDest The string to translate.
     */
    void (*UnknownToUTF8)(std::string &str);

    /*!
     * @brief Get a localised message.
     * @param dwCode The code of the message to get.
     * @return The message. Needs to be freed when done.
     */
    const char* (*GetLocalizedString)(int dwCode);


    /*!
     * @brief Get the DVD menu language.
     * @return The language. Needs to be freed when done.
     */
    const char* (*GetDVDMenuLanguage)();

    /*!
     * @brief Open the file with filename via XBMC's CFile. Needs to be closed by calling CloseFile() when done.
     * @param strFileName The filename to open.
     * @param flags The flags to pass. Documented in XBMC's File.h
     * @return A handle for the file, or NULL if it couldn't be opened.
     */
    void* (*OpenFile)(const char* strFileName, unsigned int flags);

    /*!
     * @brief Open the file with filename via XBMC's CFile in write mode. Needs to be closed by calling CloseFile() when done.
     * @param strFileName The filename to open.
     * @param bOverWrite True to overwrite, false otherwise.
     * @return A handle for the file, or NULL if it couldn't be opened.
     */
    void* (*OpenFileForWrite)(const char* strFileName, bool bOverWrite);

    /*!
     * @brief Read from an open file.
     * @param file The file handle to read from.
     * @param lpBuf The buffer to store the data in.
     * @param uiBufSize The size of the buffer.
     * @return Number of bytes read.
     */
    unsigned int (*ReadFile)(void* file, void* lpBuf, int64_t uiBufSize);

    /*!
     * @brief Read a string from an open file.
     * @param file The file handle to read from.
     * @param szLine The buffer to store the data in.
     * @param iLineLength The size of the buffer.
     * @return True when a line was read, false otherwise.
     */
    bool (*ReadFileString)(void* file, char *szLine, int iLineLength);

    /*!
     * @brief Write to a file opened in write mode.
     * @param file The file handle to write to.
     * @param lpBuf The data to write.
     * @param uiBufSize Size of the data to write.
     * @return The number of bytes read.
     */
    int (*WriteFile)(void* file, const void* lpBuf, int64_t uiBufSize);

    /*!
     * @brief Flush buffered data.
     * @param file The file handle to flush the data for.
     */
    void (*FlushFile)(void* file);

    /*!
     * @brief Seek in an open file.
     * @param file The file handle to see in.
     * @param iFilePosition The new position.
     * @param iWhence Seek argument. See stdio.h for possible values.
     * @return The new position.
     */
    int64_t (*SeekFile)(void* file, int64_t iFilePosition, int iWhence);

    /*!
     * @brief Truncate a file to the requested size.
     * @param file The file handle to truncate.
     * @param iSize The new max size.
     * @return New size?
     */
    int (*TruncateFile)(void* file, int64_t iSize);

    /*!
     * @brief The current position in an open file.
     * @param file The file handle to get the position for.
     * @return The requested position.
     */
    int64_t (*GetFilePosition)(void* file);

    /*!
     * @brief Get the file size of an open file.
     * @param file The file to get the size for.
     * @return The requested size.
     */
    int64_t (*GetFileLength)(void* file);

    /*!
     * @brief Close an open file.
     * @param file The file handle to close.
     */
    void (*CloseFile)(void* file);

    /*!
     * @brief Get the chunk size for an open file.
     * @param file the file handle to get the size for.
     * @return The requested size.
     */
    int (*GetFileChunkSize)(void* file);

    /*!
     * @brief Checks whether a directory can be opened.
     * @param strUrl The URL of the directory to check.
     * @return True when it can be opened, false otherwise.
     */
    bool (*CanOpenDirectory)(const char* strUrl);

  protected:
    int (*XBMC_register_me)(void *HANDLE);
    void (*XBMC_unregister_me)();

  private:
    void *m_libXBMC_addon;
    void *m_Handle;
    struct cb_array
    {
      const char* libPath;
    };
  };
};
