/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "utils/URI.h"
#include "utils/UrlOptions.h"

#include <stdlib.h>
#include <string>
#include <string_view>

#ifdef TARGET_WINDOWS
#undef SetPort // WIN32INCLUDES this is defined as SetPortA in WinSpool.h which is being included _somewhere_
#endif

class CURL final
{
public:
  CURL() = default;
  CURL(const CURL&) = default;
  CURL& operator=(const CURL&) = default;
  /*! \brief Transfer all URI/compatibility state, leaving the source Reset(). */
  CURL(CURL&& other);
  CURL& operator=(CURL&& other);
  explicit CURL(std::string strURL);

  bool operator==(const CURL& url) const { return url.Get() == Get(); }
  // explicit equals operator for std::string comparison
  friend bool operator==(const CURL& url, const std::string_view str) { return url.Get() == str; }

  void Reset();
  /*! \brief Interpret Kodi mixed native-path/URI input without a strict validity gate.
   * \note Recognized drive prefixes remain native. Bare notes:2026 is a URI candidate;
   *       use ParseNativePath or ./notes:2026 for native intent. Legacy adapters retain
   *       their normalization; new URI forms preserve spelling, including malformed bodies.
   * \sa https://www.rfc-editor.org/rfc/rfc8089.html#appendix-E.2
   */
  void Parse(std::string strURL);

  /*! \brief Parse explicit RFC 3986 URI input, including one-letter schemes and fragments.
   * \return True for generic validity. A malformed recognized URI remains a lossless
   *         identifier; input without a scheme resets this object and returns false.
   * \warning Failure never authorizes native-file fallback or establishes scheme validity.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3
   */
  bool ParseURI(std::string uri);

  /*! \brief Store an explicitly native filename without URI detection or normalization.
   * \note Clears all URI state; ?, #, +, % and colons remain filename bytes.
   */
  void ParseNativePath(std::string path);

  /*! \brief Whether serialization uses the source-preserving URI representation.
   * \note This is a Kodi storage guarantee, not a generic validity or backend test.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-6.2.2
   */
  bool IsLosslessURI() const { return m_uri.has_value(); }

  /*! \brief Whether Kodi permits filesystem-style operations on this location's path.
   * \note Authority-free magnet:/ and magnet:/member are Kodi extensions. Empty-path
   *       external magnets, data, URN and authority-free unknown schemes have no file
   *       hierarchy. Custom scheme:// locations retain legacy VFS behavior.
   * \sa https://www.rfc-editor.org/rfc/rfc8141.html#section-4.3
   */
  bool SupportsPathOperations() const;

  /*! \brief Whether structural navigation must retain the raw query as resource identity.
   * \note This is Kodi policy; it does not impose form semantics on generic queries.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.4
   */
  bool PreservesQueryIdentity() const;

  /*! \name Raw URI components
   * Owned copies without decoding; safe across object mutation, copying and moving.
   * Legacy adapters expose components of their current serialized spelling.
   * Presence predicates distinguish absent components from present empty components.
   * Native filenames have no URI authority, query or fragment.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-5.3
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.4
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.5
   * \{ */
  std::string GetRawScheme() const;
  std::string GetRawAuthority() const;
  std::string GetRawPath() const;
  std::string GetRawQuery() const;
  std::string GetRawFragment() const;
  bool HasAuthority() const;
  bool HasQuery() const;
  bool HasFragment() const;
  /*! \} */

  /*! \brief Return raw bytes after the scheme colon and before the fragment delimiter.
   * \return An owned copy, including the syntactic query and its ? when present;
   *         empty for native input. No decoding or normalization is performed.
   * \note This is a Kodi accessor contract. Data consumers need the entire substring,
   *       rather than only the generic path. Scheme-specific validation remains local.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3
   * \sa https://www.rfc-editor.org/rfc/rfc2397.html#section-3
   * \sa https://www.rfc-editor.org/errata/eid2045
   */
  std::string GetSchemeSpecificPart() const;

  /*! \brief Replace a lossless URI's raw path, preserving every other component byte.
   * \param path Already identified path bytes, never another outer URI.
   * \return False without changes for unsupported policy or invalid path syntax.
   * \note Validation uses the current authority presence per RFC 3986: with an authority,
   *       the path is empty or slash-prefixed and may contain empty segments, including
   *       multiple leading slashes. Without an authority, paths cannot start with "//".
   *       Preserving authority presence/content and other component bytes is a Kodi guarantee.
   * \note A magnet path can be empty (external URI) or absolute (Kodi member extension).
   *       Query identity, empty delimiters, escape spelling and fragment are retained.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.3
   * \sa https://www.bittorrent.org/beps/bep_0009.html#magnet-uri-format
   */
  bool SetURIPath(std::string path);

