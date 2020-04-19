/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstddef> // for size_t
#include <cstdint> // for int64_t
#include <map>
#include <memory>
#include <string> // for string
#include <system_error>

#include <sys/select.h> // for fd_set

/* put types of curl in namespace to avoid namespace pollution */
namespace KODI
{
namespace PLATFORM
{

enum class CurlCode
{
  OK = 0,
  UNSUPPORTED_PROTOCOL,
  FAILED_INIT,
  URL_MALFORMAT,
  NOT_BUILT_IN,
  COULDNT_RESOLVE_PROXY,
  COULDNT_RESOLVE_HOST,
  COULDNT_CONNECT,
  WEIRD_SERVER_REPLY,
  REMOTE_ACCESS_DENIED,
  FTP_ACCEPT_FAILED,
  FTP_WEIRD_PASS_REPLY,
  FTP_ACCEPT_TIMEOUT,
  FTP_WEIRD_PASV_REPLY,
  FTP_WEIRD_227_FORMAT,
  FTP_CANT_GET_HOST,
  HTTP2,
  FTP_COULDNT_SET_TYPE,
  PARTIAL_FILE,
  FTP_COULDNT_RETR_FILE,
  OBSOLETE20,
  QUOTE_ERROR,
  HTTP_RETURNED_ERROR,
  WRITE_ERROR,
  OBSOLETE24,
  UPLOAD_FAILED,
  READ_ERROR,
  OUT_OF_MEMORY,
  OPERATION_TIMEDOUT,
  OBSOLETE29,
  FTP_PORT_FAILED,
  FTP_COULDNT_USE_REST,
  OBSOLETE32,
  RANGE_ERROR,
  HTTP_POST_ERROR,
  SSL_CONNECT_ERROR,
  BAD_DOWNLOAD_RESUME,
  FILE_COULDNT_READ_FILE,
  LDAP_CANNOT_BIND,
  LDAP_SEARCH_FAILED,
  OBSOLETE40,
  FUNCTION_NOT_FOUND,
  ABORTED_BY_CALLBACK,
  BAD_FUNCTION_ARGUMENT,
  OBSOLETE44,
  INTERFACE_FAILED,
  OBSOLETE46,
  TOO_MANY_REDIRECTS,
  UNKNOWN_OPTION,
  TELNET_OPTION_SYNTAX,
  OBSOLETE50,
  PEER_FAILED_VERIFICATION,
  GOT_NOTHING,
  SSL_ENGINE_NOTFOUND,
  SSL_ENGINE_SETFAILED,
  SEND_ERROR,
  RECV_ERROR,
  OBSOLETE57,
  SSL_CERTPROBLEM,
  SSL_CIPHER,
  SSL_CACERT,
  BAD_CONTENT_ENCODING,
  LDAP_INVALID_URL,
  FILESIZE_EXCEEDED,
  USE_SSL_FAILED,
  SEND_FAIL_REWIND,
  SSL_ENGINE_INITFAILED,
  LOGIN_DENIED,
  TFTP_NOTFOUND,
  TFTP_PERM,
  REMOTE_DISK_FULL,
  TFTP_ILLEGAL,
  TFTP_UNKNOWNID,
  REMOTE_FILE_EXISTS,
  TFTP_NOSUCHUSER,
  CONV_FAILED,
  CONV_REQD,
  SSL_CACERT_BADFILE,
  REMOTE_FILE_NOT_FOUND,
  SSH,
  SSL_SHUTDOWN_FAILED,
  AGAIN,
  SSL_CRL_BADFILE,
  SSL_ISSUER_ERROR,
  FTP_PRET_FAILED,
  RTSP_CSEQ_ERROR,
  RTSP_SESSION_ERROR,
  FTP_BAD_FILE_LIST,
  CHUNK_FAILED,
  NO_CONNECTION_AVAILABLE,
  SSL_PINNEDPUBKEYNOTMATCH,
  SSL_INVALIDCERTSTATUS,
  HTTP2_STREAM,
};

enum class CurlMCode
{
  CALL_MULTI_PERFORM = -1,
  OK,
  BAD_HANDLE,
  BAD_EASY_HANDLE,
  OUT_OF_MEMORY,
  INTERNAL_ERROR,
  BAD_SOCKET,
  UNKNOWN_OPTION,
  ADDED_ALREADY,
};

enum class CurlPause
{
  RECV = 1,
  SEND = 4,
  ALL = 5
};

enum class CurlProxy
{
  HTTP = 0,
  HTTP_1_0 = 1,
  HTTPS = 2,
  SOCKS4 = 4,
  SOCKS5 = 5,
  SOCKS4A = 6,
  SOCKS5_HOSTNAME = 7
};

enum class CurlFtpMethod
{
  DEFAULT, /* let libcurl pick */
  MULTICWD, /* single CWD operation for each path part */
  NOCWD, /* no CWD at all */
  SINGLECWD, /* one CWD to full dir, then work on file */
};

enum class CurlFtpAuth
{
  DEFAULT, /* let libcurl decide */
  SSL, /* use "AUTH SSL" */
  TLS, /* use "AUTH TLS" */
};

enum class CurlHttpAuth : unsigned long
{
  NONE = 0u,
  BASIC = 1u,
  DIGEST = 2u,
  NEGOTIATE = 4u,
  NTLM = 8u,
  DIGEST_IE = 16u,
  NTLM_WB = 32u,
  ONLY = 1u << 31,
  ANY = ~16u,
  ANYSAFE = ~(1u | 16u)
};

enum class CurlHttpVersion
{
  NONE = 0,
  VERSION_1_0,
  VERSION_1_1,
  VERSION_2_0,
  VERSION_2TLS,
  VERSION_2_PRIOR_KNOWLEDGE
};

std::error_code make_error_code(CurlCode);
std::error_code make_error_code(CurlMCode);

class CCurl
{
public:
  using ProgressCallback = int(void*, long long, long long, long long, long long);
  using ReadWriteCallback = size_t(char*, size_t, size_t, void*);
  using HeaderCallback = size_t(void*, size_t, size_t, void*);

