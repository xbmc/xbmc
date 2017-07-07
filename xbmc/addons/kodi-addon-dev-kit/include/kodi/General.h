#pragma once
/*
 *      Copyright (C) 2005-2017 Team Kodi
 *      http://kodi.tv
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "AddonBase.h"

/*
 * For interface between add-on and kodi.
 *
 * This structure defines the addresses of functions stored inside Kodi which
 * are then available for the add-on to call
 *
 * All function pointers there are used by the C++ interface functions below.
 * You find the set of them on xbmc/addons/interfaces/General.cpp
 *
 * Note: For add-on development itself this is not needed
 */
typedef struct AddonToKodiFuncTable_kodi
{
  char* (*get_addon_info)(void* kodiBase, const char* id);
  bool (*open_settings_dialog)(void* kodiBase);
  char* (*unknown_to_utf8)(void* kodiBase, const char* source, bool* ret, bool failOnBadChar);
  char* (*get_localized_string)(void* kodiBase, long dwCode);
  char* (*get_language)(void* kodiBase, int format, bool region);
  bool (*queue_notification)(void* kodiBase, int type, const char* header, const char* message, const char* imageFile, unsigned int displayTime, bool withSound, unsigned int messageTime);
  void (*get_md5)(void* kodiBase, const char* text, char* md5);
  char* (*get_temp_path)(void* kodiBase);
  char* (*get_region)(void* kodiBase, const char* id);
  void (*get_free_mem)(void* kodiBase, long* free, long* total, bool as_bytes);
  int  (*get_global_idle_time)(void* kodiBase);
  void (*kodi_version)(void* kodiBase, char** compile_name, int* major, int* minor, char** revision, char** tag, char** tagversion);
} AddonToKodiFuncTable_kodi;

//==============================================================================
/// \ingroup cpp_kodi_Defs
/// @brief For kodi::QueueNotification() used message types
///
typedef enum QueueMsg
{
  /// Show info notification message
  QUEUE_INFO,
  /// Show warning notification message
  QUEUE_WARNING,
  /// Show error notification message
  QUEUE_ERROR,
  /// Show with own given image and parts if set on values
  QUEUE_OWN_STYLE
} QueueMsg;
//------------------------------------------------------------------------------

//==============================================================================
/// \ingroup cpp_kodi_Defs
/// @brief Format codes to get string from them.
///
/// Used on kodi::GetLanguage().
///
typedef enum LangFormats
{
  /// two letter code as defined in ISO 639-1
  LANG_FMT_ISO_639_1,
  /// three letter code as defined in ISO 639-2/T or ISO 639-2/B
  LANG_FMT_ISO_639_2,
  /// full language name in English
  LANG_FMT_ENGLISH_NAME
} LangFormats;
//------------------------------------------------------------------------------

