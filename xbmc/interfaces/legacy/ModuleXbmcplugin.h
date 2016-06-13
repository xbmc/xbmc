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

#include <vector>

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

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcplugin
    /// @brief \python_func{ xbmcplugin.addDirectoryItem(handle, url, listitem [,isFolder, totalItems]) }
    ///-------------------------------------------------------------------------
    /// Callback function to pass directory contents back to Kodi.
    ///
    /// @param handle               integer - handle the plugin was started
    ///                             with.
    /// @param url                  string - url of the entry. would be
    ///                             `plugin://` for another virtual directory
    /// @param listitem             ListItem - item to add.
    /// @param isFolder             [opt] bool - True=folder / False=not a
    ///                             folder(default).
    /// @param totalItems           [opt] integer - total number of items
    ///                             that will be passed.(used for progressbar)
    /// @return                     Returns a bool for successful completion.
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
    addDirectoryItem(...);
#else
    bool addDirectoryItem(int handle, const String& url, const XBMCAddon::xbmcgui::ListItem* listitem,
                          bool isFolder = false, int totalItems = 0);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcplugin
    /// @brief \python_func{ xbmcplugin.addDirectoryItems(handle, items[, totalItems]) }
    ///-------------------------------------------------------------------------
    /// Callback function to pass directory contents back to Kodi as a list.
    ///
    /// @param handle               integer - handle the plugin was started
    ///                             with.
    /// @param items                List - list of (url, listitem[, isFolder])
    ///                             as a tuple to add.
    /// @param totalItems           [opt] integer - total number of items
    ///                             that will be passed.(used for progressbar)
    /// @return                     Returns a bool for successful completion.
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
     addDirectoryItems(...);
#else
    bool addDirectoryItems(int handle,
                           const std::vector<Tuple<String,const XBMCAddon::xbmcgui::ListItem*,bool> >& items,
                           int totalItems = 0);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcplugin
    /// @brief \python_func{ xbmcplugin.endOfDirectory(handle[, succeeded, updateListing, cacheToDisc]) }
    ///-------------------------------------------------------------------------
    /// Callback function to tell Kodi that the end of the directory listing in
    /// a virtualPythonFolder module is reached.
    ///
    /// @param handle               integer - handle the plugin was started
    ///                             with.
    /// @param succeeded            [opt] bool - True=script completed
    ///                             successfully(Default)/False=Script did not.
    /// @param updateListing        [opt] bool - True=this folder should
    ///                             update the current listing/False=Folder
    ///                             is a subfolder(Default).
    /// @param cacheToDisc          [opt] bool - True=Folder will cache if
    ///                             extended time(default)/False=this folder
    ///                             will never cache to disc.
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
    endOfDirectory(...);
#else
    void endOfDirectory(int handle, bool succeeded = true, bool updateListing = false,
                        bool cacheToDisc = true);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcplugin
    /// @brief \python_func{ xbmcplugin.setResolvedUrl(handle, succeeded, listitem) }
    ///-------------------------------------------------------------------------
    /// Callback function to tell Kodi that the file plugin has been resolved to
    /// a url
    ///
    /// @param handle               integer - handle the plugin was started
    ///                             with.
    /// @param succeeded            bool - True=script completed
    ///                             successfully/False=Script did not.
    /// @param listitem             ListItem - item the file plugin resolved
    ///                             to for playback.
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
    setResolvedUrl(...);
