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

#include "Tuple.h"
#include "AddonString.h"
#include "ListItem.h"
#include "swighelper.h"

#ifndef DOXYGEN_SHOULD_SKIP_THIS
namespace XBMCAddon
{
  namespace xbmcplugin
  {
#endif /* DOXYGEN_SHOULD_SKIP_THIS */

    //
    /// \defgroup python_xbmcplugin Library - xbmcplugin
    /// @{
    /// @brief <b>Plugin functions on Kodi.</b>
    ///
    /// Offers classes and functions that allow a developer to present
    /// information through Kodi's standard menu structure. While plugins don't
    /// have the same flexibility as scripts, they boast significantly quicker
    /// development time and a more consistent user experience.
    //

    ///
    /// \ingroup python_xbmcplugin
    /// Callback function to pass directory contents back to Kodi.
    ///
    /// @param[in] handle               integer - handle the plugin was started
    ///                                 with.
    /// @param[in] url                  string - url of the entry. would be
    ///                                 `plugin://` for another virtual directory
    /// @param[in] listitem             ListItem - item to add.
    /// @param[in] isFolder             [opt] bool - True=folder / False=not a
    ///                                 folder(default).
    /// @param[in] totalItems           [opt] integer - total number of items
    ///                                 that will be passed.(used for progressbar)
    /// @return                         Returns a bool for successful completion.
    ///
    /// @note You can use the above as keywords for arguments and skip certain
    ///       optional arguments. Once you use a keyword, all following arguments
    ///       require the keyword.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// if not xbmcplugin.addDirectoryItem(int(sys.argv[1]), 'F:\\Trailers\\300.mov', listitem, totalItems=50): break
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    bool addDirectoryItem(int handle, const String& url, const XBMCAddon::xbmcgui::ListItem* listitem,
                          bool isFolder = false, int totalItems = 0);

    ///
    /// \ingroup python_xbmcplugin
    /// Callback function to pass directory contents back to Kodi as a list.
    ///
    /// @param[in] handle               integer - handle the plugin was started
    ///                                 with.
    /// @param[in] items                List - list of (url, listitem[, isFolder])
    ///                                 as a tuple to add.
    /// @param[in] totalItems           [opt] integer - total number of items
    ///                                 that will be passed.(used for progressbar)
    /// @return                         Returns a bool for successful completion.
    ///
    /// @remark Large lists benefit over using the standard addDirectoryItem().
    /// You may call this more than once to add items in chunks.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// if not xbmcplugin.addDirectoryItems(int(sys.argv[1]), [(url, listitem, False,)]: raise
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    bool addDirectoryItems(int handle,
                           const std::vector<Tuple<String,const XBMCAddon::xbmcgui::ListItem*,bool> >& items,
                           int totalItems = 0);

    ///
    /// \ingroup python_xbmcplugin
    /// Callback function to tell Kodi that the end of the directory listing in
    /// a virtualPythonFolder module is reached.
    ///
    /// @param[in] handle               integer - handle the plugin was started
    ///                                 with.
    /// @param[in] succeeded            [opt] bool - True=script completed
    ///                                 successfully(Default)/False=Script did not.
    /// @param[in] updateListing        [opt] bool - True=this folder should
    ///                                 update the current listing/False=Folder
    ///                                 is a subfolder(Default).
    /// @param[in] cacheToDisc          [opt] bool - True=Folder will cache if
    ///                                 extended time(default)/False=this folder
    ///                                 will never cache to disc.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmcplugin.endOfDirectory(int(sys.argv[1]), cacheToDisc=False)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    void endOfDirectory(int handle, bool succeeded = true, bool updateListing = false,
                        bool cacheToDisc = true);

    ///
    /// \ingroup python_xbmcplugin
    /// Callback function to tell Kodi that the file plugin has been resolved to
    /// a url
    ///
    /// @param[in] handle               integer - handle the plugin was started
    ///                                 with.
    /// @param[in] succeeded            bool - True=script completed
    ///                                 successfully/False=Script did not.
    /// @param[in] listitem             ListItem - item the file plugin resolved
    ///                                 to for playback.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmcplugin.setResolvedUrl(int(sys.argv[1]), True, listitem)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    void setResolvedUrl(int handle, bool succeeded, const XBMCAddon::xbmcgui::ListItem* listitem);

