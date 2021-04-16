/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#ifndef C_API_FILESYSTEM_H
#define C_API_FILESYSTEM_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#ifdef _WIN32 // windows
#ifndef _SSIZE_T_DEFINED
typedef intptr_t ssize_t;
#define _SSIZE_T_DEFINED
#endif // !_SSIZE_T_DEFINED

// Prevent conflicts with Windows macros where have this names.
#ifdef CreateDirectory
#undef CreateDirectory
#endif // CreateDirectory
#ifdef DeleteFile
#undef DeleteFile
#endif // DeleteFile
#ifdef RemoveDirectory
#undef RemoveDirectory
#endif // RemoveDirectory
#endif // _WIN32

#ifdef TARGET_POSIX // Linux, Mac, FreeBSD
#include <sys/types.h>
#endif // TARGET_POSIX

#ifdef __cplusplus
extern "C"
{
#endif /* __cplusplus */

  //¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  // "C" Definitions, structures and enumerators of filesystem
  //{{{

  //============================================================================
  /// @defgroup cpp_kodi_vfs_Defs_OpenFileFlags enum OpenFileFlags
  /// @ingroup cpp_kodi_vfs_Defs
  /// @brief **Flags to define way how file becomes opened**\n
  /// The values can be used together, e.g. <b>`file.Open("myfile", ADDON_READ_TRUNCATED | ADDON_READ_CHUNKED);`</b>
  ///
  /// Used on @ref kodi::vfs::CFile::OpenFile().
  ///
  ///@{
  typedef enum OpenFileFlags
  {
    /// @brief **0000 0000 0001** :\n
    /// Indicate that caller can handle truncated reads, where function
    /// returns before entire buffer has been filled.
    ADDON_READ_TRUNCATED = 0x01,

    /// @brief **0000 0000 0010** :\n
    /// Indicate that that caller support read in the minimum defined
    /// chunk size, this disables internal cache then.
    ADDON_READ_CHUNKED = 0x02,

    /// @brief **0000 0000 0100** :\n
    /// Use cache to access this file.
    ADDON_READ_CACHED = 0x04,

    /// @brief **0000 0000 1000** :\n
    /// Open without caching. regardless to file type.
    ADDON_READ_NO_CACHE = 0x08,

    /// @brief **0000 0001 0000** :\n
    /// Calcuate bitrate for file while reading.
    ADDON_READ_BITRATE = 0x10,

    /// @brief **0000 0010 0000** :\n
    /// Indicate to the caller we will seek between multiple streams in
    /// the file frequently.
    ADDON_READ_MULTI_STREAM = 0x20,

    /// @brief **0000 0100 0000** :\n
    /// indicate to the caller file is audio and/or video (and e.g. may
    /// grow).
    ADDON_READ_AUDIO_VIDEO = 0x40,

    /// @brief **0000 1000 0000** :\n
    /// Indicate that caller will do write operations before reading.
    ADDON_READ_AFTER_WRITE = 0x80,

    /// @brief **0001 0000 0000** :\n
    /// Indicate that caller want to reopen a file if its already open.
    ADDON_READ_REOPEN = 0x100
  } OpenFileFlags;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_vfs_Defs_CURLOptiontype enum CURLOptiontype
  /// @ingroup cpp_kodi_vfs_Defs
  /// @brief **CURL message types**\n
  /// Used on kodi::vfs::CFile::CURLAddOption().
  ///
  ///@{
  typedef enum CURLOptiontype
  {
    /// @brief Set a general option.
    ADDON_CURL_OPTION_OPTION,

    /// @brief Set a protocol option.\n
    ///\n
    /// The following names for *ADDON_CURL_OPTION_PROTOCOL* are possible:
    /// | Option name                         | Description
    /// |------------------------------------:|:--------------------------------
    /// | <b>`accept-charset`</b>             | Set the "accept-charset" header
    /// | <b>`acceptencoding or encoding`</b> | Set the "accept-encoding" header
    /// | <b>`active-remote`</b>              | Set the "active-remote" header
    /// | <b>`auth`</b>                       | Set the authentication method. Possible values: any, anysafe, digest, ntlm
    /// | <b>`connection-timeout`</b>         | Set the connection timeout in seconds
    /// | <b>`cookie`</b>                     | Set the "cookie" header
    /// | <b>`customrequest`</b>              | Set a custom HTTP request like DELETE
    /// | <b>`noshout`</b>                    | Set to true if kodi detects a stream as shoutcast by mistake.
    /// | <b>`postdata`</b>                   | Set the post body (value needs to be base64 encoded). (Implicitly sets the request to POST)
    /// | <b>`referer`</b>                    | Set the "referer" header
    /// | <b>`user-agent`</b>                 | Set the "user-agent" header
    /// | <b>`seekable`</b>                   | Set the stream seekable. 1: enable, 0: disable
    /// | <b>`sslcipherlist`</b>              | Set list of accepted SSL ciphers.
    ///
    ADDON_CURL_OPTION_PROTOCOL,

    /// @brief Set User and password
    ADDON_CURL_OPTION_CREDENTIALS,

    /// @brief Add a Header
    ADDON_CURL_OPTION_HEADER
  } CURLOptiontype;
  ///@}
  //----------------------------------------------------------------------------

  //============================================================================
  /// @defgroup cpp_kodi_vfs_Defs_FilePropertyTypes enum FilePropertyTypes
  /// @ingroup cpp_kodi_vfs_Defs
  /// @brief **File property types**\n
  /// Mostly to read internet sources.
  ///
  /// Used on kodi::vfs::CFile::GetPropertyValue() and kodi::vfs::CFile::GetPropertyValues().
  ///
  ///@{
  typedef enum FilePropertyTypes
  {
    /// @brief Get protocol response line.
    ADDON_FILE_PROPERTY_RESPONSE_PROTOCOL,
    /// @brief Get a response header.
    ADDON_FILE_PROPERTY_RESPONSE_HEADER,
    /// @brief Get file content type.
    ADDON_FILE_PROPERTY_CONTENT_TYPE,
    /// @brief Get file content charset.
    ADDON_FILE_PROPERTY_CONTENT_CHARSET,
    /// @brief Get file mime type.
    ADDON_FILE_PROPERTY_MIME_TYPE,
    /// @brief Get file effective URL (last one if redirected).
    ADDON_FILE_PROPERTY_EFFECTIVE_URL
  } FilePropertyTypes;
  ///@}
  //----------------------------------------------------------------------------

  //}}}

  //¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
  // "C" Internal interface tables for intercommunications between addon and kodi
  //{{{

  struct KODI_HTTP_HEADER
  {
    void* handle;

    char* (*get_value)(void* kodiBase, void* handle, const char* param);
    char** (*get_values)(void* kodiBase, void* handle, const char* param, int* length);
    char* (*get_header)(void* kodiBase, void* handle);
    char* (*get_mime_type)(void* kodiBase, void* handle);
    char* (*get_charset)(void* kodiBase, void* handle);
    char* (*get_proto_line)(void* kodiBase, void* handle);
  };

  struct STAT_STRUCTURE
  {
    /// ID of device containing file
    uint32_t deviceId;
    /// Total size, in bytes
    uint64_t size;
    /// Time of last access
    time_t accessTime;
    /// Time of last modification
    time_t modificationTime;
    /// Time of last status change
    time_t statusTime;
    /// The stat url is a directory
    bool isDirectory;
    /// The stat url is a symbolic link
    bool isSymLink;
    /// The stat url is block special
    bool isBlock;
    /// The stat url is character special
    bool isCharacter;
    /// The stat url is FIFO special
    bool isFifo;
    /// The stat url is regular
    bool isRegular;
    /// The stat url is socket
    bool isSocket;
    /// The file serial number, which distinguishes this file from all other files on the same
    /// device.
    uint64_t fileSerialNumber;
  };

  struct VFS_CACHE_STATUS_DATA
  {
    uint64_t forward;
    unsigned int maxrate;
    unsigned int currate;
    bool lowspeed;
  };

  struct VFSProperty
  {
    char* name;
    char* val;
  };

  struct VFSDirEntry
  {
    char* label; //!< item label
    char* title; //!< item title
    char* path; //!< item path
    unsigned int num_props; //!< Number of properties attached to item
    struct VFSProperty* properties; //!< Properties
    time_t date_time; //!< file creation date & time
    bool folder; //!< Item is a folder
    uint64_t size; //!< Size of file represented by item
  };

  typedef struct AddonToKodiFuncTable_kodi_filesystem
  {
    bool (*can_open_directory)(void* kodiBase, const char* url);
    bool (*create_directory)(void* kodiBase, const char* path);
    bool (*remove_directory)(void* kodiBase, const char* path);
    bool (*directory_exists)(void* kodiBase, const char* path);
    bool (*get_directory)(void* kodiBase,
                          const char* path,
                          const char* mask,
                          struct VFSDirEntry** items,
                          unsigned int* num_items);
    void (*free_directory)(void* kodiBase, struct VFSDirEntry* items, unsigned int num_items);

    bool (*file_exists)(void* kodiBase, const char* filename, bool useCache);
    bool (*stat_file)(void* kodiBase, const char* filename, struct STAT_STRUCTURE* buffer);
    bool (*delete_file)(void* kodiBase, const char* filename);
    bool (*rename_file)(void* kodiBase, const char* filename, const char* newFileName);
    bool (*copy_file)(void* kodiBase, const char* filename, const char* dest);

    char* (*get_file_md5)(void* kodiBase, const char* filename);
    char* (*get_cache_thumb_name)(void* kodiBase, const char* filename);
    char* (*make_legal_filename)(void* kodiBase, const char* filename);
    char* (*make_legal_path)(void* kodiBase, const char* path);
    char* (*translate_special_protocol)(void* kodiBase, const char* strSource);
    bool (*is_internet_stream)(void* kodiBase, const char* path, bool strictCheck);
    bool (*is_on_lan)(void* kodiBase, const char* path);
    bool (*is_remote)(void* kodiBase, const char* path);
    bool (*is_local)(void* kodiBase, const char* path);
    bool (*is_url)(void* kodiBase, const char* path);
    bool (*get_http_header)(void* kodiBase, const char* url, struct KODI_HTTP_HEADER* headers);
    bool (*get_mime_type)(void* kodiBase, const char* url, char** content, const char* useragent);
    bool (*get_content_type)(void* kodiBase,
                             const char* url,
                             char** content,
                             const char* useragent);
    bool (*get_cookies)(void* kodiBase, const char* url, char** cookies);
    bool (*http_header_create)(void* kodiBase, struct KODI_HTTP_HEADER* headers);
    void (*http_header_free)(void* kodiBase, struct KODI_HTTP_HEADER* headers);

    void* (*open_file)(void* kodiBase, const char* filename, unsigned int flags);
    void* (*open_file_for_write)(void* kodiBase, const char* filename, bool overwrite);
    ssize_t (*read_file)(void* kodiBase, void* file, void* ptr, size_t size);
    bool (*read_file_string)(void* kodiBase, void* file, char* szLine, int iLineLength);
    ssize_t (*write_file)(void* kodiBase, void* file, const void* ptr, size_t size);
    void (*flush_file)(void* kodiBase, void* file);
    int64_t (*seek_file)(void* kodiBase, void* file, int64_t position, int whence);
    int (*truncate_file)(void* kodiBase, void* file, int64_t size);
    int64_t (*get_file_position)(void* kodiBase, void* file);
    int64_t (*get_file_length)(void* kodiBase, void* file);
    double (*get_file_download_speed)(void* kodiBase, void* file);
    void (*close_file)(void* kodiBase, void* file);
    int (*get_file_chunk_size)(void* kodiBase, void* file);
    bool (*io_control_get_seek_possible)(void* kodiBase, void* file);
    bool (*io_control_get_cache_status)(void* kodiBase,
                                        void* file,
                                        struct VFS_CACHE_STATUS_DATA* status);
    bool (*io_control_set_cache_rate)(void* kodiBase, void* file, unsigned int rate);
    bool (*io_control_set_retry)(void* kodiBase, void* file, bool retry);
    char** (*get_property_values)(
        void* kodiBase, void* file, int type, const char* name, int* numValues);

    void* (*curl_create)(void* kodiBase, const char* url);
    bool (*curl_add_option)(
        void* kodiBase, void* file, int type, const char* name, const char* value);
    bool (*curl_open)(void* kodiBase, void* file, unsigned int flags);

    bool (*get_disk_space)(
        void* kodiBase, const char* path, uint64_t* capacity, uint64_t* free, uint64_t* available);
    bool (*remove_directory_recursive)(void* kodiBase, const char* path);
  } AddonToKodiFuncTable_kodi_filesystem;

  //}}}

#ifdef __cplusplus
} /* extern "C" */
#endif /* __cplusplus */

#endif /* !C_API_FILESYSTEM_H */
