/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include "Tuple.h"

#include "AddonString.h"
#include "ListItem.h"

#include "utils/log.h"

#include "SortFileItem.h"
#include "swighelper.h"

namespace XBMCAddon
{
  namespace xbmcplugin
  {
    /**
     * <pre>
     * addDirectoryItem(handle, url, listitem [,isFolder, totalItems]) -- Callback function to pass directory contents back to XBMC.
     *  - Returns a bool for successful completion.
     * 
     * handle      : integer - handle the plugin was started with.
     * url         : string - url of the entry. would be plugin:// for another virtual directory
     * listitem    : ListItem - item to add.
     * isFolder    : [opt] bool - True=folder / False=not a folder(default).
     * totalItems  : [opt] integer - total number of items that will be passed.(used for progressbar)
     * 
     * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
     *        Once you use a keyword, all following arguments require the keyword.
     * 
     * example:
     *   - if not xbmcplugin.addDirectoryItem(int(sys.argv[1]), 'F:\\\\Trailers\\\\300.mov', listitem, totalItems=50): break
     * </pre>
     */
    bool addDirectoryItem(int handle, const String& url, const XBMCAddon::xbmcgui::ListItem* listitem,
                          bool isFolder = false, int totalItems = 0);

    /**
     * <pre>
     * addDirectoryItems(handle, items [,totalItems]) -- Callback function to pass directory contents back to XBMC as a list.
     *  - Returns a bool for successful completion.
     * 
     * handle      : integer - handle the plugin was started with.
     * items       : List - list of (url, listitem[, isFolder]) as a tuple to add.
     * totalItems  : [opt] integer - total number of items that will be passed.(used for progressbar)
     * 
     *        Large lists benefit over using the standard addDirectoryItem()
     *        You may call this more than once to add items in chunks
     * 
     * example:
     *   - if not xbmcplugin.addDirectoryItems(int(sys.argv[1]), [(url, listitem, False,)]: raise
     * </pre>
     */
    bool addDirectoryItems(int handle, 
                           const std::vector<Tuple<String,const XBMCAddon::xbmcgui::ListItem*,bool> >& items, 
                           int totalItems = 0);

    /**
     * <pre>
     * endOfDirectory(handle[, succeeded, updateListing, cacheToDisc]) -- Callback function to tell XBMC that the end of the directory listing in a virtualPythonFolder module is reached.
     * 
     * handle           : integer - handle the plugin was started with.
     * succeeded        : [opt] bool - True=script completed successfully(Default)/False=Script did not.
     * updateListing    : [opt] bool - True=this folder should update the current listing/False=Folder is a subfolder(Default).
     * cacheToDisc      : [opt] bool - True=Folder will cache if extended time(default)/False=this folder will never cache to disc.
     * 
     * example:
     *   - xbmcplugin.endOfDirectory(int(sys.argv[1]), cacheToDisc=False)
     * </pre>
     */
    void endOfDirectory(int handle, bool succeeded = true, bool updateListing = false, 
                        bool cacheToDisc = true);

    /**
     * <pre>
     * setResolvedUrl(handle, succeeded, listitem) -- Callback function to tell XBMC that the file plugin has been resolved to a url
     * 
     * handle           : integer - handle the plugin was started with.
     * succeeded        : bool - True=script completed successfully/False=Script did not.
     * listitem         : ListItem - item the file plugin resolved to for playback.
     * 
     * example:
     *   - xbmcplugin.setResolvedUrl(int(sys.argv[1]), True, listitem)
     * </pre>
     */
    void setResolvedUrl(int handle, bool succeeded, const XBMCAddon::xbmcgui::ListItem* listitem);

    /**
     * <pre>
     * addSortMethod(handle, sortMethod, label2) -- Adds a sorting method for the media list.
     * 
     * handle      : integer - handle the plugin was started with.
     * sortMethod  : integer - number for sortmethod see FileItem.h.
     * label2Mask  : [opt] string - the label mask to use for the second label.  Defaults to '%D'
     *               applies to: SORT_METHOD_NONE, SORT_METHOD_UNSORTED, SORT_METHOD_VIDEO_TITLE,
     *                           SORT_METHOD_TRACKNUM, SORT_METHOD_FILE, SORT_METHOD_TITLE
     *                           SORT_METHOD_TITLE_IGNORE_THE, SORT_METHOD_LABEL
     *                           SORT_METHOD_LABEL_IGNORE_THE
     * 
     * example:
     *   - xbmcplugin.addSortMethod(int(sys.argv[1]), xbmcplugin.SORT_METHOD_TITLE)
     * </pre>
     */
    void addSortMethod(int handle, int sortMethod, const String& label2Mask = emptyString);