  /*! \brief Replace the raw query of a lossless URI without a map or form decoding.
   * \param query No value removes the delimiter; an empty value retains an empty ?.
   * \return False without changes for legacy storage or invalid query syntax.
   * \note Duplicate/empty fields, plus signs and percent-escape spelling are retained.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.4
   */
  bool SetRawQuery(std::optional<std::string> query);
  /*! \brief Replace the legacy filename or lossless navigable raw path.
   * \note Non-file identifiers are unchanged. Use SetURIPath to observe failure.
   *       Legacy authority/credential/port setters likewise leave lossless forms unchanged.
   */
  void SetFileName(std::string strFileName);
  void SetHostName(std::string strHostName)
  {
    if (!m_uri)
      m_strHostName = std::move(strHostName);
  }

  void SetUserName(std::string strUserName)
  {
    if (!m_uri)
      m_strUserName = std::move(strUserName);
  }

  void SetDomain(std::string strDomain)
  {
    if (!m_uri)
      m_strDomain = std::move(strDomain);
  }

  void SetPassword(std::string strPassword)
  {
    if (!m_uri)
      m_strPassword = std::move(strPassword);
  }

  /*! \brief Change the scheme, preserving component bytes for lossless forms.
   * \note Invalid scheme tokens leave the object unchanged. An empty scheme switches
   *       to native path bytes and clears URI state. Transitions to established scheme://
   *       adapters reapply their normalization; authority-free forms remain lossless.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.1
   */
  void SetProtocol(std::string strProtocol);
  /*! \brief Set legacy options; lossless URI forms reject this operation unchanged.
   * \note Legacy map/transport-option setters also reject lossless forms with a warning.
   *       Read-only map access is empty for those forms; use raw component access instead.
   */
  void SetOptions(std::string strOptions);
  void SetProtocolOptions(std::string strOptions);
  void SetPort(int port)
  {
    if (!m_uri)
      m_iPort = port;
  }

  bool HasPort() const { return (m_iPort != 0); }

  int GetPort() const { return m_iPort; }

  const std::string& GetHostName() const { return m_strHostName; }

  const std::string& GetDomain() const { return m_strDomain; }

  const std::string& GetUserName() const { return m_strUserName; }

  const std::string& GetPassWord() const { return m_strPassword; }

  /*! \brief Return the compatibility filename, or the raw path for navigable lossless URIs.
   * \note Non-file identifiers have no filename, share or filetype. The reference
   *       belongs to this object and may change on mutation; use GetRawPath for URI bodies.
   */
  const std::string& GetFileName() const { return m_strFileName; }

  /*! \brief Return the normalized lower-case scheme; Get() retains lossless source spelling.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.1
   */
  const std::string& GetProtocol() const { return m_strProtocol; }

  std::string GetTranslatedProtocol() const;

  const std::string& GetFileType() const { return m_strFileType; }

  const std::string& GetShareName() const { return m_strShareName; }

  /*! \brief Return legacy options, or the raw lossless query including its leading ?.
   * \note Lossless fragments are separate and the legacy options map remains empty.
   *       This reference belongs to the object and may change on mutation.
   */
  const std::string& GetOptions() const { return m_strOptions; }

  const std::string& GetProtocolOptions() const { return m_strProtocolOptions; }

  std::string GetFileNameWithoutPath() const; /* return the filename excluding path */

  /*! \brief Return / for URI paths; native and legacy embedded filenames retain native separators.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-3.3
   */
  char GetDirectorySeparator() const;

