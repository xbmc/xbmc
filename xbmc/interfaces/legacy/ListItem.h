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
    /// @brief **Selectable window list item.**
    ///
    /// The list item control is used for creating item lists in Kodi
    ///
    /// \python_class{ ListItem([label, label2, iconImage, thumbnailImage, path]) }
    ///
    /// @param label                [opt] string
    /// @param label2               [opt] string
    /// @param iconImage            __Deprecated. Use setArt__
    /// @param thumbnailImage       __Deprecated. Use setArt__
    /// @param path                 [opt] string
    ///
    class ListItem : public AddonClass
    {
    public:
#if !defined SWIG && !defined DOXYGEN_SHOULD_SKIP_THIS
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

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getLabel() }
      ///-----------------------------------------------------------------------
      /// Returns the listitem label.
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
      getLabel();
#else
      String getLabel();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getLabel2() }
      ///-----------------------------------------------------------------------
      /// Returns the second listitem label.
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
      getLabel2();
#else
      String getLabel2();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setLabel(label) }
      ///-----------------------------------------------------------------------
      /// Sets the listitem's label.
      ///
      /// @param label              string or unicode - text string.
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
      setLabel(...);
#else
      void setLabel(const String& label);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setLabel2(label) }
      ///-----------------------------------------------------------------------
      /// Sets the listitem's label2.
      ///
      /// @param label              string or unicode - text string.
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
      setLabel2(...);
#else
      void setLabel2(const String& label);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setIconImage(iconImage) }
      ///-----------------------------------------------------------------------
      /// @warning Deprecated. Use setArt
      ///
      setIconImage(...);
#else
      void setIconImage(const String& iconImage);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setThumbnailImage(thumbFilename) }
      ///-----------------------------------------------------------------------
      /// @warning Deprecated. Use setArt
      ///
      setThumbnailImage(...);
#else
      void setThumbnailImage(const String& thumbFilename);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setArt(values) }
      ///-----------------------------------------------------------------------
      /// Sets the listitem's art
      ///
      /// @param values             dictionary - pairs of `{ label: value }`.
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
      setArt(...);
#else
      void setArt(const Properties& dictionary);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setUniqueIDs(values) }
      ///-----------------------------------------------------------------------
      /// Sets the listitem's uniqueID
      ///
      /// @param values             dictionary - pairs of `{ label: value }`.
      ///  - Some example values (any string possible):
      ///  | Label         | Type                                              |
      ///  |:-------------:|:--------------------------------------------------|
      ///  | imdb          | string - uniqueid name
      ///  | tvdb          | string - uniqueid name
      ///  | tmdb          | string - uniqueid name
      ///  | anidb         | string - uniqueid name
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setUniqueIDs(values)
      /// self.list.getSelectedItem().setUniqueIDs({ 'imdb': 'tt8938399', 'tmdb' : '9837493' })
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setUniqueIDs(...);
#else
      void setUniqueIDs(const Properties& dictionary);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setRating(type, rating, votes = 0, default = True) }
      ///-----------------------------------------------------------------------
      /// Sets a listitem's rating. It needs at least type and rating param
      ///
      /// @param type       string - the type of the rating. Any string.
      /// @param rating     float - the value of the rating.
      /// @param votes      int - the number of votes. Default 0.
      /// @param default    bool - is the default rating?. Default False.
      ///  - Some example type (any string possible):
      ///  | Label         | Type                                              |
      ///  |:-------------:|:--------------------------------------------------|
      ///  | imdb          | string - rating type
      ///  | tvdb          | string - rating type
      ///  | tmdb          | string - rating type
      ///  | anidb         | string - rating type
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setRating(type, rating, votes, default))
      /// self.list.getSelectedItem().setRating("imdb", 4.6, 8940, True)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setRating(...);
