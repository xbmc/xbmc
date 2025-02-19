/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonBase.h"
#include "c-api/general.h"
#include "tools/StringUtils.h"

#ifdef __cplusplus

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
  /// The version candidate e.g. alpha, beta or release
  std::string tag;
  /// The revision of tag before
  std::string tag_revision;
} kodi_version_t;
//------------------------------------------------------------------------------

namespace kodi
{

//==============================================================================
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
inline bool ATTR_DLL_LOCAL UnknownToUTF8(const std::string& stringSrc,
                                         std::string& utf8StringDst,
                                         bool failOnBadChar = false)
{
  using namespace kodi::addon;

  bool ret = false;
  char* retString = CPrivateBase::m_interface->toKodi->kodi->unknown_to_utf8(
      CPrivateBase::m_interface->toKodi->kodiBase, stringSrc.c_str(), &ret, failOnBadChar);
  if (retString != nullptr)
  {
    if (ret)
      utf8StringDst = retString;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   retString);
  }
  return ret;
}
//------------------------------------------------------------------------------

//==============================================================================
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
inline std::string ATTR_DLL_LOCAL GetLanguage(LangFormats format = LANG_FMT_ENGLISH_NAME,
                                              bool region = false)
{
  using namespace kodi::addon;

  std::string language;
  char* retString = CPrivateBase::m_interface->toKodi->kodi->get_language(
      CPrivateBase::m_interface->toKodi->kodiBase, format, region);
  if (retString != nullptr)
  {
    if (std::strlen(retString))
      language = retString;
    CPrivateBase::m_interface->toKodi->free_string(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   retString);
  }
  return language;
}
//------------------------------------------------------------------------------

//==============================================================================
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
inline void ATTR_DLL_LOCAL QueueFormattedNotification(QueueMsg type, const char* format, ...)
{
  using namespace kodi::addon;

  va_list args;
  va_start(args, format);
  const std::string str = kodi::tools::StringUtils::FormatV(format, args);
  va_end(args);
  CPrivateBase::m_interface->toKodi->kodi->queue_notification(
      CPrivateBase::m_interface->toKodi->kodiBase, type, "", str.c_str(), "", 5000, false, 1000);
}
//------------------------------------------------------------------------------

//==============================================================================
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
inline void ATTR_DLL_LOCAL QueueNotification(QueueMsg type,
                                             const std::string& header,
                                             const std::string& message,
                                             const std::string& imageFile = "",
                                             unsigned int displayTime = 5000,
                                             bool withSound = true,
                                             unsigned int messageTime = 1000)
{
  using namespace kodi::addon;

  CPrivateBase::m_interface->toKodi->kodi->queue_notification(
      CPrivateBase::m_interface->toKodi->kodiBase, type, header.c_str(), message.c_str(),
      imageFile.c_str(), displayTime, withSound, messageTime);
}
//------------------------------------------------------------------------------

//============================================================================
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
inline std::string ATTR_DLL_LOCAL GetMD5(const std::string& text)
{
  using namespace kodi::addon;

  char* md5ret = static_cast<char*>(malloc(40 * sizeof(char))); // md5 size normally 32 bytes
  CPrivateBase::m_interface->toKodi->kodi->get_md5(CPrivateBase::m_interface->toKodi->kodiBase,
                                                   text.c_str(), md5ret);
  std::string md5 = md5ret;
  free(md5ret);
  return md5;
}
//----------------------------------------------------------------------------