#else
    void setResolvedUrl(int handle, bool succeeded, const XBMCAddon::xbmcgui::ListItem* listitem);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcplugin
    /// @brief \python_func{ xbmcplugin.addSortMethod(handle, sortMethod [,label2Mask]) }
    ///-------------------------------------------------------------------------
    /// Adds a sorting method for the media list.
    ///
    /// @param handle               integer - handle the plugin was started
    ///                             with.
    /// @param sortMethod           integer - see available sort methods at
    ///                             the bottom (or see SortFileItem.h).
    /// | Value                                        | Description           |
    /// |----------------------------------------------|-----------------------|
    /// | xbmcplugin.SORT_METHOD_NONE                  | Do not sort
    /// | xbmcplugin.SORT_METHOD_LABEL                 | Sort by label
    /// | xbmcplugin.SORT_METHOD_LABEL_IGNORE_THE      | Sort by the label and ignore "The" before
    /// | xbmcplugin.SORT_METHOD_DATE                  | Sort by the date
    /// | xbmcplugin.SORT_METHOD_SIZE                  | Sort by the size
    /// | xbmcplugin.SORT_METHOD_FILE                  | Sort by the file
    /// | xbmcplugin.SORT_METHOD_DRIVE_TYPE            | Sort by the drive type
    /// | xbmcplugin.SORT_METHOD_TRACKNUM              | Sort by the track number
    /// | xbmcplugin.SORT_METHOD_DURATION              | Sort by the duration
    /// | xbmcplugin.SORT_METHOD_TITLE                 | Sort by the title
    /// | xbmcplugin.SORT_METHOD_TITLE_IGNORE_THE      | Sort by the title and ignore "The" before
    /// | xbmcplugin.SORT_METHOD_ARTIST                | Sort by the artist
    /// | xbmcplugin.SORT_METHOD_ARTIST_IGNORE_THE     | Sort by the artist and ignore "The" before
    /// | xbmcplugin.SORT_METHOD_ALBUM                 | Sort by the album
    /// | xbmcplugin.SORT_METHOD_ALBUM_IGNORE_THE      | Sort by the album and ignore "The" before
    /// | xbmcplugin.SORT_METHOD_GENRE                 | Sort by the genre
    /// | xbmcplugin.SORT_SORT_METHOD_VIDEO_YEAR, xbmcplugin.SORT_METHOD_YEAR | Sort by the year
    /// | xbmcplugin.SORT_METHOD_VIDEO_RATING          | Sort by the video rating
    /// | xbmcplugin.SORT_METHOD_PROGRAM_COUNT         | Sort by the program count
    /// | xbmcplugin.SORT_METHOD_PLAYLIST_ORDER        | Sort by the playlist order
    /// | xbmcplugin.SORT_METHOD_EPISODE               | Sort by the episode
    /// | xbmcplugin.SORT_METHOD_VIDEO_TITLE           | Sort by the video title
    /// | xbmcplugin.SORT_METHOD_VIDEO_SORT_TITLE      | Sort by the video sort title
    /// | xbmcplugin.SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE |  Sort by the video sort title and ignore "The" before
    /// | xbmcplugin.SORT_METHOD_PRODUCTIONCODE        | Sort by the production code
    /// | xbmcplugin.SORT_METHOD_SONG_RATING           | Sort by the song rating
    /// | xbmcplugin.SORT_METHOD_MPAA_RATING           | Sort by the mpaa rating
    /// | xbmcplugin.SORT_METHOD_VIDEO_RUNTIME         | Sort by video runtime
    /// | xbmcplugin.SORT_METHOD_STUDIO                | Sort by the studio
    /// | xbmcplugin.SORT_METHOD_STUDIO_IGNORE_THE     | Sort by the studio and ignore "The" before
    /// | xbmcplugin.SORT_METHOD_UNSORTED              | Use list not sorted
    /// | xbmcplugin.SORT_METHOD_BITRATE               | Sort by the bitrate
    /// | xbmcplugin.SORT_METHOD_LISTENERS             | Sort by the listeners
    /// | xbmcplugin.SORT_METHOD_COUNTRY               | Sort by the country
    /// | xbmcplugin.SORT_METHOD_DATEADDED             | Sort by the added date
    /// | xbmcplugin.SORT_METHOD_FULLPATH              | Sort by the full path name
    /// | xbmcplugin.SORT_METHOD_LABEL_IGNORE_FOLDERS  | Sort by the label names and ignore related folder names
    /// | xbmcplugin.SORT_METHOD_LASTPLAYED            | Sort by last played date
    /// | xbmcplugin.SORT_METHOD_PLAYCOUNT             | Sort by the play count
    /// | xbmcplugin.SORT_METHOD_CHANNEL               | Sort by the channel
    /// | xbmcplugin.SORT_METHOD_DATE_TAKEN            | Sort by the taken date
    /// | xbmcplugin.SORT_METHOD_VIDEO_USER_RATING     | Sort by the rating of the user of video
    /// | xbmcplugin.SORT_METHOD_SONG_USER_RATING      | Sort by the rating of the user of song
    /// @param label2Mask           [opt] string - the label mask to use for
    ///                             the second label.  Defaults to `%%D`
    /// - applies to:
    /// |                                         |                             |                                         |
    /// |-----------------------------------------|-----------------------------|-----------------------------------------|
    /// | SORT_METHOD_NONE                        | SORT_METHOD_UNSORTED        | SORT_METHOD_VIDEO_TITLE                 |
    /// | SORT_METHOD_TRACKNUM                    | SORT_METHOD_FILE            | SORT_METHOD_TITLE                       |
    /// | SORT_METHOD_TITLE_IGNORE_THE            | SORT_METHOD_LABEL           | SORT_METHOD_LABEL_IGNORE_THE            |
    /// | SORT_METHOD_VIDEO_SORT_TITLE            | SORT_METHOD_FULLPATH        | SORT_METHOD_VIDEO_SORT_TITLE_IGNORE_THE |
    /// | SORT_METHOD_LABEL_IGNORE_FOLDERS        | SORT_METHOD_CHANNEL         |                                         |
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
    addSortMethod(...);