    ///
    /// \ingroup python_xbmcplugin
    /// Adds a sorting method for the media list.
    ///
    /// @param[in] handle               integer - handle the plugin was started
    ///                                 with.
    /// @param[in] sortMethod           integer - see available
    ///                                 \ref python_xbmcplugin_sort_method "here on List of sort methods".
    /// @param[in] label2Mask           [opt] string - the label mask to use for
    ///                                 the second label.  Defaults to `%%D`
    ///
    /// @note to add multiple sort methods just call the method multiple times.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmcplugin.addSortMethod(int(sys.argv[1]), xbmcplugin.SORTMETHOD_DATEADDED)
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    void addSortMethod(int handle, int sortMethod, const String& label2Mask = emptyString);

    ///
    /// \ingroup python_xbmcplugin
    /// Returns the value of a setting as a string.
    ///
    /// @param[in] handle               integer - handle the plugin was started
    ///                                 with.
    /// @param[in] id                   string - id of the setting that the
    ///                                 module needs to access.
    /// @return                         Setting value as string
    ///
    /// @note You can use the above as a keyword.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// apikey = xbmcplugin.getSetting(int(sys.argv[1]), 'apikey')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    String getSetting(int handle, const char* id);

    ///
    /// \ingroup python_xbmcplugin
    /// Sets a plugin setting for the current running plugin.
    ///
    /// @param[in] handle    : integer - handle the plugin was started with.
    /// @param[in] id        : string - id of the setting that the module needs to access.
    /// @param[in] value     : string or unicode - value of the setting.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmcplugin.setSetting(int(sys.argv[1]), id='username', value='teamxbmc')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    void setSetting(int handle, const String& id, const String& value);

    ///
    /// \ingroup python_xbmcplugin
    /// Sets the plugins content.
    ///
    /// @param[in] handle      : integer - handle the plugin was started with.
    /// @param[in] content     : string - content type (eg. movies)
    ///
    /// @par Available content strings
    /// |          |          |          |          |
    /// |:--------:|:--------:|:--------:|:--------:|
    /// |  files   |  songs   | artists  | albums
    /// | movies   | tvshows  | episodes | musicvideos
    ///
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmcplugin.setContent(int(sys.argv[1]), 'movies')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    void setContent(int handle, const char* content);

    ///
    /// \ingroup python_xbmcplugin
    /// Sets the plugins name for skins to display.
    ///
    /// @param[in] handle      : integer - handle the plugin was started with.
    /// @param[in] category    : string or unicode - plugins sub category.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmcplugin.setPluginCategory(int(sys.argv[1]), 'Comedy')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    void setPluginCategory(int handle, const String& category);

    ///
    /// \ingroup python_xbmcplugin
    /// Sets the plugins fanart and color for skins to display.
    ///
    /// @param[in] handle      : integer - handle the plugin was started with.
    /// @param[in] image       : [opt] string - path to fanart image.
    /// @param[in] color1      : [opt] hexstring - color1. (e.g. '0xFFFFFFFF')
    /// @param[in] color2      : [opt] hexstring - color2. (e.g. '0xFFFF3300')
    /// @param[in] color3      : [opt] hexstring - color3. (e.g. '0xFF000000')
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmcplugin.setPluginFanart(int(sys.argv[1]), 'special://home/addons/plugins/video/Apple movie trailers II/fanart.png', color2='0xFFFF3300')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    void setPluginFanart(int handle, const char* image = NULL,
                         const char* color1 = NULL,
                         const char* color2 = NULL,
                         const char* color3 = NULL);

