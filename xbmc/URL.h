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

#include "utils/StdString.h"
#include "utils/UrlOptions.h"

#ifdef TARGET_WINDOWS
#undef SetPort // WIN32INCLUDES this is defined as SetPortA in WinSpool.h which is being included _somewhere_
#endif

class CURL
{
public:
  CURL(const std::string& strURL);
  CURL(const std::wstring& strURL);
  CURL();
  virtual ~CURL(void);

  void Reset();
  void Parse(const std::string& strURL);
  void SetFileName(const std::string& strFileName);
  void SetHostName(const std::string& strHostName);
  void SetUserName(const std::string& strUserName);
  void SetPassword(const std::string& strPassword);
  void SetProtocol(const std::string& strProtocol);
  void SetOptions(const std::string& strOptions);
  void SetProtocolOptions(const std::string& strOptions);
  void SetPort(int port);

  bool HasPort() const;

  int GetPort() const;
  const std::string& GetHostName() const;
  const std::string& GetDomain() const;
  const std::string& GetUserName() const;
  const std::string& GetPassWord() const;
  const std::string& GetFileName() const;
  const std::string& GetProtocol() const;
  const std::string GetTranslatedProtocol() const;
  const std::string& GetFileType() const;
  const std::string& GetShareName() const;
  const std::string& GetOptions() const;
  const std::string& GetProtocolOptions() const;
  const std::string GetFileNameWithoutPath() const; /* return the filename excluding path */

  char GetDirectorySeparator() const;

  std::string Get() const;
  std::string GetWithoutUserDetails(bool redact = false) const;
  std::string GetWithoutFilename() const;
  std::string GetRedacted() const;
  static std::string GetRedacted(const std::string& path);
  bool IsLocal() const;
  bool IsLocalHost() const;
  static bool IsFileOnly(const std::string &url); ///< return true if there are no directories in the url.
  static bool IsFullPath(const std::string &url); ///< return true if the url includes the full path
  static std::string Decode(const std::string& strURLData);
  static std::string Encode(const std::string& strURLData);
  static std::string TranslateProtocol(const std::string& prot);

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
  int m_iPort;
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