  /*! \brief Serialize to an owned string using established legacy normalization or exact URI bytes.
   * \note Exact round trips are Kodi's lossless-storage guarantee, not a restriction
   *       on normalizations permitted by RFC 3986.
   * \sa https://www.rfc-editor.org/rfc/rfc3986.html#section-6.2.2
   */
  std::string Get() const;
  /*! \brief Serialize without legacy options, or without a lossless URI's query and fragment.
   * \warning Explicitly discards query identity; structural navigation should use SetURIPath.
   */
  std::string GetWithoutOptions() const;
  /*! \brief Serialize without credentials, optionally replacing them with redaction labels.
   * \param redact Also replaces data resources and their wrappers with bounded text when true.
   * \note Non-redacted data retains its complete source. Raw URI components are not decoded.
   */
  std::string GetWithoutUserDetails(bool redact = false) const;
  /*! \brief Return the legacy filename prefix, or lossless URI bytes preceding the path.
   * \return Empty for non-file identifiers; lossless query and fragment are omitted.
   */
  std::string GetWithoutFilename() const;
  std::string GetRedacted() const;
  /*! \brief Redact credentials and complete data bodies, including malformed candidates.
   * \note Kodi logging policy replaces a data resource, or an image, parent-in-hostname,
   *       or stack wrapper containing one, with scheme:[REDACTED] without copying its body.
   *       Get() and GetWithoutUserDetails(false) retain data bytes.
   * \sa https://www.rfc-editor.org/rfc/rfc2397.html#section-3
   */
  static std::string GetRedacted(std::string_view path);
  /*! \brief Test Kodi local-location compatibility, not the presence of a URI authority.
   * \note A new identifier without a hostname does not imply a native file or backend.
   */
  bool IsLocal() const;
  bool IsLocalHost() const;
  /*! \brief Test Kodi absolute-location compatibility, independently of handler availability.
   * \note Recognized identifiers and native roots qualify; drive-relative C:foo retains
   *       its historical true result. Interior :// text does not create an outer scheme.
   */
  static bool IsFullPath(const std::string& url);
  static std::string Decode(std::string_view strURLData);
  static std::string Encode(std::string_view strURLData);

  /*! \brief Check whether a URL is a given URL scheme.
   Comparison is case-insensitive as per RFC1738
   \param type a lower-case scheme name, e.g. "smb".
   \return true if the url is of the given scheme, false otherwise.
   */
  bool IsProtocol(std::string_view type) const { return IsProtocolEqual(m_strProtocol, type); }

  /*! \brief Check whether a URL protocol is a given URL scheme.
   Both parameters MUST be lower-case.  Typically this would be called using
   the result of TranslateProtocol() which enforces this for protocol.
   \param protocol a lower-case scheme name, e.g. "ftp"
   \param type a lower-case scheme name, e.g. "smb".
   \return true if the url is of the given scheme, false otherwise.
   */
  static bool IsProtocolEqual(const std::string& protocol, std::string_view type);

  /*! \brief Check whether a URL is a given filetype.
   Comparison is effectively case-insensitive as both the parameter
   and m_strFileType are lower-case.
   \param type a lower-case filetype, e.g. "mp3".
   \return true if the url is of the given filetype, false otherwise.
   */
  bool IsFileType(std::string_view type) const { return m_strFileType == type; }

  void GetOptions(std::map<std::string, std::string>& options) const;
  bool HasOption(const std::string& key) const;
  bool GetOption(const std::string& key, std::string& value) const;
  std::string GetOption(const std::string& key) const;
  void SetOption(const std::string& key, const std::string& value);
  void RemoveOption(const std::string& key);

  void GetProtocolOptions(std::map<std::string, std::string>& options) const;
  bool HasProtocolOption(const std::string& key) const;
  bool GetProtocolOption(const std::string& key, std::string& value) const;
  std::string GetProtocolOption(const std::string& key) const;
  void SetProtocolOption(const std::string& key, const std::string& value);
  void RemoveProtocolOption(const std::string& key);

  bool HasExtension(std::string_view extensions) const;
  std::string GetExtension() const;
  bool IsStack() const;
  bool IsMultiPath() const;
  bool IsFavourite() const;
  bool IsPlugin() const;
  bool IsScript() const;
  bool IsAddonsPath() const;
  bool IsSourcesPath() const;
  bool IsCDDA() const;
  bool IsISO9660() const;
  bool IsMusicDb() const;
  bool IsVideoDb() const;
  bool IsBlurayPath() const;
  bool IsAndroidApp() const;
  bool IsLibraryFolder() const;
  bool IsUPnP() const;
  bool IsAPK() const;
  bool IsZIP() const; // also checks for comic books!
  bool IsArchive() const;
  bool IsCBZ() const;
  bool IsCBR() const;
  bool IsDiscImage() const;
  bool IsPicture() const;

  bool HasParentInHostname() const;
  bool HasEncodedHostname() const;
  bool HasEncodedFilename() const;

  bool IsLibraryContent() const;

  bool IsBDFile() const;
  bool IsDVDFile() const;
  bool IsOpticalMediaFile() const;

private:
  friend class CURIRedactor;

  void StoreURI(std::string uri);
  void UpdateURIProjection();
  void UpdateFileName(std::string filename);
  std::optional<CURI> m_uri;
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
