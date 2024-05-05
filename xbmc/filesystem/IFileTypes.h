/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>

namespace XFILE
{

/* indicate that caller can handle truncated reads, where function returns before entire buffer has been filled */
  static const unsigned int READ_TRUNCATED = 0x01;

/* indicate that that caller support read in the minimum defined chunk size, this disables internal cache then */
  static const unsigned int READ_CHUNKED = 0x02;

/* use cache to access this file */
  static const unsigned int READ_CACHED = 0x04;

/* open without caching. regardless to file type. */
  static const unsigned int READ_NO_CACHE = 0x08;

/* calculate bitrate for file while reading */
  static const unsigned int READ_BITRATE = 0x10;

/* indicate to the caller we will seek between multiple streams in the file frequently */
  static const unsigned int READ_MULTI_STREAM = 0x20;

/* indicate to the caller file is audio and/or video (and e.g. may grow) */
  static const unsigned int READ_AUDIO_VIDEO = 0x40;

/* indicate that caller will do write operations before reading  */
  static const unsigned int READ_AFTER_WRITE = 0x80;

/* indicate that caller want to reopen a file if its already open  */
  static const unsigned int READ_REOPEN = 0x100;

/* indicate that caller want open a file without intermediate buffer regardless to file type */
  static const unsigned int READ_NO_BUFFER = 0x200;

struct SNativeIoControl
{
  unsigned long int   request;
  void*               param;
};

struct SCacheStatus
{
  uint64_t maxforward; /**< forward cache max capacity in bytes */
  uint64_t forward; /**< number of bytes cached forward of current position */
  uint32_t maxrate; /**< maximum allowed read(fill) rate (bytes/second) */
  uint32_t currate; /**< average read rate (bytes/second) since last position change */
  uint32_t lowrate; /**< low speed read rate (bytes/second) (if any, else 0) */
};

enum CACHE_BUFFER_MODES
{
  CACHE_BUFFER_MODE_INTERNET = 0,
  CACHE_BUFFER_MODE_ALL = 1,
  CACHE_BUFFER_MODE_TRUE_INTERNET = 2,
  CACHE_BUFFER_MODE_NONE = 3,
  CACHE_BUFFER_MODE_NETWORK = 4,
};

typedef enum {
  IOCTRL_NATIVE        = 1,  /**< SNativeIoControl structure, containing what should be passed to native ioctrl */
  IOCTRL_SEEK_POSSIBLE = 2,  /**< return 0 if known not to work, 1 if it should work */
  IOCTRL_CACHE_STATUS  = 3,  /**< SCacheStatus structure */
  IOCTRL_CACHE_SETRATE = 4,  /**< unsigned int with speed limit for caching in bytes per second */
  IOCTRL_SET_CACHE     = 8,  /**< CFileCache */
  IOCTRL_SET_RETRY     = 16, /**< Enable/disable retry within the protocol handler (if supported) */
} EIoControl;

enum CURLOPTIONTYPE
{
  CURL_OPTION_OPTION,     /**< Set a general option   */
  CURL_OPTION_PROTOCOL,   /**< Set a protocol option (see below)  */
  CURL_OPTION_CREDENTIALS,/**< Set User and password  */
  CURL_OPTION_HEADER      /**< Add a Header           */
};

/**
 * The following names for CURL_OPTION_PROTOCOL are possible:
 *
 * accept-charset: Set the "accept-charset" header
 * acceptencoding or encoding: Set the "accept-encoding" header
 * active-remote: Set the "active-remote" header
 * auth: Set the authentication method. Possible values: any, anysafe, digest, ntlm
 * connection-timeout: Set the connection timeout in seconds
 * cookie: Set the "cookie" header
 * customrequest: Set a custom HTTP request like DELETE
 * noshout: Set to true if kodi detects a stream as shoutcast by mistake.
 * postdata: Set the post body (value needs to be base64 encoded). (Implicitly sets the request to POST)
 * referer: Set the "referer" header
 * user-agent: Set the "user-agent" header
 * seekable: Set the stream seekable. 1: enable, 0: disable
 * sslcipherlist: Set list of accepted SSL ciphers.
 */

enum FileProperty
{
  FILE_PROPERTY_RESPONSE_PROTOCOL,          /**< Get response protocol line  */
  FILE_PROPERTY_RESPONSE_HEADER,            /**< Get response Header value  */
  FILE_PROPERTY_CONTENT_TYPE,               /**< Get file content-type  */
  FILE_PROPERTY_CONTENT_CHARSET,            /**< Get file content charset  */
  FILE_PROPERTY_MIME_TYPE,                  /**< Get file mime type  */
  FILE_PROPERTY_EFFECTIVE_URL               /**< Get effective URL for redirected streams  */
};

class IFileCallback
{
public:
  virtual bool OnFileCallback(void* pContext, int ipercent, float avgSpeed) = 0;
  virtual ~IFileCallback() = default;
};
}