    ///
    /// \ingroup python_xbmcplugin
    /// Sets a container property for this plugin.
    ///
    /// @param[in] handle      : integer - handle the plugin was started with.
    /// @param[in] key         : string - property name.
    /// @param[in] value       : string or unicode - value of property.
    ///
    /// @note Key is NOT case sensitive.
    ///
    ///
    /// ------------------------------------------------------------------------
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ..
    /// xbmcplugin.setProperty(int(sys.argv[1]), 'Emulator', 'M.A.M.E.')
    /// ..
    /// ~~~~~~~~~~~~~
    ///
    void setProperty(int handle, const char* key, const String& value);
    ///@}

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    SWIG_CONSTANT(int,SORT_METHOD_NONE);
    SWIG_CONSTANT(int,SORT_METHOD_LABEL);
    SWIG_CONSTANT(int,SORT_METHOD_LABEL_IGNORE_THE);
    SWIG_CONSTANT(int,SORT_METHOD_DATE);
    SWIG_CONSTANT(int,SORT_METHOD_SIZE);
    SWIG_CONSTANT(int,SORT_METHOD_FILE);
    SWIG_CONSTANT(int,SORT_METHOD_DRIVE_TYPE);
    SWIG_CONSTANT(int,SORT_METHOD_TRACKNUM);
    SWIG_CONSTANT(int,SORT_METHOD_DURATION);
    SWIG_CONSTANT(int,SORT_METHOD_TITLE);
    SWIG_CONSTANT(int,SORT_METHOD_TITLE_IGNORE_THE);
    SWIG_CONSTANT(int,SORT_METHOD_ARTIST);
    SWIG_CONSTANT(int,SORT_METHOD_ARTIST_IGNORE_THE);
    SWIG_CONSTANT(int,SORT_METHOD_ALBUM);
    SWIG_CONSTANT(int,SORT_METHOD_ALBUM_IGNORE_THE);
    SWIG_CONSTANT(int,SORT_METHOD_GENRE);
    SWIG_CONSTANT2(int,SORT_METHOD_VIDEO_YEAR,SORT_METHOD_YEAR);
    SWIG_CONSTANT(int,SORT_METHOD_VIDEO_RATING);
    SWIG_CONSTANT(int,SORT_METHOD_PROGRAM_COUNT);
    SWIG_CONSTANT(int,SORT_METHOD_PLAYLIST_ORDER);
    SWIG_CONSTANT(int,SORT_METHOD_EPISODE);
    SWIG_CONSTANT(int,SORT_METHOD_VIDEO_TITLE);
    SWIG_CONSTANT(int,SORT_METHOD_VIDEO_SORT_TITLE);
    SWIG_CONSTANT(int,SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE);
    SWIG_CONSTANT(int,SORT_METHOD_PRODUCTIONCODE);
    SWIG_CONSTANT(int,SORT_METHOD_SONG_RATING);
    SWIG_CONSTANT(int,SORT_METHOD_MPAA_RATING);
    SWIG_CONSTANT(int,SORT_METHOD_VIDEO_RUNTIME);
    SWIG_CONSTANT(int,SORT_METHOD_STUDIO);
    SWIG_CONSTANT(int,SORT_METHOD_STUDIO_IGNORE_THE);
    SWIG_CONSTANT(int,SORT_METHOD_UNSORTED);
    SWIG_CONSTANT(int,SORT_METHOD_BITRATE);
    SWIG_CONSTANT(int,SORT_METHOD_LISTENERS);
    SWIG_CONSTANT(int,SORT_METHOD_COUNTRY);
    SWIG_CONSTANT(int,SORT_METHOD_DATEADDED);
    SWIG_CONSTANT(int,SORT_METHOD_FULLPATH);
    SWIG_CONSTANT(int,SORT_METHOD_LABEL_IGNORE_FOLDERS);
    SWIG_CONSTANT(int,SORT_METHOD_LASTPLAYED);
    SWIG_CONSTANT(int,SORT_METHOD_PLAYCOUNT);
    SWIG_CONSTANT(int,SORT_METHOD_CHANNEL);
    SWIG_CONSTANT(int,SORT_METHOD_DATE_TAKEN);
    SWIG_CONSTANT(int,SORT_METHOD_VIDEO_USER_RATING);
    SWIG_CONSTANT(int,SORT_METHOD_SONG_USER_RATING);
  }
}
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