#else
    void addSortMethod(int handle, int sortMethod, const String& label2Mask = emptyString);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcplugin
    /// @brief \python_func{ xbmcplugin.getSetting(handle, id) }
    ///-------------------------------------------------------------------------
    /// Returns the value of a setting as a string.
    ///
    /// @param handle               integer - handle the plugin was started
    ///                             with.
    /// @param id                   string - id of the setting that the
    ///                             module needs to access.
    /// @return                     Setting value as string
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
    getSetting(...);
#else
    String getSetting(int handle, const char* id);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcplugin
    /// @brief \python_func{ xbmcplugin.setSetting(handle, id, value) }
    ///-------------------------------------------------------------------------
    /// Sets a plugin setting for the current running plugin.
    ///
    /// @param handle    integer - handle the plugin was started with.
    /// @param id        string - id of the setting that the module needs to access.
    /// @param value     string or unicode - value of the setting.
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
    setSetting(...);
#else
    void setSetting(int handle, const String& id, const String& value);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcplugin
    /// @brief \python_func{ xbmcplugin.setContent(handle, content) }
    ///-------------------------------------------------------------------------
    /// Sets the plugins content.
    ///
    /// @param handle      integer - handle the plugin was started with.
    /// @param content     string - content type (eg. movies)
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
    setContent(...);
#else
    void setContent(int handle, const char* content);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcplugin
    /// @brief \python_func{ xbmcplugin.setPluginCategory(handle, category) }
    ///-------------------------------------------------------------------------
    /// Sets the plugins name for skins to display.
    ///
    /// @param handle      integer - handle the plugin was started with.
    /// @param category    string or unicode - plugins sub category.
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
    setPluginCategory(...);
#else
    void setPluginCategory(int handle, const String& category);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcplugin
    /// @brief \python_func{ xbmcplugin.setPluginFanart(handle, image, color1, color2, color3) }
    ///-------------------------------------------------------------------------
    /// Sets the plugins fanart and color for skins to display.
    ///
    /// @param handle      integer - handle the plugin was started with.
    /// @param image       [opt] string - path to fanart image.
    /// @param color1      [opt] hexstring - color1. (e.g. '0xFFFFFFFF')
    /// @param color2      [opt] hexstring - color2. (e.g. '0xFFFF3300')
    /// @param color3      [opt] hexstring - color3. (e.g. '0xFF000000')
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
    setPluginFanart(...);
#else
    void setPluginFanart(int handle, const char* image = NULL,
                         const char* color1 = NULL,
                         const char* color2 = NULL,
                         const char* color3 = NULL);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcplugin
    /// @brief \python_func{ xbmcplugin.setProperty(handle, key, value) }
    ///-------------------------------------------------------------------------
    /// Sets a container property for this plugin.
    ///
    /// @param handle      integer - handle the plugin was started with.
    /// @param key         string - property name.
    /// @param value       string or unicode - value of property.
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
    setProperty(...);
    ///@}
#else
    void setProperty(int handle, const char* key, const String& value);
#endif

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