#else
      void setRating(std::string type, float rating, int votes = 0, bool def = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getArt(key) }
      ///-----------------------------------------------------------------------
      /// Returns a listitem art path as a string, similar to an infolabel.\n
      ///
      /// @param key            string - art name.
      /// - Some default art values (any string possible):
      ///  | Label         | Type                                             |
      ///  |---------------|--------------------------------------------------|
      ///  | thumb         | string - image path
      ///  | poster        | string - image path
      ///  | banner        | string - image path
      ///  | fanart        | string - image path
      ///  | clearart      | string - image path
      ///  | clearlogo     | string - image path
      ///  | landscape     | string - image path
      ///  | icon          | string - image path
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// poster = self.list.getSelectedItem().getArt('poster')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getArt(key);
#else
      String getArt(const char* key);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getUniqueID(key) }
      ///-----------------------------------------------------------------------
      /// Returns a listitem uniqueID as a string, similar to an infolabel.\n
      ///
      /// @param key            string - uniqueID name.
      /// - Some default uniqueID values (any string possible):
      ///  | Label         | Type                                             |
      ///  |---------------|--------------------------------------------------|
      ///  | imdb          | string - uniqueid name
      ///  | tvdb          | string - uniqueid name
      ///  | tmdb          | string - uniqueid name
      ///  | anidb         | string - uniqueid name
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// uniqueID = self.list.getSelectedItem().getUniqueID('imdb')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getUniqueID(key);
#else
      String getUniqueID(const char* key);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getRating(key) }
      ///-----------------------------------------------------------------------
      /// Returns a listitem rating as a float.\n
      ///
      /// @param key            string - rating type.
      /// - Some default key values (any string possible):
      ///  | Label         | Type                                             |
      ///  |---------------|--------------------------------------------------|
      ///  | imdb          | string - type name
      ///  | tvdb          | string - type name
      ///  | tmdb          | string - type name
      ///  | anidb         | string - type name
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// rating = self.list.getSelectedItem().getRating('imdb')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getRating(key);
#else
      float getRating(const char* key);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getVotes(key) }
      ///-----------------------------------------------------------------------
      /// Returns a listitem votes as a integer.\n
      ///
      /// @param key            string - rating type.
      /// - Some default key values (any string possible):
      ///  | Label         | Type                                             |
      ///  |---------------|--------------------------------------------------|
      ///  | imdb          | string - type name
      ///  | tvdb          | string - type name
      ///  | tmdb          | string - type name
      ///  | anidb         | string - type name
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// votes = self.list.getSelectedItem().getVotes('imdb')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getVotes(key);
#else
      int getVotes(const char* key);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ select(selected) }
      ///-----------------------------------------------------------------------
      /// Sets the listitem's selected status.
      ///
      /// @param selected           bool - True=selected/False=not selected
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
      select(...);
#else
      void select(bool selected);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ isSelected() }
      ///-----------------------------------------------------------------------
      /// Returns the listitem's selected status.
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
      isSelected();
#else
      bool isSelected();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setInfo(type, infoLabels) }
      ///-----------------------------------------------------------------------
      /// Sets the listitem's infoLabels.
      ///
      /// @param type               string - type of
      /// @param infoLabels         dictionary - pairs of `{ label: value }`
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
      /// | country       | string (Germany)
      /// | year          | integer (2009)
      /// | episode       | integer (4)
      /// | season        | integer (1)
      /// | top250        | integer (192)
      /// | setid         | integer (14)
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
      /// | set           | string (Batman Collection) - name of the collection
      /// | imdbnumber    | string (tt0110293) - IMDb code
      /// | code          | string (101) - Production code
      /// | aired         | string (2008-12-07)
      /// | credits       | string (Andy Kaufman) - writing credits
      /// | lastplayed    | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
      /// | album         | string (The Joshua Tree)
      /// | artist        | list (['U2'])
      /// | votes         | string (12345 votes)
      /// | trailer       | string (/home/user/trailer.avi)
      /// | dateadded     | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
      /// | mediatype     | string - "video", "movie", "tvshow", "season", "episode" or "musicvideo"
      /// | dbid          | integer (23) - Only add this for items which are part of the local db. You also need to set the correct 'mediatype'!
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
      /// | mediatype     | string - "music", "song", "album", "artist"
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
      /// self.list.getSelectedItem().setInfo('video', { 'genre': 'Comedy' })
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setInfo(...);
#else
      void setInfo(const char* type, const InfoLabelDict& infoLabels);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ addStreamInfo(type, values) }
      ///-----------------------------------------------------------------------
      /// @brief Add a stream with details.
      ///
      /// @param type              string - type of stream(video/audio/subtitle).
      /// @param values            dictionary - pairs of { label: value }.
      ///
      /// - Video Values:
      /// | Label         | Description                                     |
      /// |--------------:|:------------------------------------------------|
      /// | codec         | string (h264)
      /// | aspect        | float (1.78)
      /// | width         | integer (1280)
      /// | height        | integer (720)
      /// | duration      | integer (seconds)
      ///
      /// - Audio Values:
      /// | Label         | Description                                     |
      /// |--------------:|:------------------------------------------------|
      /// | codec         | string (dts)
      /// | language      | string (en)
      /// | channels      | integer (2)
      ///
      /// - Subtitle Values:
      /// | Label         | Description                                     |
      /// |--------------:|:------------------------------------------------|
      /// | language      | string (en)
      ///
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.list.getSelectedItem().addStreamInfo('video', { 'codec': 'h264', 'width' : 1280 })
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      addStreamInfo(...);
#else
      void addStreamInfo(const char* cType, const Properties& dictionary);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ addContextMenuItems([(label, action,)*], replaceItems) }
      ///-----------------------------------------------------------------------
      /// Adds item(s) to the context menu for media lists.
      ///
      /// @param items               list - [(label, action,)*] A list of tuples consisting of label and action pairs.
      ///   - label           string or unicode - item's label.
      ///   - action          string or unicode - any built-in function to perform.
      /// @param replaceItems        [opt] bool - Deprecated!
      ///
      ///
      /// List of functions - http://kodi.wiki/view/List_of_Built_In_Functions
      ///
      /// @note You can use the above as keywords for arguments and skip certain optional arguments.\n
      /// Once you use a keyword, all following arguments require the keyword.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// listitem.addContextMenuItems([('Theater Showtimes', 'RunScript(special://home/scripts/showtimes/default.py,Iron Man)',)])
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      addContextMenuItems(...);
#else
      void addContextMenuItems(const std::vector<Tuple<String,String> >& items, bool replaceItems = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setProperty(key, value) }
      ///-----------------------------------------------------------------------
      /// Sets a listitem property, similar to an infolabel.
      ///
      /// @param key            string - property name.
      /// @param value          string or unicode - value of property.
      ///
      /// @note Key is NOT case sensitive.\n
      /// You can use the above as keywords for arguments and skip certain optional arguments.\n
      /// Once you use a keyword, all following arguments require the keyword.\n
      /// \n
      /// Some of these are treated internally by Kodi, such as the 'StartOffset' property, which is
      /// the offset in seconds at which to start playback of an item.  Others may be used in the skin
      /// to add extra information, such as 'WatchedCount' for tvshow items
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.list.getSelectedItem().setProperty('AspectRatio', '1.85 : 1')
      /// self.list.getSelectedItem().setProperty('StartOffset', '256.4')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setProperty(...);