//==============================================================================
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
inline std::string ATTR_DLL_LOCAL GetRegion(const std::string& id)
{
  using namespace kodi::addon;

  AddonToKodiFuncTable_Addon* toKodi = CPrivateBase::m_interface->toKodi;

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
//------------------------------------------------------------------------------

//==============================================================================
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
inline void ATTR_DLL_LOCAL GetFreeMem(long& free, long& total, bool asBytes = false)
{
  using namespace kodi::addon;

  free = -1;
  total = -1;
  AddonToKodiFuncTable_Addon* toKodi = CPrivateBase::m_interface->toKodi;
  toKodi->kodi->get_free_mem(toKodi->kodiBase, &free, &total, asBytes);
}
//------------------------------------------------------------------------------

//==============================================================================
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
inline int ATTR_DLL_LOCAL GetGlobalIdleTime()
{
  using namespace kodi::addon;

  AddonToKodiFuncTable_Addon* toKodi = CPrivateBase::m_interface->toKodi;
  return toKodi->kodi->get_global_idle_time(toKodi->kodiBase);
}
//------------------------------------------------------------------------------

//==============================================================================
/// \ingroup cpp_kodi
/// @brief Get the currently used skin identification name from Kodi
///
/// @return The active skin id name as a string
///
///
/// @note This is not the full path like 'special://home/addons/MediaCenter',
/// but only 'MediaCenter'.
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ..
/// std::string skinid = kodi::GetCurrentSkinId();
/// ..
/// ~~~~~~~~~~~~~
///
inline std::string ATTR_DLL_LOCAL GetCurrentSkinId()
{
  using namespace kodi::addon;

  AddonToKodiFuncTable_Addon* toKodi = CPrivateBase::m_interface->toKodi;

  std::string strReturn;
  char* strMsg = toKodi->kodi->get_current_skin_id(toKodi->kodiBase);
  if (strMsg != nullptr)
  {
    if (std::strlen(strMsg))
      strReturn = strMsg;
    toKodi->free_string(toKodi->kodiBase, strMsg);
  }
  return strReturn;
}
//------------------------------------------------------------------------------

//==============================================================================
/// \ingroup cpp_kodi
/// @brief To check another addon is available and usable inside Kodi.
///
/// @param[in] id The wanted addon identification string to check
/// @param[out] version Version string of addon if **installed** inside Kodi
/// @param[out] enabled Set to true <b>`true* </b> if addon is enabled
/// @return Returns <b>`true* </b> if addon is installed
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
///
/// bool enabled = false;
/// std::string version;
/// bool ret = kodi::IsAddonAvailable("inputstream.adaptive", version, enabled);
/// fprintf(stderr, "Available inputstream.adaptive version '%s' and enabled '%s'\n",
///            ret ? version.c_str() : "not installed", enabled ? "yes" : "no");
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL IsAddonAvailable(const std::string& id,
                                            std::string& version,
                                            bool& enabled)
{
  using namespace kodi::addon;

  AddonToKodiFuncTable_Addon* toKodi = CPrivateBase::m_interface->toKodi;

  char* cVersion = nullptr;
  bool ret = toKodi->kodi->is_addon_avilable(toKodi->kodiBase, id.c_str(), &cVersion, &enabled);
  if (cVersion)
  {
    version = cVersion;
    toKodi->free_string(toKodi->kodiBase, cVersion);
  }
  return ret;
}
//------------------------------------------------------------------------------

//==============================================================================
/// \ingroup cpp_kodi
/// @brief Get current Kodi information and versions, returned data from the following
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
inline void ATTR_DLL_LOCAL KodiVersion(kodi_version_t& version)
{
  using namespace kodi::addon;

  char* compile_name = nullptr;
  char* revision = nullptr;
  char* tag = nullptr;
  char* tag_revision = nullptr;

  AddonToKodiFuncTable_Addon* toKodi = CPrivateBase::m_interface->toKodi;
  toKodi->kodi->kodi_version(toKodi->kodiBase, &compile_name, &version.major, &version.minor,
                             &revision, &tag, &tag_revision);
  if (compile_name != nullptr)
  {
    version.compile_name = compile_name;
    toKodi->free_string(toKodi->kodiBase, compile_name);
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
//------------------------------------------------------------------------------

//==============================================================================
/// \ingroup cpp_kodi
/// @brief To get keyboard layout characters
///
/// This is used to get the keyboard layout currently used from Kodi by the
/// there set language.
///
/// @param[in] modifierKey the key to define the needed layout (uppercase, symbols...)
/// @param[out] layout_name name of used layout
/// @param[out] layout list of selected keyboard layout
/// @return true if request succeeded
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// std::string layout_name;
/// std::vector<std::vector<std::string>> layout;
/// kodi::GetKeyboardLayout(STD_KB_MODIFIER_KEY_SHIFT | STD_KB_MODIFIER_KEY_SYMBOL, layout_name, layout);
/// fprintf(stderr, "Layout: '%s'\n", layout_name.c_str());
/// for (unsigned int row = 0; row < STD_KB_BUTTONS_MAX_ROWS; row++)
/// {
///   for (unsigned int column = 0; column < STD_KB_BUTTONS_PER_ROW; column++)
///   {
///     fprintf(stderr, " - Row: '%02i'; Column: '%02i'; Text: '%s'\n", row, column, layout[row][column].c_str());
///   }
/// }
/// ...
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL GetKeyboardLayout(int modifierKey,
                                             std::string& layout_name,
                                             std::vector<std::vector<std::string>>& layout)
{
  using namespace kodi::addon;

  AddonToKodiFuncTable_Addon* toKodi = CPrivateBase::m_interface->toKodi;
  AddonKeyboardKeyTable c_layout;
  char* c_layout_name = nullptr;
  bool ret =
      toKodi->kodi->get_keyboard_layout(toKodi->kodiBase, &c_layout_name, modifierKey, &c_layout);
  if (ret)
  {
    if (c_layout_name)
    {
      layout_name = c_layout_name;
      toKodi->free_string(toKodi->kodiBase, c_layout_name);
    }

    layout.resize(STD_KB_BUTTONS_MAX_ROWS);
    for (unsigned int row = 0; row < STD_KB_BUTTONS_MAX_ROWS; row++)
    {
      layout[row].resize(STD_KB_BUTTONS_PER_ROW);
      for (unsigned int column = 0; column < STD_KB_BUTTONS_PER_ROW; column++)
      {
        char* button = c_layout.keys[row][column];
        if (button)
        {
          layout[row][column] = button;
          toKodi->free_string(toKodi->kodiBase, button);
        }
      }
    }
  }
  return ret;
}
//------------------------------------------------------------------------------

//==============================================================================
/// \ingroup cpp_kodi
/// @brief To change keyboard layout characters
///
/// This is used to change the keyboard layout currently used from Kodi
///
/// @param[out] layout_name new name of used layout (input string not used!)
/// @return true if request succeeded
///
/// @note @ref GetKeyboardLayout must be called afterwards.
///
///
/// ------------------------------------------------------------------------
///
/// **Example:**
/// ~~~~~~~~~~~~~{.cpp}
/// #include <kodi/General.h>
/// ...
/// std::string layout_name;
/// kodi::ChangeKeyboardLayout(layout_name);
///
/// std::vector<std::vector<std::string>> layout;
/// kodi::GetKeyboardLayout(STD_KB_MODIFIER_KEY_SHIFT | STD_KB_MODIFIER_KEY_SYMBOL, layout_name, layout);
/// fprintf(stderr, "Layout: '%s'\n", layout_name.c_str());
/// for (unsigned int row = 0; row < STD_KB_BUTTONS_MAX_ROWS; row++)
/// {
///   for (unsigned int column = 0; column < STD_KB_BUTTONS_PER_ROW; column++)
///   {
///     fprintf(stderr, " - Row: '%02i'; Column: '%02i'; Text: '%s'\n", row, column, layout[row][column].c_str());
///   }
/// }
/// ...
/// ~~~~~~~~~~~~~
///
inline bool ATTR_DLL_LOCAL ChangeKeyboardLayout(std::string& layout_name)
{
  using namespace kodi::addon;

  AddonToKodiFuncTable_Addon* toKodi = CPrivateBase::m_interface->toKodi;
  char* c_layout_name = nullptr;
  bool ret = toKodi->kodi->change_keyboard_layout(toKodi->kodiBase, &c_layout_name);
  if (c_layout_name)
  {
    layout_name = c_layout_name;
    toKodi->free_string(toKodi->kodiBase, c_layout_name);
  }

  return ret;
}
//------------------------------------------------------------------------------

} /* namespace kodi */

#endif /* __cplusplus */