    /**
     * <pre>
     * getSetting(handle, id) -- Returns the value of a setting as a string.
     * 
     * handle    : integer - handle the plugin was started with.
     * id        : string - id of the setting that the module needs to access.
     * 
     * *Note, You can use the above as a keyword.
     * 
     * example:
     *   - apikey = xbmcplugin.getSetting(int(sys.argv[1]), 'apikey')
     * </pre>
     */
    String getSetting(int handle, const char* id);

    /**
     * <pre>
     * setSetting(handle, id, value) -- Sets a plugin setting for the current running plugin.
     * 
     * handle    : integer - handle the plugin was started with.
     * id        : string - id of the setting that the module needs to access.
     * value     : string or unicode - value of the setting.
     * 
     * example:
     *   - xbmcplugin.setSetting(int(sys.argv[1]), id='username', value='teamxbmc')
     * </pre>
     */
    void setSetting(int handle, const String& id, const String& value);

    /**
     * <pre>
     * setContent(handle, content) -- Sets the plugins content.
     * 
     * handle      : integer - handle the plugin was started with.
     * content     : string - content type (eg. movies)
     * 
     *  *Note:  content: files, songs, artists, albums, movies, tvshows, episodes, musicvideos
     * 
     * example:
     *   - xbmcplugin.setContent(int(sys.argv[1]), 'movies')
     * </pre>
     */
    void setContent(int handle, const char* content);

    /**
     * <pre>
     * setPluginCategory(handle, category) -- Sets the plugins name for skins to display.
     * 
     * handle      : integer - handle the plugin was started with.
     * category    : string or unicode - plugins sub category.
     * 
     * example:
     *   - xbmcplugin.setPluginCategory(int(sys.argv[1]), 'Comedy')
     * </pre>
     */
    void setPluginCategory(int handle, const String& category);

    /**
     * <pre>
     * setPluginFanart(handle, image, color1, color2, color3) -- Sets the plugins fanart and color for skins to display.
     * 
     * handle      : integer - handle the plugin was started with.
     * image       : [opt] string - path to fanart image.
     * color1      : [opt] hexstring - color1. (e.g. '0xFFFFFFFF')
     * color2      : [opt] hexstring - color2. (e.g. '0xFFFF3300')
     * color3      : [opt] hexstring - color3. (e.g. '0xFF000000')
     * 
     * example:
     *   - xbmcplugin.setPluginFanart(int(sys.argv[1]), 'special://home/addons/plugins/video/Apple movie trailers II/fanart.png', color2='0xFFFF3300')\n
     * </pre>
     */
    void setPluginFanart(int handle, const char* image = NULL, 
                         const char* color1 = NULL,
                         const char* color2 = NULL,
                         const char* color3 = NULL);

    /**
     * <pre>
     * setProperty(handle, key, value) -- Sets a container property for this plugin.
     * 
     * handle      : integer - handle the plugin was started with.
     * key         : string - property name.
     * value       : string or unicode - value of property.
     * 
     * *Note, Key is NOT case sensitive.
     * 
     * example:
     *   - xbmcplugin.setProperty(int(sys.argv[1]), 'Emulator', 'M.A.M.E.')\n
     * </pre>
     */
    void setProperty(int handle, const char* key, const String& value);

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
    SWIG_CONSTANT(int,SORT_METHOD_PRODUCTIONCODE);
    SWIG_CONSTANT(int,SORT_METHOD_SONG_RATING);
    SWIG_CONSTANT(int,SORT_METHOD_MPAA_RATING);
    SWIG_CONSTANT(int,SORT_METHOD_VIDEO_RUNTIME);
    SWIG_CONSTANT(int,SORT_METHOD_STUDIO);
    SWIG_CONSTANT(int,SORT_METHOD_STUDIO_IGNORE_THE);
    SWIG_CONSTANT(int,SORT_METHOD_UNSORTED);
    SWIG_CONSTANT(int,SORT_METHOD_BITRATE);
    SWIG_CONSTANT(int,SORT_METHOD_LISTENERS);
  }
}