  CCurl();
  ~CCurl();

  std::error_code EasyPerform();
  std::error_code EasyPause(CurlPause type);
  std::error_code EasyResume();
  void EasyReset();

  std::error_code MultiPerform(int& running_handles);
  std::error_code MultiFdSet(fd_set* read_fd_set,
                             fd_set* write_fd_set,
                             fd_set* exc_fd_set,
                             int* max_fd);
  void UseMulti();
  void RemoveMulti();
  bool IsMulti();

  int64_t GetContentLength(std::error_code& ec);
  std::string GetContentType(std::error_code& ec);
  long GetFileTime(std::error_code& ec);
  long GetResponseCode(std::error_code& ec);
  std::string GetEffectiveUrl(std::error_code& ec);
  std::string GetRedirectURL(std::error_code& ec);
  std::string GetCookies(std::error_code& ec);
  double GetDownloadSpeed(std::error_code& ec);
  long GetTimeout(std::error_code& ec);
  std::error_code GetMultiMessage(int& messages);

  std::error_code DisableBody();
  std::error_code EnableBody();
  std::error_code DisableFtpEPSV();
  std::error_code DisableSslVerifypeer();
  std::error_code DisableSignals();
  std::error_code DisableTransferText();
  std::error_code DisableIPV6();
  std::error_code EnableFileTime();
  std::error_code EnableUpload();
  std::error_code EnableCookies();
  std::error_code FollowRedirects(int max);
  std::error_code WriteData(void* data);
  std::error_code FlushCookies();
  std::error_code FailOnError();
  std::error_code IgnoreContentLength();

  std::error_code SetTimeout(int timeout);
  std::error_code SetConnectTimeout(int timeout);
  std::error_code SetFtpFileMethod(CurlFtpMethod method);
  std::error_code SetProgressCallback(ProgressCallback cb);
  std::error_code SetCallbacks(void* state,
                               ReadWriteCallback write,
                               ReadWriteCallback read,
                               HeaderCallback header);
  std::error_code SetUsernamePassword(const std::string& user, const std::string& pass);
  std::error_code SetCookies(const std::string& cookies);
  std::error_code SetAlias(const std::string& alias);
  std::error_code SetUrl(const std::string& url);
  std::error_code SetPostData(const std::string& data);
  std::error_code SetOrClearReferer(const std::string& referer);
  std::error_code SetFtpAuth(CurlFtpAuth auth);
  std::error_code SetHttpAuth(CurlHttpAuth auth);
  std::error_code SetHttpVersion(CurlHttpVersion version);
  std::error_code SetFtpPort(const std::string& port);
  std::error_code SetSkipFtpPassiveIp(bool skip);
  std::error_code SetAcceptEncoding(const std::string& encoding);
  std::error_code SetUserAgent(const std::string& useragent);
  std::error_code SetLowSpeedLimit(int bps, int time);
  std::error_code SetCipherList(const std::string& ciphers);
  std::error_code SetProxyOptions(CurlProxy proxy,
                                  const std::string& host,
                                  int port,
                                  const std::string& user,
                                  const std::string& password);
  std::error_code SetCustomRequest(const std::string& request);
  std::error_code SetResumeRange(const char* start, int64_t end);
  std::error_code SetHeaders(const std::map<std::string, std::string>& headers);
  std::error_code AddHeader(const std::string& header);

private:
  struct SSession;
  std::unique_ptr<SSession> m_session;
};

} // namespace PLATFORM
} // namespace KODI

// This is a legal usage of sticking something into namespace std
// These should precede the include of system_error
namespace std
{
template<>
struct is_error_code_enum<KODI::PLATFORM::CurlCode> : true_type
{
};

template<>
struct is_error_code_enum<KODI::PLATFORM::CurlMCode> : true_type
{
};
} // namespace std
