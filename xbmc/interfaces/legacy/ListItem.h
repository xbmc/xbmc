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

#pragma once

#include <map>
#include <vector>

#include "AddonClass.h"
#include "Tuple.h"
#include "Dictionary.h"
#include "Alternative.h"
#include "ListItem.h"
#include "FileItem.h"
#include "AddonString.h"
#include "commons/Exception.h"
#include "InfoTagVideo.h"
#include "InfoTagMusic.h"


namespace XBMCAddon
{
  namespace xbmcgui
  {
    XBMCCOMMONS_STANDARD_EXCEPTION(ListItemException);

    // This is a type that represents either a String or a String Tuple
    typedef Alternative<StringOrInt,Tuple<String, StringOrInt> > InfoLabelStringOrTuple;

    // This type is either a String or a list of InfoLabelStringOrTuple types
    typedef Alternative<StringOrInt, std::vector<InfoLabelStringOrTuple> > InfoLabelValue;

    // The type contains the dictionary values for the ListItem::setInfo call.
    // The values in the dictionary can be either a String, or a list of items.
    // If it's a list of items then the items can be either a String or a Tuple.
    typedef Dictionary<InfoLabelValue> InfoLabelDict;

    //
    /// \defgroup python_xbmcgui_listitem ListItem
    /// \ingroup python_xbmcgui
    /// @{
    /// @brief <b>Selectable window list item.</b>
    ///
    /// The list item control is used for creating item lists in Kodi
    ///
    /// <b><c>ListItem([label, label2, iconImage, thumbnailImage, path])</c></b>
    ///
    /// @param[in] label                [opt] string
    /// @param[in] label2               [opt] string
    /// @param[in] iconImage            __Deprecated. Use setArt__
    /// @param[in] thumbnailImage       __Deprecated. Use setArt__
    /// @param[in] path                 [opt] string
    ///
    class ListItem : public AddonClass
    {
    public:
#ifndef SWIG
      CFileItemPtr item;
#endif

      ListItem(const String& label = emptyString,
               const String& label2 = emptyString,
               const String& iconImage = emptyString,
               const String& thumbnailImage = emptyString,
               const String& path = emptyString);

#ifndef SWIG
      inline ListItem(CFileItemPtr pitem) : item(pitem) {}

      static inline AddonClass::Ref<ListItem> fromString(const String& str)
      {
        AddonClass::Ref<ListItem> ret = AddonClass::Ref<ListItem>(new ListItem());
        ret->item.reset(new CFileItem(str));
        return ret;
      }
#endif