//==============================================================================
/// \ingroup cpp_kodi_Defs
/// @brief For kodi::Version used structure
///
typedef struct kodi_version_t
{
  /// Application name, normally 'Kodi'
  std::string compile_name;
  /// Major code version of Kodi
  int major;
  /// Minor code version of Kodi
  int minor;
  /// The Revision contains a id and the build date, e.g. 20170706-c6b22fe217-dirty
  std::string revision;
  /// The version canditate e.g. alpha, beta or release
  std::string tag;
  /// The revision of tag before
  std::string tag_revision;
} kodi_version_t;
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief Returns the value of an addon property as a string
///
/// @param[in] id id of the property that the module needs to access
/// |              | Choices are  |              |
/// |:------------:|:------------:|:------------:|
/// |  author      | icon         | stars        |
/// |  changelog   | id           | summary      |
/// |  description | name         | type         |
/// |  disclaimer  | path         | version      |
/// |  fanart      | profile      |              |
///
/// @return AddOn property as a string
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// std::string addonName = kodi::GetAddonInfo("name");
/// ...
/// ~~~~~~~~~~~~~
///
inline std::string GetAddonInfo(const std::string& id)
{
  AddonToKodiFuncTable_Addon* toKodi = ::kodi::addon::CAddonBase::m_interface->toKodi;

  std::string strReturn;
  char* strMsg = toKodi->kodi->get_addon_info(toKodi->kodiBase, id.c_str());
  if (strMsg != nullptr)
  {
    if (std::strlen(strMsg))
      strReturn = strMsg;
    toKodi->free_string(toKodi->kodiBase, strMsg);
  }
  return strReturn;
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief Opens this Add-Ons settings dialog.
///
/// @return true if settings were changed and the dialog confirmed, false otherwise.
///
///
/// --------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ..
/// kodi::OpenSettings();
/// ..
/// ~~~~~~~~~~~~~
///
inline bool OpenSettings()
{
  return ::kodi::addon::CAddonBase::m_interface->toKodi->kodi->open_settings_dialog(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase);
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief Returns an addon's localized 'unicode string'.
///
/// @param[in] labelId    string you want to localize
/// @param[in] defaultStr [opt] The default message, also helps to identify
///                       the code that is used <em>(default is
///                       <b><c>empty</c></b>)</em>
/// @return               The localized message, or default if the add-on
///                       helper fails to return a message
///
/// @note Label id's \b 30000 to \b 30999 and \b 32000 to \b 32999 are related 
/// to the add-on's own included strings from 
/// <b>./resources/language/resource.language.??_??/strings.po</b>
/// All other strings are from Kodi core language files.
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// std::string str = kodi::GetLocalizedString(30005, "Use me as default");
/// ...
/// ~~~~~~~~~~~~~
///
inline std::string GetLocalizedString(uint32_t labelId, const std::string& defaultStr = "")
{
  std::string retString = defaultStr;
  char* strMsg = ::kodi::addon::CAddonBase::m_interface->toKodi->kodi->get_localized_string(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, labelId);
  if (strMsg != nullptr)
  {
    if (std::strlen(strMsg))
      retString = strMsg;
    ::kodi::addon::CAddonBase::m_interface->toKodi->free_string(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, strMsg);
  }
  return retString;
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief Translate a string with an unknown encoding to UTF8.
///
/// @param[in]  stringSrc       The string to translate.
/// @param[out] utf8StringDst   The translated string.
/// @param[in]  failOnBadChar   [opt] returns failed if bad character is inside <em>(default is <b><c>false</c></b>)</em>
/// @return                     true if OK
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// std::string ret;
/// if (!kodi::UnknownToUTF8("test string", ret, true))
///   fprintf(stderr, "Translation to UTF8 failed!\n");
/// ...
/// ~~~~~~~~~~~~~
///
inline bool UnknownToUTF8(const std::string& stringSrc, std::string& utf8StringDst, bool failOnBadChar = false)
{
  bool ret = false;
  char* retString = ::kodi::addon::CAddonBase::m_interface->toKodi->kodi->unknown_to_utf8(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase,
                                                                                          stringSrc.c_str(), &ret, failOnBadChar);
  if (retString != nullptr)
  {
    if (ret)
      utf8StringDst = retString;
    ::kodi::addon::CAddonBase::m_interface->toKodi->free_string(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, retString);
  }
  return ret;
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief Returns the active language as a string.
///
/// @param[in] format Used format of the returned language string
///  | enum code:            | Description:                                               |
///  |----------------------:|------------------------------------------------------------|
///  | LANG_FMT_ENGLISH_NAME | full language name in English (Default)                    |
///  | LANG_FMT_ISO_639_1    | two letter code as defined in ISO 639-1                    |
///  | LANG_FMT_ISO_639_2    | three letter code as defined in ISO 639-2/T or ISO 639-2/B |
/// @param[in] region [opt] append the region delimited by "-" of the language (setting) to the returned language string <em>(default is <b><c>false</c></b>)</em>
/// @return active language
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// std::string language = kodi::GetLanguage(LANG_FMT_ISO_639_1, false);
/// ...
/// ~~~~~~~~~~~~~
///
inline std::string GetLanguage(LangFormats format = LANG_FMT_ENGLISH_NAME, bool region = false)
{
  std::string language;
  char* retString = ::kodi::addon::CAddonBase::m_interface->toKodi->kodi->get_language(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, format, region);
  if (retString != nullptr)
  {
    if (std::strlen(retString))
      language = retString;
    ::kodi::addon::CAddonBase::m_interface->toKodi->free_string(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, retString);
  }
  return language;
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief Writes the C string pointed by format in the GUI. If format includes
/// format specifiers (subsequences beginning with %), the additional arguments
/// following format are formatted and inserted in the resulting string replacing
/// their respective specifiers.
///
/// After the format parameter, the function expects at least as many additional
/// arguments as specified by format.
///
/// @param[in] type          The message type.
///  |  enum code:    | Description:                      |
///  |---------------:|-----------------------------------|
///  |  QUEUE_INFO    | Show info notification message    |
///  |  QUEUE_WARNING | Show warning notification message |
///  |  QUEUE_ERROR   | Show error notification message   |
/// @param[in] format        The format of the message to pass to display in Kodi.
///                      C string that contains the text to be written to the stream.
///                      It can optionally contain embedded format specifiers that are
///                      replaced by the values specified in subsequent additional
///                      arguments and formatted as requested.
///  |  specifier | Output                                             | Example
///  |------------|----------------------------------------------------|------------
///  |  d or i    | Signed decimal integer                             | 392
///  |  u         | Unsigned decimal integer                           | 7235
///  |  o         | Unsigned octal                                     | 610
///  |  x         | Unsigned hexadecimal integer                       | 7fa
///  |  X         | Unsigned hexadecimal integer (uppercase)           | 7FA
///  |  f         | Decimal floating point, lowercase                  | 392.65
///  |  F         | Decimal floating point, uppercase                  | 392.65
///  |  e         | Scientific notation (mantissa/exponent), lowercase | 3.9265e+2
///  |  E         | Scientific notation (mantissa/exponent), uppercase | 3.9265E+2
///  |  g         | Use the shortest representation: %e or %f          | 392.65
///  |  G         | Use the shortest representation: %E or %F          | 392.65
///  |  a         | Hexadecimal floating point, lowercase              | -0xc.90fep-2
///  |  A         | Hexadecimal floating point, uppercase              | -0XC.90FEP-2
///  |  c         | Character                                          | a
///  |  s         | String of characters                               | sample
///  |  p         | Pointer address                                    | b8000000
///  |  %         | A % followed by another % character will write a single % to the stream. | %
///
/// The length sub-specifier modifies the length of the data type. This is a chart
/// showing the types used to interpret the corresponding arguments with and without
/// length specifier (if a different type is used, the proper type promotion or
/// conversion is performed, if allowed):
///  | length| d i           | u o x X               | f F e E g G a A | c     | s       | p       | n               |
///  |-------|---------------|-----------------------|-----------------|-------|---------|---------|-----------------|
///  | (none)| int           | unsigned int          | double          | int   | char*   | void*   | int*            |
///  | hh    | signed char   | unsigned char         |                 |       |         |         | signed char*    |
///  | h     | short int     | unsigned short int    |                 |       |         |         | short int*      |
///  | l     | long int      | unsigned long int     |                 | wint_t| wchar_t*|         | long int*       |
///  | ll    | long long int | unsigned long long int|                 |       |         |         | long long int*  |
///  | j     | intmax_t      | uintmax_t             |                 |       |         |         | intmax_t*       |
///  | z     | size_t        | size_t                |                 |       |         |         | size_t*         |
///  | t     | ptrdiff_t     | ptrdiff_t             |                 |       |         |         | ptrdiff_t*      |
///  | L     |               |                       | long double     |       |         |         |                 |
///  <b>Note:</b> that the c specifier takes an int (or wint_t) as argument, but performs the proper conversion to a char value
///  (or a wchar_t) before formatting it for output.
/// @param[in] ... (additional arguments) Depending on the format string, the function
///            may expect a sequence of additional arguments, each containing a value
///            to be used to replace a format specifier in the format string (or a pointer
///            to a storage location, for n).
///            There should be at least as many of these arguments as the number of values specified
///            in the format specifiers. Additional arguments are ignored by the function.
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// kodi::QueueFormattedNotification(QUEUE_WARNING, "I'm want to inform you, here with a test call to show '%s'", "this");
/// ...
/// ~~~~~~~~~~~~~
///
inline void QueueFormattedNotification(QueueMsg type, const char* format, ... )
{
  va_list args;
  char buffer[16384];
  va_start(args, format);
  vsprintf(buffer, format, args);
  va_end(args);
  ::kodi::addon::CAddonBase::m_interface->toKodi->kodi->queue_notification(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase,
                                                                           type, "", buffer, "", 5000, false, 1000);
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief Queue a notification in the GUI.
///
/// @param[in] type          The message type.
///  |  enum code:           | Description:
///  |----------------------:|-----------------------------------
///  |  QUEUE_INFO           | Show info notification message
///  |  QUEUE_WARNING        | Show warning notification message
///  |  QUEUE_ERROR          | Show error notification message
///  |  QUEUE_OWN_STYLE      | If used can be with imageFile the wanted image set or if leaved empty shown as info, also are the other optional values available then
/// @param[in] header        Header Name (if leaved empty becomes addon name used)
/// @param[in] message       Message to display on Kodi
/// @param[in] imageFile     [opt] The image file to show on message (to use must be type set to QUEUE_OWN_STYLE)
/// @param[in] displayTime   [opt] The time how long message is displayed <b>(default 5 sec)</b> (to use must be type set to QUEUE_OWN_STYLE)
/// @param[in] withSound     [opt] if true also warning sound becomes played <b>(default with sound)</b> (to use must be type set to QUEUE_OWN_STYLE)
/// @param[in] messageTime   [opt] how many milli seconds start show of notification <b>(default 1 sec)</b> (to use must be type set to QUEUE_OWN_STYLE)
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// kodi::QueueNotification(QUEUE_OWN_STYLE, "I'm want to inform you", "Here with a test call", "", 3000, false, 1000);
/// ...
/// ~~~~~~~~~~~~~
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// kodi::QueueNotification(QUEUE_WARNING, "I'm want to inform you", "Here with a test call");
/// ...
/// ~~~~~~~~~~~~~
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// kodi::QueueNotification(QUEUE_OWN_STYLE, "", "Here with a test call", "./myImage.png");
/// ...
/// ~~~~~~~~~~~~~
///
inline void QueueNotification(QueueMsg type, const std::string& header,
                              const std::string& message, const std::string& imageFile = "",
                              unsigned int displayTime = 5000, bool withSound = true,
                              unsigned int messageTime = 1000)
{
  ::kodi::addon::CAddonBase::m_interface->toKodi->kodi->queue_notification(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase,
                                                                           type, header.c_str(), message.c_str(), imageFile.c_str(), displayTime,
                                                                           withSound, messageTime);
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief Get the MD5 digest of the given text
///
/// @param[in]  text  text to compute the MD5 for
/// @return           Returned MD5 digest
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// std::string md5 = kodi::GetMD5("Make me as md5");
/// fprintf(stderr, "My md5 digest is: '%s'\n", md5.c_str());
/// ...
/// ~~~~~~~~~~~~~
///
inline std::string GetMD5(const std::string& text)
{
  char* md5ret = static_cast<char*>(malloc(40*sizeof(char))); // md5 size normally 32 bytes
  ::kodi::addon::CAddonBase::m_interface->toKodi->kodi->get_md5(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, text.c_str(), md5ret);
  std::string md5 = md5ret;
  free(md5ret);
  return md5;
}
} /* namespace kodi */
//----------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief To get a temporary path for the addon
///
/// This gives a temporary path which the addon can use individually for its things.
///
/// The content of this folder will be deleted when Kodi is finished!
///
/// @param[in] append A string to append to returned temporary path
/// @return Individual path for the addon
///
inline std::string GetTempAddonPath(const std::string& append = "")
{
  char* str = ::kodi::addon::CAddonBase::m_interface->toKodi->kodi->get_temp_path(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase);
  std::string ret = str;
  ::kodi::addon::CAddonBase::m_interface->toKodi->free_string(::kodi::addon::CAddonBase::m_interface->toKodi->kodiBase, str);
  if (!append.empty())
  {
    if (append.at(0) != '\\' &&
        append.at(0) != '/')
#ifdef TARGET_WINDOWS
      ret.append("\\");
#else
      ret.append("/");
#endif
    ret.append(append);
  }
  return ret;
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief Returns your regions setting as a string for the specified id
///
/// @param[in] id id of setting to return
/// |              | Choices are  |              |
/// |:------------:|:------------:|:------------:|
/// |  dateshort   | time         | tempunit     |
/// |  datelong    | meridiem     | speedunit    |
///
/// @return settings string
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// std::string timeFormat = kodi::GetRegion("time");
/// ...
/// ~~~~~~~~~~~~~
///
inline std::string GetRegion(const std::string& id)
{
  AddonToKodiFuncTable_Addon* toKodi = ::kodi::addon::CAddonBase::m_interface->toKodi;

  std::string strReturn;
  char* strMsg = toKodi->kodi->get_region(toKodi->kodiBase, id.c_str());
  if (strMsg != nullptr)
  {
    if (std::strlen(strMsg))
      strReturn = strMsg;
    toKodi->free_string(toKodi->kodiBase, strMsg);
  }
  return strReturn;
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief Returns the amount of free memory in MByte (or as bytes) as an long
/// integer
///
/// @param[out] free free memory
/// @param[out] total total memory
/// @param[in] asBytes [opt] if set to true becomes returned as bytes, otherwise
///                    as mega bytes
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// long freeMem;
/// long totalMem;
/// kodi::GetFreeMem(freeMem, totalMem);
/// ...
/// ~~~~~~~~~~~~~
///
inline void GetFreeMem(long& free, long& total, bool asBytes = false)
{
  free = -1;
  total = -1;
  AddonToKodiFuncTable_Addon* toKodi = ::kodi::addon::CAddonBase::m_interface->toKodi;
  toKodi->kodi->get_free_mem(toKodi->kodiBase, &free, &total, asBytes);
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief Returns the elapsed idle time in seconds as an integer
///
/// @return idle time
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// int time = kodi::GetGlobalIdleTime();
/// ...
/// ~~~~~~~~~~~~~
///
inline int GetGlobalIdleTime()
{
  AddonToKodiFuncTable_Addon* toKodi = ::kodi::addon::CAddonBase::m_interface->toKodi;
  return toKodi->kodi->get_global_idle_time(toKodi->kodiBase);
}
} /* namespace kodi */
//------------------------------------------------------------------------------

//==============================================================================
namespace kodi {
///
/// \ingroup cpp_kodi
/// @brief Get current Kodi informations and versions, returned data from the following
/// <b><tt>kodi_version_t version; kodi::KodiVersion(version);</tt></b>
/// is e.g.:
/// ~~~~~~~~~~~~~{.cpp}
/// version.compile_name = Kodi
/// version.major        = 18
/// version.minor        = 0
/// version.revision     = 20170706-c6b22fe217-di
/// version.tag          = alpha
/// version.tag_revision = 1
/// ~~~~~~~~~~~~~
///
/// @param[out] version structure to store data from kodi
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// kodi_version_t version;
/// kodi::KodiVersion(version);
/// fprintf(stderr,
///     "kodi_version_t version;\n"
///     "kodi::KodiVersion(version);\n"
///     " - version.compile_name = %s\n"
///     " - version.major        = %i\n"
///     " - version.minor        = %i\n"
///     " - version.revision     = %s\n"
///     " - version.tag          = %s\n"
///     " - version.tag_revision = %s\n",
///             version.compile_name.c_str(), version.major, version.minor,
///             version.revision.c_str(), version.tag.c_str(), version.tag_revision.c_str());
/// ...
/// ~~~~~~~~~~~~~
///
inline void KodiVersion(kodi_version_t& version)
{
  char* compile_name = nullptr;
  char* revision = nullptr;
  char* tag = nullptr;
  char* tag_revision = nullptr;

  AddonToKodiFuncTable_Addon* toKodi = ::kodi::addon::CAddonBase::m_interface->toKodi;
  toKodi->kodi->kodi_version(toKodi->kodiBase, &compile_name, &version.major, &version.minor, &revision, &tag, &tag_revision);
  if (compile_name != nullptr)
  {
    version.compile_name  = compile_name;
    toKodi->free_string
    (
      toKodi->kodiBase,
      compile_name
    );
  }
  if (revision != nullptr)
  {
    version.revision = revision;
    toKodi->free_string(toKodi->kodiBase, revision);
  }
  if (tag != nullptr)
  {
    version.tag = tag;
    toKodi->free_string(toKodi->kodiBase, tag);
  }
  if (tag_revision != nullptr)
  {
    version.tag_revision = tag_revision;
    toKodi->free_string(toKodi->kodiBase, tag_revision);
  }
}
} /* namespace kodi */
//------------------------------------------------------------------------------