#else
      void setProperty(const char * key, const String& value);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getProperty(key) }
      ///-----------------------------------------------------------------------
      /// Returns a listitem property as a string, similar to an infolabel.
      ///
      /// @param key            string - property name.
      ///
      /// @note Key is NOT case sensitive.\n
      ///       You can use the above as keywords for arguments and skip certain optional arguments.\n
      ///       Once you use a keyword, all following arguments require the keyword.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// AspectRatio = self.list.getSelectedItem().getProperty('AspectRatio')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getProperty(...);
#else
      String getProperty(const char* key);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setPath(path) }
      ///-----------------------------------------------------------------------
      /// Sets the listitem's path.
      ///
      /// @param path           string or unicode - path, activated when item is clicked.
      ///
      /// @note You can use the above as keywords for arguments.
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// self.list.getSelectedItem().setPath(path='ActivateWindow(Weather)')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setPath(...);
#else
      void setPath(const String& path);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setMimeType(mimetype) }
      ///-----------------------------------------------------------------------
      /// Sets the listitem's mimetype if known.
      ///
      /// @param mimetype           string or unicode - mimetype
      ///
      /// If known prehand, this can (but does not have to) avoid HEAD requests
      /// being sent to HTTP servers to figure out file type.
      ///
      setMimeType(...);
#else
      void setMimeType(const String& mimetype);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setContentLookup(enable) }
      ///-----------------------------------------------------------------------
      /// Enable or disable content lookup for item.
      ///
      /// If disabled, HEAD requests to e.g determine mime type will not be sent.
      ///
      /// @param enable  bool to enable content lookup
      ///
      setContentLookup(...);
#else
      void setContentLookup(bool enable);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setSubtitles(subtitleFiles) }
      ///-----------------------------------------------------------------------
      /// Sets subtitles for this listitem.
      ///
      /// @param subtitleFiles list with path to subtitle files
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
      setSubtitles(...);
#else
      void setSubtitles(const std::vector<String>& subtitleFiles);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getdescription() }
      ///-----------------------------------------------------------------------
      /// Returns the description of this PlayListItem.
      ///
      /// @return Description string of play list item
      ///
      getdescription();
#else
      String getdescription();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getduration() }
      ///-----------------------------------------------------------------------
      /// Returns the duration of this PlayListItem
      ///
      /// @return duration as string
      ///
      getduration();
#else
      String getduration();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getfilename() }
      ///-----------------------------------------------------------------------
      /// Returns the filename of this PlayListItem.
      ///
      /// @return [string] filename
      ///
      getfilename();
#else
      String getfilename();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getVideoInfoTag() }
      ///-----------------------------------------------------------------------
      /// Returns the VideoInfoTag for this item.
      ///
      /// @return     video info tag
      ///
      getVideoInfoTag();
#else
      xbmc::InfoTagVideo* getVideoInfoTag();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getMusicInfoTag() }
      ///-----------------------------------------------------------------------
      /// Returns the MusicInfoTag for this item.
      ///
      /// @return     music info tag
      ///
      getMusicInfoTag();
#else
      xbmc::InfoTagMusic* getMusicInfoTag();
#endif
    };

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    typedef std::vector<ListItem*> ListItemList;
#endif
  }
}


