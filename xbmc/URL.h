/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/UrlOptions.h"

#include <stdlib.h>
#include <string>
#include <string_view>

#ifdef TARGET_WINDOWS
#undef SetPort // WIN32INCLUDES this is defined as SetPortA in WinSpool.h which is being included _somewhere_
#endif

class CURL
{
public:
  explicit CURL(std::string strURL) { Parse(std::move(strURL)); }

  CURL() = default;
  virtual ~CURL(void);

  // explicit equals operator for std::string comparison
  bool operator==(const std::string &url) const { return Get() == url; }

  void Reset();
  void Parse(std::string strURL);
  void SetFileName(std::string strFileName);
  void SetHostName(std::string strHostName) { m_strHostName = std::move(strHostName); }

  void SetUserName(std::string strUserName) { m_strUserName = std::move(strUserName); }

  void SetDomain(std::string strDomain) { m_strDomain = std::move(strDomain); }

  void SetPassword(std::string strPassword) { m_strPassword = std::move(strPassword); }

  void SetProtocol(std::string strProtocol);
  void SetOptions(std::string strOptions);
  void SetProtocolOptions(std::string strOptions);
  void SetPort(int port)
  {
    m_iPort = port;
  }

  bool HasPort() const
  {
    return (m_iPort != 0);
  }

  int GetPort() const
  {
    return m_iPort;
  }

  const std::string& GetHostName() const
  {
    return m_strHostName;
  }

  const std::string& GetDomain() const
  {
    return m_strDomain;
  }

  const std::string& GetUserName() const
  {
    return m_strUserName;
  }

  const std::string& GetPassWord() const
  {
    return m_strPassword;
  }

  const std::string& GetFileName() const
  {
    return m_strFileName;
  }

  const std::string& GetProtocol() const
  {
    return m_strProtocol;
  }

  std::string GetTranslatedProtocol() const;

  const std::string& GetFileType() const
  {
    return m_strFileType;
  }

  const std::string& GetShareName() const
  {
      return m_strShareName;
  }

  const std::string& GetOptions() const
  {
    return m_strOptions;
  }

  const std::string& GetProtocolOptions() const
  {
    return m_strProtocolOptions;
  }

  std::string GetFileNameWithoutPath() const; /* return the filename excluding path */

  char GetDirectorySeparator() const;

  std::string Get() const;
  std::string GetWithoutOptions() const;
  std::string GetWithoutUserDetails(bool redact = false) const;
  std::string GetWithoutFilename() const;
  std::string GetRedacted() const;
  static std::string GetRedacted(std::string path);
  bool IsLocal() const;
  bool IsLocalHost() const;
  static bool IsFileOnly(const std::string &url); ///< return true if there are no directories in the url.
  static bool IsFullPath(const std::string &url); ///< return true if the url includes the full path
  static std::string Decode(std::string_view strURLData);
  static std::string Encode(std::string_view strURLData);

  /*! \brief Check whether a URL is a given URL scheme.
   Comparison is case-insensitive as per RFC1738
   \param type a lower-case scheme name, e.g. "smb".
   \return true if the url is of the given scheme, false otherwise.
   */
  bool IsProtocol(const char *type) const
  {
    return IsProtocolEqual(m_strProtocol, type);
  }

  /*! \brief Check whether a URL protocol is a given URL scheme.
   Both parameters MUST be lower-case.  Typically this would be called using
   the result of TranslateProtocol() which enforces this for protocol.
   \param protocol a lower-case scheme name, e.g. "ftp"
   \param type a lower-case scheme name, e.g. "smb".
   \return true if the url is of the given scheme, false otherwise.
   */
  static bool IsProtocolEqual(const std::string& protocol, const char *type);

  /*! \brief Check whether a URL is a given filetype.
   Comparison is effectively case-insensitive as both the parameter
   and m_strFileType are lower-case.
   \param type a lower-case filetype, e.g. "mp3".
   \return true if the url is of the given filetype, false otherwise.
   */
  bool IsFileType(const char *type) const
  {
    return m_strFileType == type;
  }

  void GetOptions(std::map<std::string, std::string> &options) const;
  bool HasOption(const std::string &key) const;
  bool GetOption(const std::string &key, std::string &value) const;
  std::string GetOption(const std::string &key) const;
  void SetOption(const std::string &key, const std::string &value);
  void RemoveOption(const std::string &key);

  void GetProtocolOptions(std::map<std::string, std::string> &options) const;
  bool HasProtocolOption(const std::string &key) const;
  bool GetProtocolOption(const std::string &key, std::string &value) const;
  std::string GetProtocolOption(const std::string &key) const;
  void SetProtocolOption(const std::string &key, const std::string &value);
  void RemoveProtocolOption(const std::string &key);

protected:
  int m_iPort = 0;
  std::string m_strHostName;
  std::string m_strShareName;
  std::string m_strDomain;
  std::string m_strUserName;
  std::string m_strPassword;
  std::string m_strFileName;
  std::string m_strProtocol;
  std::string m_strFileType;
  std::string m_strOptions;
  std::string m_strProtocolOptions;
  CUrlOptions m_options;
  CUrlOptions m_protocolOptions;
};