      virtual ~ListItem();

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Returns the listitem label.
      ///
      /// @return                       Label of item
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # getLabel()
      /// label = self.list.getSelectedItem().getLabel()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      String getLabel();

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Returns the second listitem label.
      ///
      /// @return                       Second label of item
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # getLabel2()
      /// label = self.list.getSelectedItem().getLabel2()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      String getLabel2();

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Sets the listitem's label.
      ///
      /// @param[in] label              string or unicode - text string.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setLabel(label)
      /// self.list.getSelectedItem().setLabel('Casino Royale')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void setLabel(const String& label);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Sets the listitem's label2.
      ///
      /// @param[in] label              string or unicode - text string.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setLabel2(label)
      /// self.list.getSelectedItem().setLabel2('Casino Royale')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void setLabel2(const String& label);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @warning Deprecated. Use setArt
      ///
      void setIconImage(const String& iconImage);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @warning Deprecated. Use setArt
      ///
      void setThumbnailImage(const String& thumbFilename);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Sets the listitem's art
      ///
      /// @param[in] values             dictionary - pairs of `{ label: value }`.
      ///  - Some default art values (any string possible):
      ///  | Label         | Type                                              |
      ///  |:-------------:|:--------------------------------------------------|
      ///  | thumb         | string - image filename
      ///  | poster        | string - image filename
      ///  | banner        | string - image filename
      ///  | fanart        | string - image filename
      ///  | clearart      | string - image filename
      ///  | clearlogo     | string - image filename
      ///  | landscape     | string - image filename
      ///  | icon          | string - image filename
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setArt(values)
      /// self.list.getSelectedItem().setArt({ 'poster': 'poster.png', 'banner' : 'banner.png' })
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void setArt(const Properties& dictionary);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Sets the listitem's selected status.
      ///
      /// @param[in] selected           bool - True=selected/False=not selected
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # select(selected)
      /// self.list.getSelectedItem().select(True)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void select(bool selected);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Returns the listitem's selected status.
      ///
      ///
      /// @return                       bool - true if selected, otherwise false
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # isSelected()
      /// is = self.list.getSelectedItem().isSelected()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      bool isSelected();

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Sets the listitem's infoLabels.
      ///
      /// @param[in] type               string - type of
      /// @param[in] infoLabels         dictionary - pairs of `{ label: value }`
      ///
      /// __Available types__
      /// | Command name | Description           |
      /// |:------------:|:----------------------|
      /// | video        | Video information
      /// | music        | Music information
      /// | pictures     | Pictures informantion
      ///
      /// @note To set pictures exif info, prepend `exif:` to the label. Exif values must be passed
      ///       as strings, separate value pairs with a comma. <b>(eg. <c>{'exif:resolution': '720,480'}</c></b>
      ///       See \ref kodi_pictures_infotag for valid strings.\n
      ///       \n
      ///       You can use the above as keywords for arguments and skip certain optional arguments.
      ///       Once you use a keyword, all following arguments require the keyword.
      ///
      /// __General Values__ (that apply to all types):
      /// | Info label    | Description                                        |
      /// |--------------:|:---------------------------------------------------|
      /// | count         | integer (12) - can be used to store an id for later, or for sorting purposes
      /// | size          | long (1024) - size in bytes
      /// | date          | string (%d.%m.%Y / 01.01.2009) - file date
      ///
      /// __Video Values__:
      /// | Info label    | Description                                        |
      /// |--------------:|:---------------------------------------------------|
      /// | genre         | string (Comedy)
      /// | year          | integer (2009)
      /// | episode       | integer (4)
      /// | season        | integer (1)
      /// | top250        | integer (192)
      /// | tracknumber   | integer (3)
      /// | rating        | float (6.4) - range is 0..10
      /// | userrating    | integer (9) - range is 1..10
      /// | watched       | depreciated - use playcount instead
      /// | playcount     | integer (2) - number of times this item has been played
      /// | overlay       | integer (2) - range is `0..7`.  See \ref kodi_guilib_listitem_iconoverlay "Overlay icon types" for values
      /// | cast          | list (["Michal C. Hall","Jennifer Carpenter"]) - if provided a list of tuples cast will be interpreted as castandrole
      /// | castandrole   | list of tuples ([("Michael C. Hall","Dexter"),("Jennifer Carpenter","Debra")])
      /// | director      | string (Dagur Kari)
      /// | mpaa          | string (PG-13)
      /// | plot          | string (Long Description)
      /// | plotoutline   | string (Short Description)
      /// | title         | string (Big Fan)
      /// | originaltitle | string (Big Fan)
      /// | sorttitle     | string (Big Fan)
      /// | duration      | integer (245) - duration in seconds
      /// | studio        | string (Warner Bros.)
      /// | tagline       | string (An awesome movie) - short description of movie
      /// | writer        | string (Robert D. Siegel)
      /// | tvshowtitle   | string (Heroes)
      /// | premiered     | string (2005-03-04)
      /// | status        | string (Continuing) - status of a TVshow
      /// | code          | string (tt0110293) - IMDb code
      /// | aired         | string (2008-12-07)
      /// | credits       | string (Andy Kaufman) - writing credits
      /// | lastplayed    | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
      /// | album         | string (The Joshua Tree)
      /// | artist        | list (['U2'])
      /// | votes         | string (12345 votes)
      /// | trailer       | string (/home/user/trailer.avi)
      /// | dateadded     | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
      /// | mediatype     | string - "video", "movie", "tvshow", "season", "episode" or "musicvideo"
      ///
      /// __Music Values__:
      /// | Info label    | Description                                        |
      /// |--------------:|:---------------------------------------------------|
      /// | tracknumber   | integer (8)
      /// | discnumber    | integer (2)
      /// | duration      | integer (245) - duration in seconds
      /// | year          | integer (1998)
      /// | genre         | string (Rock)
      /// | album         | string (Pulse)
      /// | artist        | string (Muse)
      /// | title         | string (American Pie)
      /// | rating        | float - range is between 0 and 10
      /// | userrating    | integer - range is 1..10
      /// | lyrics        | string (On a dark desert highway...)
      /// | playcount     | integer (2) - number of times this item has been played
      /// | lastplayed    | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
      ///
      /// __Picture Values__:
      /// | Info label    | Description                                        |
      /// |--------------:|:---------------------------------------------------|
      /// | title         | string (In the last summer-1)
      /// | picturepath   | string (`/home/username/pictures/img001.jpg`)
      /// | exif*         | string (See \ref kodi_pictures_infotag for valid strings)
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setInfo(type, infoLabels)
      /// self.list.getSelectedItem().setInfo('video', { 'genre': 'Comedy' })
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void setInfo(const char* type, const InfoLabelDict& infoLabels);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Add a stream with details.
      ///
      /// @param[in] type               string - type of stream(video/audio/subtitle).
      /// @param[in] values             dictionary - pairs of { label: value }.
      /// - Video Values:
      ///  | Label       | Type          | Example value     |
      ///  |:-----------:|:-------------:|:------------------|
      ///  | codec       | string        | h264
      ///  | aspect      | float         | 1.78
      ///  | width       | integer       | 1280
      ///  | height      | integer       | 720
      ///  | duration    | integer       | seconds
      ///
      /// - Audio Values:
      ///  | Label       | Type          | Example value     |
      ///  |:-----------:|:-------------:|:------------------|
      ///  | codec       | string        | dts
      ///  | language    | string        | en
      ///  | channels    | integer       | 2
      ///
      /// - Subtitle Values:
      ///  | Label       | Type          | Example value    |
      ///  |:-----------:|:-------------:|:-----------------|
      ///  | language    | string        | en
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # addStreamInfo(type, values)
      /// self.list.getSelectedItem().addStreamInfo('video', { 'codec': 'h264', 'width' : 1280 })
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void addStreamInfo(const char* cType, const Properties& dictionary);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Adds item(s) to the context menu for media lists.
      ///
      /// @param[in] items              list - <b><c>[(label, action,)*]</c></b> A list of tuples consisting of label and action pairs.
      ///                               - `label`          : string or unicode - item's label.
      ///                               - `action`         : string or unicode - any built-in function to perform.
      /// @param[in] replaceItems       [opt] bool - True=only your items will show/False=your items will be added to context menu(Default).
      ///
      /// [List of built-in functions](http://kodi.wiki/view/List_of_Built_In_Functions)
      ///
      /// @note You can use the above as keywords for arguments and skip certain
      ///       optional arguments.\n
      ///       Once you use a keyword, all following arguments require the
      ///       keyword.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # addContextMenuItems([(label, action,)*], replaceItems)
      /// listitem.addContextMenuItems([('Theater Showtimes', 'RunScript(special://home/scripts/showtimes/default.py,Iron Man)',)])
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void addContextMenuItems(const std::vector<Tuple<String,String> >& items, bool replaceItems = false);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Sets a listitem property, similar to an infolabel.
      ///
      /// @param[in] key            string - property name.
      /// @param[in] value          string or unicode - value of property.
      ///
      /// @note Key is NOT case sensitive.
      ///       You can use the above as keywords for arguments and skip certain\n
      ///       optional arguments.\n
      ///       Once you use a keyword, all following arguments require the
      ///       keyword.
      ///
      /// Some of these are treated internally by Kodi, such as the
      /// <b>'StartOffset'</b> property, which is the offset in seconds at which to
      /// start playback of an item.  Others may be used in the skin to add
      /// extra information, such as <b>'WatchedCount'</b> for tvshow items
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setProperty(key, value)
      /// self.list.getSelectedItem().setProperty('AspectRatio', '1.85 : 1')
      /// self.list.getSelectedItem().setProperty('StartOffset', '256.4')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void setProperty(const char* key, const String& value);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Returns a listitem property as a string, similar to an infolabel.
      ///
      /// @param[in] key                string - property name.
      /// @return                       string - List item property
      ///
      /// @note Key is NOT case sensitive.\n
      ///       You can use the above as keywords for arguments and skip certain
      ///       optional arguments.\n
      ///       Once you use a keyword, all following arguments require the
      ///       keyword.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # getProperty(key)
      /// AspectRatio = self.list.getSelectedItem().getProperty('AspectRatio')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      String getProperty(const char* key);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Sets the listitem's path.
      ///
      /// @param[in] path               string or unicode - path, activated when
      ///                               item is clicked.
      ///
      /// @note You can use the above as keywords for arguments.
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setPath(path)
      /// self.list.getSelectedItem().setPath(path='ActivateWindow(Weather)')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void setPath(const String& path);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Sets the listitem's mimetype if known.
      ///
      /// @param[in] mimetype           string or unicode - mimetype.
      ///
      /// If known prehand, this can (but does not have to) avoid HEAD requests
      /// being sent to HTTP servers to figure out file type.
      ///
      void setMimeType(const String& mimetype);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Enable or disable content lookup for item.
      ///
      /// If disabled, HEAD requests to e.g. determine mime type will not be
      /// sent.
      ///
      /// @param[in] enable             bool - True = enable, False = disable
      ///
      void setContentLookup(bool enable);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Sets subtitles for this listitem.
      ///
      /// @param[in] subtitleFiles      string - List of subtitles to add
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// listitem.setSubtitles(['special://temp/example.srt', 'http://example.com/example.srt'])
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      void setSubtitles(const std::vector<String>& subtitleFiles);

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Returns the description of this PlayListItem.
      ///
      /// @return                       Description string
      ///
      String getdescription();

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Returns the duration of this PlayListItem
      ///
      /// @return                       Duration of item as string
      ///
      String getduration();

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Returns the filename of this PlayListItem.
      ///
      /// @return                       Filename string
      ///
      String getfilename();

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Returns the VideoInfoTag for this item.
      ///
      /// @return                       Video info tag
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      xbmc::
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      InfoTagVideo* getVideoInfoTag();

      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief Returns the MusicInfoTag for this item.
      ///
      /// @return                       Music info tag
      ///
#ifndef DOXYGEN_SHOULD_SKIP_THIS
      xbmc::
#endif /* DOXYGEN_SHOULD_SKIP_THIS */
      InfoTagMusic* getMusicInfoTag();
    };
    /// @}

    typedef std::vector<ListItem*> ListItemList;

  }
}


