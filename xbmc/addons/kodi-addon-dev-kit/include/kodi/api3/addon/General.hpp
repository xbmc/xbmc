#pragma once
/*
 *      Copyright (C) 2015 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>

#include "../definitions.hpp"
#include "../.internal/AddonLib_internal.hpp"

API_NAMESPACE

namespace KodiAPI
{
namespace AddOn
{
  /*!
   * @defgroup settings_xml settings.xml
   * \ingroup CPP_KodiAPI_AddOn
   * @{
   * @brief <b>Add-on settings.xml description</b>
   *
   * <tt>settings.xml</tt> is a XML file that contains the current configuration for the addon and should be placed in the
   * resources direcory. If your addon has configurable items that are set by the user, put them here. This file defines
   * what the user sees when they click on Addon settings for your addon. You don't need to do any coding to utilise this
   * functionality. The format for the settings file is relatively straightforward as can be seen in the following.
   * - <b>Example:</b>
   *
   * \code{.xml}
   * <?xml version="1.0" encoding="utf-8" standalone="yes"?>
   * <settings>
   *   <category label="32100">
   *     <setting id="scrolldelay" type="slider" label="32101" option="int" default="1" range="0,10" />
   *     <setting id="scrollmode" type="enum" label="32102" lvalues="32401|32402" default="0" />
   *     <setting id="navtimeout" type="slider" label="32103" option="int" default="2" range="1,10" />
   *     <setting id="refreshrate" type="slider" label="32104" option="int" default="1" range="1,20" />
   *     <setting id="sep1" type="sep" />
   *     <setting id="usealternatecharset" type="bool" label="32105" default="false" />
   *     <setting id="charset" enable="eq(-1,true)" type="enum" label="32106" lvalues="32411|32412|32413|32414|32415|32416|32417" default="0" subsetting="true"/>
   *     <setting id="sep2" type="sep" />
   *     <setting id="useextraelements" type="bool" label="32107" default="true" />
   *   </category>
   *   <category label="32200">
   *     <setting label="32206" type="lsep" />
   *     <setting id="dimonvideoplayback" type="bool" label="32203" default="false" subsetting="true" />
   *     <setting id="dimonmusicplayback" type="bool" label="32204" default="false" subsetting="true" />
   *     <setting id="dimonscreensaver" type="bool" label="32201" default="false" subsetting="true" />
   *     <setting id="dimdelay" type="slider" label="32205" default="0" range="0,30" subsetting="true" />
   *     <setting id="sep3" type="sep" />
   *     <setting id="dimonshutdown" type="bool" label="32202" default="false" />
   *   </category>
   *   <category label="32300">
   *     <setting id="remotelcdproc" type="bool" label="32301" default="false" />
   *     <setting id="hostip" enable="eq(-1,true)" type="ipaddress" label="32302" default="127.0.0.1" subsetting="true" />
   *     <setting id="hostport" enable="eq(-2,true)" type="number" label="32303" default="13666" subsetting="true" />
   *     <setting id="sep4" type="sep" />
   *     <setting id="heartbeat" type="bool" label="32304" default="false" />
   *     <setting id="sep5" type="sep" />
   *     <setting id="hideconnpopups" type="bool" label="32305" default="true" />
   *   </category>
   * </settings>
   * \endcode
   *
   * Settings can have additional attributes:
   *
   * \htmlonly
   * <table border="1">
   *   <tr>
   *     <th>Type</th>                         <th>Description</th>
   *   </tr>
   *   <tr>
   *     <td><b>source=""</b></td>             <td>
   * <b>video</b>, <b>music</b>, <b>pictures</b>, <b>programs</b>, <b>files</b>, <b>local</b> or <b>blank</b>.<br>
   * If source is blank it will use the type for shares if it is a valid share if not a valid<br>
   * share it will use, both local and network drives.<br></td>
   *   </tr>
   *   <tr>
   *     <td><b>visible=""</b></td>            <td><b>true</b>, <b>false</b> or <b>conditional</b>.<br>
   * Determines if the setting is displayed in the settings dialog (default = 'true')"<br>
   * <br>
   * You can also use a conditional statement:<br>
   * &nbsp; &nbsp; &nbsp;- <b><em><tt>visible="System.HasAddon(plugin.video.youtube)"</tt></em></b><br>
   * &nbsp; &nbsp; &nbsp;- <b><em><tt>visible="eq(-2,true)"</tt></em></b> ( same syntax like for enable="" )<br>
   * <br>
   * </td>
   *   </tr>
   *   <tr>
   *     <td><b>enable=""</b></td>             <td>
   * Allows you to determine whether this setting should be shown based on the value of<br>
   * another setting.<br>
   * 3 comparators are available:<br>
   * &nbsp; &nbsp; &nbsp;- <b><em><tt>eq()</tt></em></b> Equal to<br>
   * &nbsp; &nbsp; &nbsp;- <b><em><tt>gt()</tt></em></b> Greater than<br>
   * &nbsp; &nbsp; &nbsp;- <b><em><tt>lt()</tt></em></b> Less than<br>
   * <br>
   * You can AND the comparators using the + symbol and negate it using the ! symbol<br>
   *  (e.g. !eq()). Each comparator takes 2 operands:<br>
   * Relative position of setting to compare<br>
   * Value to compare against<br>
   * Thus if we place settings in our file in this order:<br>
   * &nbsp; &nbsp; &nbsp;- myFirst setting<br>
   * &nbsp; &nbsp; &nbsp;- mySecondSetting<br>
   * &nbsp; &nbsp; &nbsp;- myThirdSetting<br>
   * <br>
   * for the third setting we might add the option:<br>
   * &nbsp; &nbsp; &nbsp;- <b><em><tt>enable="gt(-2,3) + lt(-2,10)"</tt></em></b><br>
   * <br>
   * You can also use a conditional statement:<br>
   * &nbsp; &nbsp; &nbsp;- <b><em><tt>enable="System.HasAddon(plugin.video.youtube)"</tt></em></b><br>
   * <br>
   * </td>
   *   </tr>
   *   <tr>
   *     <td><b>subsetting="true"</b></td>     <td>Add dash in front of option<br><br></td>
   *   </tr>
   * </table>
   * <br>
   * \endhtmlonly
   *
   * <b><em>Void type examples for add-on</em></b> <tt>./resources/settings.xml</tt>
   *   - Line separators add a horizontal separating line between other element. They are purely user-interface elements
   *     that do not allow user input and therefore have no meaningful value as a setting.
   *   - <tt>lsep</tt>
   *     - Shows a horizontal line with a text.
   *        \code{.xml}
   *        <setting label="32024" type="lsep"/>
   *        \endcode
   *     - <b><em>label="id"</em></b> (required) - an id from the language file that indicates which text to display.
   *
   *   - <tt>sep</tt>
   *     - Shows a horizontal line.
   *        \code{.xml}
   *        <setting type="sep"/>
   *        <setting id="sep1" type="sep" />
   *        \endcode
   *
   *   - <tt>action</tt>
   *     - Adds line to configuration windows which allow to execute action
   *        \code{.xml}
   *        <setting label="32002" type="action" id="backend" action="XBMC.NotifyAll(service.xbmc.tts,SETTINGS.BACKEND_DIALOG)"/>
   *        \endcode
   *
   * @}
   */

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_AddOn_General General
  /// \ingroup CPP_KodiAPI_AddOn
  /// @{
  /// @brief <b>General functions</b>
  ///
  /// The below listed general functions allow to send your own commands to
  /// perform either there commands, or to obtain information.
  ///
  /// These are pure static functions them no other initialization need.
  ///
  /// It has the header \ref kodi/api3/addon/General.hpp "#include <kodi/api3/addon/General.hpp>" be included
  /// to enjoy it.
  ///

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_AddOn_General_Defs Definitions, structures and enumerators
  /// \ingroup CPP_KodiAPI_AddOn_General
  /// @brief <b>Library definition values</b>
  ///

  namespace General
  {
    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Get a string settings value for this add-on.
    ///
    /// \warning For this function and others below is no access on Kodi about passwords, paths and network parts allowed!
    /// Text input elements allow a user to input text in various formats. The "label" attribute must contain an id from the
    /// language file that indicates which text to display for the input field.
    /// To get path from add-on can be from <tt>GetSettingString</tt> the <tt>settingValue</tt> <a><b>__addonpath__</b></a> used.
    ///
    /// @param[in] settingName  The name of the setting to get.
    /// @param[in] settingValue The value.
    /// @param[in] global       [opt] If set to true becomes Kodi itself asked about a setting <em>(default is <b><c>false</c></b>)</em>
    /// @return  true if successfull done
    ///
    /// <b><em>String type examples for add-on</em></b> <tt>./resources/settings.xml</tt>
    ///   - <tt>text</tt>
    ///     - Allow a user to enter one line of text.
    ///        \code{.xml}
    ///        <setting id="addon.language" type="text" label="30523" default="en-US"/>
    ///        \endcode
    ///       - <b><em><tt>id="string"</tt></em></b> (required) - the name of the setting.
    ///       - <b><em><tt>label="id"</tt></em></b> (required) - an id from the language file that indicates which text to display.
    ///       - <b><em><tt>option="hidden"|"urlencoded"</tt></em></b> (optional)
    ///         - if set to <b><em>"hidden"</em></b>, each characters entered by the user will be obfuscated with an asterisks ("*"), as is usual for entering passwords.
    ///         - if set to <b><em>"urlencoded"</em></b>, each non-alphanumeric characters entered by the user will be "escaped" using %XX encoding.
    ///       - <b><em><tt>default="value"</tt></em></b> (optional) - the default value.
    ///
    ///   - <tt>folder</tt>
    ///     - Allow the user to browse for and select a folder.
    ///        \code{.xml}
    ///        <setting id="iconpath" type="folder" source="files" label="30048" default="" />
    ///        <setting id="download-folder" type="folder" label="32010"/>
    ///        <setting id="image_path" type="folder" source="pictures" label="32012" visible="eq(-2,1)" default=""/>
    ///        <setting label="32033" type="folder" id="folder" source="auto" option="writeable"/>
    ///        \endcode
    ///       - <b><em><tt>id="string"</tt></em></b> (required) - the name of the setting
    ///       - <b><em><tt>label="id"</tt></em></b> (required) - an id from the language file that indicates which text to display.
    ///       - <b><em><tt>source="auto"|"images"|...</tt></em></b> (optional, default="auto") - select a starting folder for the browse dialog.
    ///        | Source types      | Description                            |
    ///        |:-----------------:|----------------------------------------|
    ///        | music             | Include music related sources          |
    ///        | video             | Include video related sources          |
    ///        | pictures          | Include pictures related sources       |
    ///        | programs          | Include praograms related sources      |
    ///        | local             | Include local system related sources   |
    ///
    ///       - <b><em><tt>value="value"</tt></em></b> (optional, default="") - the default value.
    ///       - <b><em><tt>option="writeable"</tt></em></b> (optional, default="") - the user can be allowed to create and select new folders by setting this argument.
    ///
    ///   - <tt>file</tt>
    ///     - To select file by the user.
    ///        \code{.xml}
    ///        <setting id="rtmp_binary" type="file" label="103" default="" />
    ///        \endcode
    ///
    ///   - <tt>ipaddress</tt>
    ///     - To use setting for ip address with the related dialog.
    ///        \code{.xml}
    ///        <setting id="hostip" enable="eq(-1,true)" type="ipaddress" label="32302" default="127.0.0.1" subsetting="true" />
    ///        \endcode
    ///       - <b><em><tt>id="string"</tt></em></b> (required) - the name of the setting.
    ///       - <b><em><tt>label="id"</tt></em></b> (required) - an id from the language file that indicates which text to display.
    ///       - <b><em><tt>default="value"</tt></em></b> (optional) - the default value.
    ///
    ///   - <tt>fileenum</tt>
    ///     - Display files in a folder as an enum selector.
    ///        \code{.xml}
    ///        <-- Example: List sub-folders of the 'resources' folder for this add-on in the settings. --/>
    ///        <setting id="myenum" type="fileenum" values="resources" mask="/"/>
    ///        <setting id="trailer.ratingMax" label="32062" type="fileenum" values="$PROFILE/settings/ratings" default="MPAA" mask="/"/>
    ///        \endcode
    ///       - <b><em><tt>mask="value"</tt></em></b> - filter selectable files. Examples: "/" - display only folders. "*.txt" - display only .txt
    ///         files.
    ///       - <b><em><tt>option="hideext"</tt></em></b> - hide file extensions.
    ///
    ///   - <tt>labelenum</tt>
    ///     - Shows a spin control about given string values and use a string value from selection.
    ///        \code{.xml}
    ///        <setting id="bitrate" type="labelenum" label="30003" default="Max" values="Min|Max|Select" />
    ///        <setting id="trailer.quality" label="32067" type="labelenum" values="480p|720p|1080p" default="720p" />
    ///        <setting id="scope" label="32092" type="labelenum" values="16:9|2.35:1|2.40:1" default="0" />
    ///        <setting id="quality" type="labelenum" label="$LOCALIZE[622]" values="720p|480p" default="720p" />
    ///        \endcode
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <b>Code example:</b>
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    ///
    /// std::string bitrate = "Max";
    /// if (!KodiAPI::AddOn::General::GetSettingString("bitrate", bitrate))
    ///   KodiAPI::AddOn::General::Log(LOG_ERROR, "Couldn't get 'bitrate' setting, falling back to default");
    /// ~~~~~~~~~~~~~
    ///
    bool GetSettingString(const std::string& settingName, std::string& settingValue, bool global = false);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Numeric input elements allow a user to enter a number. The "label" attribute must contain an id from the language
    /// file that indicates which text to display for the input field.
    ///
    /// @param[in] settingName     The name of the setting to get.
    /// @param[in] settingValue    The value.
    /// @param[in] global          [opt] If set to true becomes Kodi itself asked about a setting <em>(default is <b><c>false</c></b>)</em>
    /// @return                true if successfull done
    ///
    /// <b><em>Integer type examples for add-on</em></b> <tt>./resources/settings.xml</tt>
    ///   - <tt>number</tt>
    ///     - To get a numeric entry
    ///        \code{.xml}
    ///        <setting id="kodion.view.default" type="number" label="30027" default="50"/>
    ///        \endcode
    ///       - <b><em><tt>id="string"</tt></em></b> (required) - the name of the setting.
    ///       - <b><em><tt>label="id"</tt></em></b> (required) - an id from the language file that indicates which text to display.
    ///       - <b><em><tt>default="value"</tt></em></b> (optional) - the default value.
    ///
    ///   - <tt>enum</tt>
    ///     - Shows a spin control about given string values and use a integer value from selection
    ///        \code{.xml}
    ///        <setting id="kodion.video.quality" type="enum" label="30010" enable="eq(1,false)" lvalues="30016|30017|30011|30012|30013" default="3" />
    ///        <setting id="notification_length" type="enum" label="101" values="1|2|3|4|5|6|7|8|9|10" default="5" />
    ///        <setting id="time-of-day" type="enum" values="All|Day only|Night only|Smart" label="32006" visible="true" default="0"/>
    ///        \endcode
    ///       - <b><em><tt>id="string"</tt></em></b> (required) - the name of the setting
    ///       - <b><em><tt>label="id"</tt></em></b> (required) - an id from the language file that indicates which text to display.
    ///       - <b><em><tt>values="value1[|value2[...]]"</tt></em></b> - or -
    ///       - <b><em><tt>lvalues="id1[|id2[...]]"</tt></em></b> (required):
    ///          - A list of values or language file ids from which the user can choose.
    ///          - values="$HOURS" is special case to select hour ( works only for type="enum" )
    ///       - <b><em><tt>sort="yes"</tt></em></b> - sorts labels ( works only for type="labelenum" )
    ///
    ///   - <tt>slider</tt>
    ///     - Allows the user to enter a number using a horizontal sliding bar.
    ///        \code{.xml}
    ///        <setting id="kodion.cache.size" type="slider" label="30024" default="5" range="1,1,20" option="int"/>
    ///        <setting label="32053" type="slider" id="limit" default="20" range="5,5,100" option="int" />
    ///        <setting label="32053" type="slider" id="limit" default="20" range="0,100"/>
    ///        \endcode
    ///       - <b><em><tt>id="string"</tt></em></b> (required) - the name of the setting.
    ///       - <b><em><tt>label="id"</tt></em></b> (required) - an id from the language file that indicates which text to display.
    ///       - <b><em><tt>range="min[,step],max"</tt></em></b> (required) - specify the range of valid values.
    ///       - <b><em><tt>option="int"|"float"|"percent"</tt></em></b> (required) - specifies whether to allow the user to choose between
    ///         integers, floating point numbers or a percentage.
    ///       - <b><em><tt>default="value"</tt></em></b> (optional) - the default value.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <b>Code example:</b>
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    ///
    /// int cachesize = 5;
    /// if (!KodiAPI::AddOn::General::GetSettingInt("kodion.cache.size", bitrate))
    ///   KodiAPI::AddOn::General::Log(LOG_ERROR, "Couldn't get 'kodion.cache.size' setting, falling back to default");
    /// ~~~~~~~~~~~~~
    ///
    bool GetSettingInt(const std::string& settingName, int& settingValue, bool global = false);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Get a boolean settings value for this add-on.
    /// @param[in] settingName  The name of the setting to get.
    /// @param[in] settingValue The value.
    /// @param[in] global       [opt] If set to true becomes Kodi itself asked about a setting <em>(default is <b><c>false</c></b>)</em>
    /// @return  true if successfull done
    ///
    /// <b><em>Boolean type examples for add-on</em></b> <tt>./resources/settings.xml</tt>
    ///   - <tt>bool</tt>
    ///     - Shows a boolean check point to settings dialog
    ///        \code{.xml}
    ///        <setting id="kodion.setup_wizard" type="bool" label="30025" default="true"/>
    ///        <setting id="show-notifications" type="bool" label="32018" default="true" />
    ///        \endcode
    ///       - <b><em><tt>id="string"</tt></em></b> (required) - the name of the setting
    ///       - <b><em><tt>label="id"</tt></em></b> (required) - an id from the language file that indicates which text to display.
    ///       - <b><em><tt>default="true"|"false"</tt></em></b> (optional) - the default value.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <b>Code example:</b>
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    ///
    /// bool show_notifications = true;
    /// if (!KodiAPI::AddOn::General::GetSettingBoolean("show-notifications", show_notifications))
    ///   KodiAPI::AddOn::General::Log(LOG_ERROR, "Couldn't get 'show-notifications' setting, falling back to default");
    /// ~~~~~~~~~~~~~
    ///
    bool GetSettingBoolean(const std::string& settingName, bool& settingValue, bool global = false);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Get a float settings value for this add-on.
    /// @param[in] settingName  The name of the setting to get.
    /// @param[in] settingValue The value.
    /// @param[in] global       [opt] If set to true becomes Kodi itself asked about a setting <em>(default is <b><c>false</c></b>)</em>
    /// @return  true if successful done
    ///
    /// <b><em>Floating type examples for add-on</em></b> <tt>./resources/settings.xml</tt>
    ///   - <tt>slider</tt>
    ///     - Shows a moveable slider control to get values from range
    ///        \code{.xml}
    ///        <setting id="kodi.amplication.volume" type="slider" label="30024" default="1.0" range="0.0,0.1,2.0" option="float"/>
    ///        \endcode
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// <b>Code example:</b>
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    ///
    /// float volume = 1.0;
    /// if (!KodiAPI::AddOn::General::GetSettingFloat("kodi.amplication.volume", volume))
    ///   KodiAPI::AddOn::General::Log(LOG_ERROR, "Couldn't get 'kodi.amplication.volume' setting, falling back to default");
    /// ~~~~~~~~~~~~~
    ///
    bool GetSettingFloat(const std::string& settingName, float& settingValue, bool global = false);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Opens this Add-Ons settings dialog.
    ///
    ///
    /// --------------------------------------------------------------------------
    ///
    ///  **Example:**
    ///  ~~~~~~~~~~~~~{.cpp}
    ///  ..
    ///  KodiAPI::AddOn::General::OpenSettings();
    ///  ..
    ///  ~~~~~~~~~~~~~
    ///
    void OpenSettings();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
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
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::QueueFormattedNotification(QUEUE_WARNING, "I'm want to inform you, here with a test call to show '%s'", "this");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void QueueFormattedNotification(const queue_msg type, const char* format, ... );
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Queue a notification in the GUI.
    /// @param[in] type          The message type.
    ///  |  enum code:    | Description:                      |
    ///  |---------------:|-----------------------------------|
    ///  |  QUEUE_INFO    | Show info notification message    |
    ///  |  QUEUE_WARNING | Show warning notification message |
    ///  |  QUEUE_ERROR   | Show error notification message   |
    /// @param[in] aCaption      Header Name
    /// @param[in] aDescription  Message to display on Kodi
    /// @param[in] displayTime   [opt] The time how long message is displayed <b>(default 5 sec)</b>
    /// @param[in] withSound     [opt] if true also warning sound becomes played <b>(default with sound)</b>
    /// @param[in] messageTime   [opt] how many milli seconds start show of notification <b>(default 1 sec)</b>
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::QueueNotification(QUEUE_INFO, "I'm want to inform you", "Here with a test call", 3000, false, 1000);
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void QueueNotification(const queue_msg type, const std::string& aCaption, const std::string& aDescription, unsigned int displayTime = 5000, bool withSound = true, unsigned int messageTime = 1000);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Queue a notification in the GUI.
    /// @param[in] aCaption      Header Name
    /// @param[in] aDescription  Message to display on Kodi
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::QueueNotification("I'm want to inform you", "Here with a test call");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void QueueNotification(const std::string& aCaption, const std::string& aDescription);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Queue a notification in the GUI.
    /// @param[in] aImageFile    The image file to show on message
    /// @param[in] aCaption      Header Name
    /// @param[in] aDescription  Message to display on Kodi
    /// @param[in] displayTime   [opt] The time how long message is displayed in ms <b>(default 5 sec)</b>
    /// @param[in] withSound     [opt] true also warning sound becomes played <b>(default with sound)</b>
    /// @param[in] messageTime   [opt] in how many milli seconds start show of notification in ms <b>(default 1 sec)</b>
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::QueueNotification("./myImage.png", "I'm want to inform you", "Here with a test call", 3000, true, 1000);
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void QueueNotification(const std::string& aImageFile, const std::string& aCaption, const std::string& aDescription, unsigned int displayTime = 5000, bool withSound = true, unsigned int  messageTime = 1000);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Get the MD5 digest of the given text
    ///
    /// @param[in]  text  text to compute the MD5 for
    /// @param[out] md5   Returned MD5 digest
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// std::string md5;
    /// KodiAPI::AddOn::General::GetMD5("Make me as md5", md5);
    /// fprintf(stderr, "My md5 digest is: '%s'", md5.c_str());
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void GetMD5(const std::string& text, std::string& md5);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Translate a string with an unknown encoding to UTF8.
    ///
    /// @param[in]  stringSrc       The string to translate.
    /// @param[out] utf8StringDst   The translated string.
    /// @param[in]  failOnBadChar   [opt] return failed if bad character is inside <em>(default is <b><c>false</c></b>)</em>
    /// @return                     true if OK
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// std::string ret;
    /// if (!KodiAPI::AddOn::General::UnknownToUTF8("test string", ret, true))
    ///   fprintf(stderr, "Translation to UTF8 failed!\n");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    bool UnknownToUTF8(const std::string& stringSrc, std::string& utf8StringDst, bool failOnBadChar = false);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Returns an addon's localized 'unicode string'.
    /// @param[in] labelId    for string you want to localize.
    /// @param[in] strDefault [opt] The default message, also helps to identify the code that is used <em>(default is <b><c>empty</c></b>)</em>
    /// @return The localized message, or strDefault if the add-on helper fails to return a message
    ///
    /// @note Label id's \b 30000 to \b 30999 and \b 32000 to \b 32999 are related to own add-on included
    /// strings inside <b>./resources/language/resource.language.??_??/strings.po</b>
    /// all others are from Kodi itself.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// std::string str = KodiAPI::AddOn::General::GetLocalizedString(30005, "Use me as default");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    std::string GetLocalizedString(uint32_t labelId, const std::string& strDefault = "");
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Returns the active language as a string.
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
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// std::string language = KodiAPI::AddOn::General::GetLanguage(LANG_FMT_ISO_639_1, false);
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    std::string GetLanguage(lang_formats format = LANG_FMT_ENGLISH_NAME, bool region = false);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Get the DVD menu language.
    /// @return The DVD menu langauge, or empty if unknown
    ///
    std::string GetDVDMenuLanguage();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Start or stop a server.
    /// @param[in] typ Used format of the returned language string
    /// | enum code:             | Description:                                               |
    /// |-----------------------:|------------------------------------------------------------|
    /// | ADDON_ES_WEBSERVER     | [To control Kodi's builtin webserver](http://kodi.wiki/view/Webserver)
    /// | ADDON_ES_AIRPLAYSERVER | [AirPlay is a proprietary protocol stack/suite developed by Apple Inc.](http://kodi.wiki/view/AirPlay)
    /// | ADDON_ES_JSONRPCSERVER | [Control JSON-RPC HTTP/TCP socket-based interface](http://kodi.wiki/view/JSON-RPC_API)
    /// | ADDON_ES_UPNPRENDERER  | [UPnP client (aka UPnP renderer)](http://kodi.wiki/view/UPnP/Client)
    /// | ADDON_ES_UPNPSERVER    | [Control built-in UPnP A/V media server (UPnP-server)](http://kodi.wiki/view/UPnP/Server)
    /// | ADDON_ES_EVENTSERVER   | [Set eventServer part that accepts remote device input on all platforms](http://kodi.wiki/view/EventServer)
    /// | ADDON_ES_ZEROCONF      | [Control Kodi's Avahi Zeroconf](http://kodi.wiki/view/Zeroconf)
    /// @param[in] start start (True) or stop (False) a server
    /// @param[in] wait [opt] wait on stop before returning (not supported by all servers) <em>(default is <b><c>false</c></b>)</em>
    /// @return true if successfull done
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// if (!KodiAPI::AddOn::General::StartServer(ADDON_ES_WEBSERVER, true))
    ///   KodiAPI::AddOn::General::Log(LOG_ERROR, "Start of web server failed!");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    bool StartServer(eservers typ, bool start, bool wait = false);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Suspend Audio engine
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::AudioSuspend();
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void AudioSuspend();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Resume Audio engine
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::AudioResume();
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void AudioResume();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Get the current global Kodi audio volume
    /// @param[in] percentage [opt] if set to false becomes amplication level returned
    /// @return The volume value
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// float percent = KodiAPI::AddOn::General::GetVolume(true);
    /// fprintf(stderr, "Current volume is set to %02f %%\n");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    float GetVolume(bool percentage = true);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Set the current global Kodi audio volume
    /// @param[in] value the volume to use
    /// @param[in] isPercentage [opt] if set to false becomes amplication level use
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::SetVolume(0.45, false);
    /// ...
    /// ~~~~~~~~~~~~~
    /// @b or
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::SetVolume(75.0, true);
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void SetVolume(float value, bool isPercentage = true);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Ask if Kodi audio is muted
    /// @return true if audio is muted
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// bool muted = KodiAPI::AddOn::General::IsMuted();
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    bool IsMuted();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Toggle the audio volume between on and off
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::ToggleMute();
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void ToggleMute();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Set Kodi's mute
    /// @param[in] mute with set to true becomes it muted otherwise audio is present
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::SetMute(true); // Enable muted audio
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void SetMute(bool mute);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Returns the dvd state as an integer
    /// @return The current state of drive
    ///
    dvd_state GetOpticalDriveState();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Send eject signal to optical drive
    /// @return true if successfull done
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// if (!KodiAPI::AddOn::General::EjectOpticalDrive())
    ///   KodiAPI::AddOn::General::Log(LOG_ERROR, "Eject of drive failed!");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    bool EjectOpticalDrive();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Get current Kodi informations and versions, returned data from the following
    /// <b><tt>kodi_version_t version; KodiAPI::AddOn::General::KodiVersion(version);</tt></b>
    /// is e.g.:
    /// ~~~~~~~~~~~~~{.cpp}
    /// version.compile_name = Kodi
    /// version.major        = 16
    /// version.minor        = 0
    /// version.revision     = 2015-11-30-74edffb-dirty
    /// version.tag          = beta
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
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// kodi_version_t version;
    /// KodiAPI::AddOn::General::KodiVersion(version);
    /// fprintf(stderr,
    ///     "kodi_version_t version;\n"
    ///     "KodiAPI::AddOn::General::KodiVersion(version);\n"
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
    void KodiVersion(kodi_version& version);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Tell Kodi to stop work, go to exit and stop his work.
    /// \warning Kodi is really quited!
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::KodiQuit();
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void KodiQuit();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Shutdown the htpc
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::HTPCShutdown();
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void HTPCShutdown();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Restart the htpc
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::HTPCRestart();
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void HTPCRestart();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Execute a python script
    /// @param[in] script filename to execute
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::ExecuteScript("special://home/scripts/update.py");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void ExecuteScript(const std::string& script);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Execute a built in Kodi function
    ///
    /// @param[in] function builtin function to execute
    /// @param[in] wait [opt] if true wait until finished
    ///
    ///
    /// See list of \ref page_List_of_built_in_functions "build in functions"
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// KodiAPI::AddOn::General::ExecuteBuiltin("RunXBE(c:\\avalaunch.xbe)");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    void ExecuteBuiltin(const std::string& function, bool wait = false);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Execute an JSONRPC command
    /// @param[in] jsonrpccommand jsonrpc command to execute
    /// @return  From jsonrpc returned string for command
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// std::string response = KodiAPI::AddOn::General::ExecuteJSONRPC('{ \"jsonrpc\": \"2.0\", \"method\": \"JSONRPC.Introspect\", \"id\": 1 }');
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    std::string ExecuteJSONRPC(const std::string& jsonrpccommand);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Returns your regions setting as a string for the specified id
    /// @param[in] id id of setting to return
    /// |              | Choices are  |              |
    /// |:------------:|:------------:|:------------:|
    /// |  dateshort   | time         | tempunit     |
    /// |  datelong    | meridiem     | speedunit    |
    ///
    /// @return settings string
    ///
    /// @warning Throws exception on wrong string value!
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// std::string timeFormat = KodiAPI::AddOn::General::GetRegion("time");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    std::string GetRegion(const std::string& id);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Returns the amount of free memory in MB as an integer
    /// @return free memory
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// long freeMem = KodiAPI::AddOn::General::GetFreeMem();
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    long GetFreeMem();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Returns the elapsed idle time in seconds as an integer
    /// @return idle time
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// int time = KodiAPI::AddOn::General::GetGlobalIdleTime();
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    int GetGlobalIdleTime();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Returns the value of an addon property as a string
    /// @param[in] id id of the property that the module needs to access
    /// |              | Choices are  |              |
    /// |:------------:|:------------:|:------------:|
    /// |  author      | icon         | stars        |
    /// |  changelog   | id           | summary      |
    /// |  description | name         | type         |
    /// |  disclaimer  | path         | version      |
    /// |  fanart      | profile      |              |
    ///
    /// @return Returned information string
    ///
    /// @warning Throws exception on wrong string value!
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// std::string addonName = KodiAPI::AddOn::General::GetAddonInfo("name");
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    std::string GetAddonInfo(const std::string& id);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Returns the translated path
    ///
    /// @param[in] path  string or unicode - Path to format
    /// @return      A human-readable string suitable for logging
    ///
    /// @note        Only useful if you are coding for both Linux and Windows.
    ///              e.g. Converts 'special://masterprofile/script_data' -> '/home/user/.kodi/UserData/script_data'
    ///              on Linux.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// std::string path = KodiAPI::AddOn::General::TranslatePath("special://masterprofile/script_data");
    /// fprintf(stderr, "Translated path is: %s\n", path.c_str());
    /// ...
    /// ~~~~~~~~~~~~~
    /// or
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// fprintf(stderr, "Directory 'special://temp' is '%s'\n", KodiAPI::AddOn::General::TranslatePath("special://temp").c_str());
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    std::string TranslatePath(const std::string& path);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// \ingroup CPP_KodiAPI_AddOn_General
    /// @brief Translate an add-on status return code into a human-readable string
    /// @param[in] status The return code
    /// @return A human-readable string suitable for logging
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.cpp}
    /// #include <kodi/api3/addon/General.hpp>
    /// ...
    /// std::string error = KodiAPI::AddOn::General::TranslateAddonStatus(ADDON_STATUS_PERMANENT_FAILURE);
    /// fprintf(stderr, "Error is: %s\n", error.c_str());
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    std::string TranslateAddonStatus(ADDON_STATUS status);
    //----------------------------------------------------------------------------
  }; /* namespace General */
  //@}

} /* namespace AddOn */
} /* namespace KodiAPI */

END_NAMESPACE()