/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "AddonClass.h"
#include "AddonString.h"
#include "Alternative.h"
#include "Dictionary.h"
#include "FileItem.h"
#include "InfoTagGame.h"
#include "InfoTagMusic.h"
#include "InfoTagPicture.h"
#include "InfoTagVideo.h"
#include "ListItem.h"
#include "Tuple.h"
#include "commons/Exception.h"

#include <map>
#include <memory>
#include <utility>
#include <vector>


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
    class ListItem : public AddonClass
    {
    public:
#if !defined SWIG && !defined DOXYGEN_SHOULD_SKIP_THIS
      CFileItemPtr item;
      bool m_offscreen;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
    ///
    /// \ingroup python_xbmcgui_listitem
    /// @brief Selectable window list item.
    ///
    /// The list item control is used for creating item lists in Kodi
    ///
    /// \python_class{ ListItem([label, label2, path, offscreen]) }
    ///
    /// @param label                [opt] string (default `""`) - the label to display on the item
    /// @param label2               [opt] string (default `""`) - the label2 of the item
    /// @param path                 [opt] string (default `""`) - the path for the item
    /// @param offscreen            [opt] bool (default `False`) - if GUI based locks should be
    ///                                          avoided. Most of the times listitems are created
    ///                                          offscreen and added later to a container
    ///                                          for display (e.g. plugins) or they are not
    ///                                          even displayed (e.g. python scrapers).
    ///                                          In such cases, there is no need to lock the
    ///                                          GUI when creating the items (increasing your addon
    ///                                          performance).
    ///                                          Note however, that if you are creating listitems
    ///                                          and managing the container itself (e.g using
    ///                                          WindowXML or WindowXMLDialog classes) subsquent
    ///                                          modifications to the item will require locking.
    ///                                          Thus, in such cases, use the default value (`False`).
    ///
    ///
    ///-----------------------------------------------------------------------
    /// @python_v16 **iconImage** and **thumbnailImage** are deprecated. Use **setArt()**.
    /// @python_v18 Added **offscreen** argument.
    /// @python_v19 Removed **iconImage** and **thumbnailImage**. Use **setArt()**.
    ///
    /// **Example:**
    /// ~~~~~~~~~~~~~{.py}
    /// ...
    /// listitem = xbmcgui.ListItem('Casino Royale')
    /// ...
    /// ~~~~~~~~~~~~~
    ///
    ListItem([label, label2, path, offscreen]);
#else
    ListItem(const String& label = emptyString,
             const String& label2 = emptyString,
             const String& path = emptyString,
             bool offscreen = false);
#endif

#if !defined SWIG && !defined DOXYGEN_SHOULD_SKIP_THIS
      inline explicit ListItem(CFileItemPtr pitem) : item(std::move(pitem)), m_offscreen(false) {}

      static inline AddonClass::Ref<ListItem> fromString(const String& str)
      {
        AddonClass::Ref<ListItem> ret = AddonClass::Ref<ListItem>(new ListItem());
        ret->item = std::make_shared<CFileItem>(str);
        return ret;
      }
#endif

#ifndef DOXYGEN_SHOULD_SKIP_THIS
      ~ListItem() override;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getLabel() }
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
      /// label = listitem.getLabel()
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
      /// label = listitem.getLabel2()
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
      /// listitem.setLabel('Casino Royale')
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
      /// listitem.setLabel2('Casino Royale')
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
      /// @brief \python_func{ getDateTime() }
      /// Returns the list item's datetime in W3C format (YYYY-MM-DDThh:mm:ssTZD).
      ///
      /// @return                   string or unicode - datetime string (W3C).
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # getDateTime()
      /// strDateTime = listitem.getDateTime()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      getDateTime();
#else
      String getDateTime();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setDateTime(dateTime) }
      /// Sets the list item's datetime in W3C format.
      /// The following formats are supported:
      /// - YYYY
      /// - YYYY-MM-DD
      /// - YYYY-MM-DDThh:mm[TZD]
      /// - YYYY-MM-DDThh:mm:ss[TZD]
      /// where the timezone (TZD) is always optional and can be in one of the
      /// following formats:
      /// - Z (for UTC)
      /// - +hh:mm
      /// - -hh:mm
      ///
      /// @param label              string or unicode - datetime string (W3C).
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setDate(dateTime)
      /// listitem.setDateTime('2021-03-09T12:30:00Z')
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setDateTime(...);
#else
      void setDateTime(const String& dateTime);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setArt(values) }
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
      /// @python_v13 New function added.
      /// @python_v16 Added new label **icon**.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setArt(values)
      /// listitem.setArt({ 'poster': 'poster.png', 'banner' : 'banner.png' })
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
      /// @brief \python_func{ setIsFolder(isFolder) }
      /// Sets if this listitem is a folder.
      ///
      /// @param isFolder            bool - True=folder / False=not a folder (default).
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// @python_v18 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setIsFolder(isFolder)
      /// listitem.setIsFolder(True)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setIsFolder(...);
#else
      void setIsFolder(bool isFolder);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setUniqueIDs(values, defaultrating) }
      /// Sets the listitem's uniqueID
      ///
      /// @param values             dictionary - pairs of `{ label: value }`.
      /// @param defaultrating      [opt] string - the name of default rating.
      ///
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
      /// @python_v20 Deprecated. Use **InfoTagVideo.setUniqueIDs()** instead.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setUniqueIDs(values, defaultrating)
      /// listitem.setUniqueIDs({ 'imdb': 'tt8938399', 'tmdb' : '9837493' }, "imdb")
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setUniqueIDs(...);
#else
      void setUniqueIDs(const Properties& dictionary, const String& defaultrating = "");
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setRating(type, rating, votes = 0, defaultt = False) }
      /// Sets a listitem's rating. It needs at least type and rating param
      ///
      /// @param type       string - the type of the rating. Any string.
      /// @param rating     float - the value of the rating.
      /// @param votes      int - the number of votes. Default 0.
      /// @param defaultt   bool - is the default rating?. Default False.
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
      /// @python_v20 Deprecated. Use **InfoTagVideo.setRating()** instead.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setRating(type, rating, votes, defaultt))
      /// listitem.setRating("imdb", 4.6, 8940, True)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setRating(...);
#else
      void setRating(const std::string& type, float rating, int votes = 0, bool defaultt = false);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ addSeason(number, name = "") }
      /// Add a season with name to a listitem. It needs at least the season number
      ///
      /// @param number     int - the number of the season.
      /// @param name       string - the name of the season. Default "".
      ///
      ///
      ///-----------------------------------------------------------------------
      ///
      /// @python_v18 New function added.
      /// @python_v20 Deprecated. Use **InfoTagVideo.addSeason()** or **InfoTagVideo.addSeasons()** instead.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # addSeason(number, name))
      /// listitem.addSeason(1, "Murder House")
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      addSeason(...);
#else
      void addSeason(int number, std::string name = "");
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getArt(key) }
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
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// poster = listitem.getArt('poster')
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
      /// @brief \python_func{ isFolder() }
      /// Returns whether the item is a folder or not.
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// isFolder = listitem.isFolder()
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      isFolder();
#else
      bool isFolder() const;
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getUniqueID(key) }
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
      /// @python_v20 Deprecated. Use **InfoTagVideo.getUniqueID()** instead.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// uniqueID = listitem.getUniqueID('imdb')
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
      /// @python_v20 Deprecated. Use **InfoTagVideo.getRating()** instead.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// rating = listitem.getRating('imdb')
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
      /// @python_v20 Deprecated. Use **InfoTagVideo.getVotesAsInt()** instead.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// votes = listitem.getVotes('imdb')
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
      /// listitem.select(True)
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
      /// selected = listitem.isSelected()
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
      /// Sets the listitem's infoLabels.
      ///
      /// @param type               string - type of info labels
      /// @param infoLabels         dictionary - pairs of `{ label: value }`
      ///
      /// __Available types__
      /// | Command name | Description           |
      /// |:------------:|:----------------------|
      /// | video        | Video information
      /// | music        | Music information
      /// | pictures     | Pictures informanion
      /// | game         | Game information
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
      /// | genre         | string (Comedy) or list of strings (["Comedy", "Animation", "Drama"])
      /// | country       | string (Germany) or list of strings (["Germany", "Italy", "France"])
      /// | year          | integer (2009)
      /// | episode       | integer (4)
      /// | season        | integer (1)
      /// | sortepisode   | integer (4)
      /// | sortseason    | integer (1)
      /// | episodeguide  | string (Episode guide)
      /// | showlink      | string (Battlestar Galactica) or list of strings (["Battlestar Galactica", "Caprica"])
      /// | top250        | integer (192)
      /// | setid         | integer (14)
      /// | tracknumber   | integer (3)
      /// | rating        | float (6.4) - range is 0..10
      /// | userrating    | integer (9) - range is 1..10 (0 to reset)
      /// | watched       | deprecated - use playcount instead
      /// | playcount     | integer (2) - number of times this item has been played
      /// | overlay       | integer (2) - range is `0..7`.  See \ref kodi_guilib_listitem_iconoverlay "Overlay icon types" for values
      /// | cast          | list (["Michal C. Hall","Jennifer Carpenter"]) - if provided a list of tuples cast will be interpreted as castandrole
      /// | castandrole   | list of tuples ([("Michael C. Hall","Dexter"),("Jennifer Carpenter","Debra")])
      /// | director      | string (Dagur Kari) or list of strings (["Dagur Kari", "Quentin Tarantino", "Chrstopher Nolan"])
      /// | mpaa          | string (PG-13)
      /// | plot          | string (Long Description)
      /// | plotoutline   | string (Short Description)
      /// | title         | string (Big Fan)
      /// | originaltitle | string (Big Fan)
      /// | sorttitle     | string (Big Fan)
      /// | duration      | integer (245) - duration in seconds
      /// | studio        | string (Warner Bros.) or list of strings (["Warner Bros.", "Disney", "Paramount"])
      /// | tagline       | string (An awesome movie) - short description of movie
      /// | writer        | string (Robert D. Siegel) or list of strings (["Robert D. Siegel", "Jonathan Nolan", "J.K. Rowling"])
      /// | tvshowtitle   | string (Heroes)
      /// | premiered     | string (2005-03-04)
      /// | status        | string (Continuing) - status of a TVshow
      /// | set           | string (Batman Collection) - name of the collection
      /// | setoverview   | string (All Batman movies) - overview of the collection
      /// | tag           | string (cult) or list of strings (["cult", "documentary", "best movies"]) - movie tag
      /// | videoversion  | string (Video version)
      /// | imdbnumber    | string (tt0110293) - IMDb code
      /// | code          | string (101) - Production code
      /// | aired         | string (2008-12-07)
      /// | credits       | string (Andy Kaufman) or list of strings (["Dagur Kari", "Quentin Tarantino", "Chrstopher Nolan"]) - writing credits
      /// | lastplayed    | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
      /// | album         | string (The Joshua Tree)
      /// | artist        | list (['U2'])
      /// | votes         | string (12345 votes)
      /// | path          | string (/home/user/movie.avi)
      /// | trailer       | string (/home/user/trailer.avi)
      /// | dateadded     | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
      /// | mediatype     | string - "video", "movie", "tvshow", "season", "episode" or "musicvideo"
      /// | dbid          | integer (23) - Only add this for items which are part of the local db. You also need to set the correct 'mediatype'!
      ///
      /// __Music Values__:
      /// | Info label               | Description                                        |
      /// |-------------------------:|:---------------------------------------------------|
      /// | tracknumber              | integer (8)
      /// | discnumber               | integer (2)
      /// | duration                 | integer (245) - duration in seconds
      /// | year                     | integer (1998)
      /// | genre                    | string (Rock)
      /// | album                    | string (Pulse)
      /// | artist                   | string (Muse)
      /// | title                    | string (American Pie)
      /// | rating                   | float - range is between 0 and 10
      /// | userrating               | integer - range is 1..10
      /// | lyrics                   | string (On a dark desert highway...)
      /// | playcount                | integer (2) - number of times this item has been played
      /// | lastplayed               | string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
      /// | mediatype                | string - "music", "song", "album", "artist"
      /// | dbid                     | integer (23) - Only add this for items which are part of the local db. You also need to set the correct 'mediatype'!
      /// | listeners                | integer (25614)
      /// | musicbrainztrackid       | string (cd1de9af-0b71-4503-9f96-9f5efe27923c)
      /// | musicbrainzartistid      | string (d87e52c5-bb8d-4da8-b941-9f4928627dc8)
      /// | musicbrainzalbumid       | string (24944755-2f68-3778-974e-f572a9e30108)
      /// | musicbrainzalbumartistid | string (d87e52c5-bb8d-4da8-b941-9f4928627dc8)
      /// | comment                  | string (This is a great song)
      ///
      /// __Picture Values__:
      /// | Info label    | Description                                        |
      /// |--------------:|:---------------------------------------------------|
      /// | title         | string (In the last summer-1)
      /// | picturepath   | string (`/home/username/pictures/img001.jpg`)
      /// | exif*         | string (See \ref kodi_pictures_infotag for valid strings)
      ///
      /// __Game Values__:
      /// | Info label    | Description                                        |
      /// |--------------:|:---------------------------------------------------|
      /// | title         | string (Super Mario Bros.)
      /// | platform      | string (Atari 2600)
      /// | genres        | list (["Action","Strategy"])
      /// | publisher     | string (Nintendo)
      /// | developer     | string (Square)
      /// | overview      | string (Long Description)
      /// | year          | integer (1980)
      /// | gameclient    | string (game.libretro.fceumm)
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v14 Added new label **discnumber**.
      /// @python_v15 **duration** has to be set in seconds.
      /// @python_v16 Added new label **mediatype**.
      /// @python_v17
      /// Added labels **setid**, **set**, **imdbnumber**, **code**, **dbid** (video), **path** and **userrating**.
      /// Expanded the possible infoLabels for the option **mediatype**.
      /// @python_v18 Added new **game** type and associated infolabels.
      /// Added labels **dbid** (music), **setoverview**, **tag**, **sortepisode**, **sortseason**, **episodeguide**, **showlink**.
      /// Extended labels **genre**, **country**, **director**, **studio**, **writer**, **tag**, **credits** to also use a list of strings.
      /// @python_v20 Partially deprecated. Use explicit setters in **InfoTagVideo**, **InfoTagMusic**, **InfoTagPicture** or **InfoTagGame** instead.
      /// @python_v21 Added videoversion infolabel for movies
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// listitem.setInfo('video', { 'genre': 'Comedy' })
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
      /// @brief \python_func{ setCast(actors) }
      /// Set cast including thumbnails
      ///
      /// @param actors            list of dictionaries (see below for relevant keys)
      ///
      /// - Keys:
      /// | Label         | Description                                     |
      /// |--------------:|:------------------------------------------------|
      /// | name          | string (Michael C. Hall)
      /// | role          | string (Dexter)
      /// | thumbnail     | string (http://www.someurl.com/someimage.png)
      /// | order         | integer (1)
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      /// @python_v20 Deprecated. Use **InfoTagVideo.setCast()** instead.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// actors = [{"name": "Actor 1", "role": "role 1"}, {"name": "Actor 2", "role": "role 2"}]
      /// listitem.setCast(actors)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setCast(...);
#else
      void setCast(const std::vector<Properties>& actors);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setAvailableFanart(images) }
      /// Set available images (needed for video scrapers)
      ///
      /// @param images            list of dictionaries (see below for relevant keys)
      ///
      /// - Keys:
      /// | Label         | Description                                     |
      /// |--------------:|:------------------------------------------------|
      /// | image         | string (http://www.someurl.com/someimage.png)
      /// | preview       | [opt] string (http://www.someurl.com/somepreviewimage.png)
      /// | colors        | [opt] string (either comma separated Kodi hex values ("FFFFFFFF,DDDDDDDD") or TVDB RGB Int Triplets ("|68,69,59|69,70,58|78,78,68|"))
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// fanart = [{"image": path_to_image_1, "preview": path_to_preview_1}, {"image": path_to_image_2, "preview": path_to_preview_2}]
      /// listitem.setAvailableFanart(fanart)
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setAvailableFanart(...);
#else
      void setAvailableFanart(const std::vector<Properties>& images);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ addAvailableArtwork(images) }
      /// Add an image to available artworks (needed for video scrapers)
      ///
      /// @param url            string (image path url)
      /// @param art_type       string (image type)
      /// @param preview        [opt] string (image preview path url)
      /// @param referrer       [opt] string (referrer url)
      /// @param cache          [opt] string (filename in cache)
      /// @param post           [opt] bool (use post to retrieve the image, default false)
      /// @param isgz           [opt] bool (use gzip to retrieve the image, default false)
      /// @param season         [opt] integer (number of season in case of season thumb)
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v18 New function added.
      /// @python_v19 New param added (preview).
      /// @python_v20 Deprecated. Use **InfoTagVideo.addAvailableArtwork()** instead.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// listitem.addAvailableArtwork(path_to_image_1, "thumb")
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      addAvailableArtwork(...);
#else
      void addAvailableArtwork(const std::string& url,
                               const std::string& art_type = "",
                               const std::string& preview = "",
                               const std::string& referrer = "",
                               const std::string& cache = "",
                               bool post = false,
                               bool isgz = false,
                               int season = -1);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ addStreamInfo(type, values) }
      /// Add a stream with details.
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
      /// @python_v20 Deprecated. Use **InfoTagVideo.addVideoStream()**, **InfoTagVideo.addAudioStream()** or **InfoTagVideo.addSubtitleStream()** instead.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// listitem.addStreamInfo('video', { 'codec': 'h264', 'width' : 1280 })
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
      /// @brief \python_func{ addContextMenuItems([(label, action),*]) }
      /// Adds item(s) to the context menu for media lists.
      ///
      /// @param items               list - [(label, action),*] A list of tuples consisting of label and action pairs.
      ///   - label           string or unicode - item's label.
      ///   - action          string or unicode - any available \link page_List_of_built_in_functions built-in function \endlink .
      ///
      /// @note You can use the above as keywords for arguments and skip certain optional arguments.\n
      /// Once you use a keyword, all following arguments require the keyword.
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 Completely removed previously available argument **replaceItems**.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// listitem.addContextMenuItems([('Theater Showtimes', 'RunScript(script.myaddon,title=Iron Man)')])
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
      /// - **Internal Properties**
      /// | Key           | Description                                     |
      /// |--------------:|:------------------------------------------------|
      /// | inputstream   | string (inputstream.adaptive) - Set the inputstream add-on that will be used to play the item
      /// | IsPlayable    | string - "true", "false" - Mark the item as playable, **mandatory for playable items**
      /// | MimeType      | string (application/x-mpegURL) - Set the MimeType of the item before playback
      /// | ResumeTime    | float (1962.0) - Set the resume point of the item in seconds
      /// | SpecialSort   | string - "top", "bottom" - The item will remain at the top or bottom of the current list
      /// | StartOffset   | float (60.0) - Set the offset in seconds at which to start playback of the item
      /// | StartPercent  | float (15.0) - Set the percentage at which to start playback of the item
      /// | StationName   | string ("My Station Name") - Used to enforce/override MusicPlayer.StationName infolabel from addons (e.g. in radio addons)
      /// | TotalTime     | float (7848.0) - Set the total time of the item in seconds
      /// | OverrideInfotag | string - "true", "false" - When true will override all info from previous listitem
      /// | ForceResolvePlugin | string - "true", "false" - When true ensures that a plugin will always receive callbacks to resolve paths (useful for playlist cases)
      /// | rtsp_transport | string - "udp", "udp_multicast" or "tcp" - Allow to force the rtsp transport mode for rtsp streams
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 OverrideInfotag property added
      /// @python_v20 **ResumeTime** and **TotalTime** deprecated. Use **InfoTagVideo.setResumePoint()** instead.
      /// @python_v20 ForceResolvePlugin property added
      /// @python_v20 rtsp_transport property added
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// listitem.setProperty('AspectRatio', '1.85 : 1')
      /// listitem.setProperty('StartOffset', '256.4')
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
      /// @brief \python_func{ setProperties(values) }
      /// Sets multiple properties for listitem's
      ///
      /// @param values             dictionary - pairs of `{ label: value }`.
      ///
      /// @python_v18 New function added.
      ///
      ///-----------------------------------------------------------------------
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// # setProperties(values)
      /// listitem.setProperties({ 'AspectRatio': '1.85', 'StartOffset' : '256.4' })
      /// ...
      /// ~~~~~~~~~~~~~
      ///
      setProperties(...);
#else
      void setProperties(const Properties& dictionary);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getProperty(key) }
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
      /// @python_v20 **ResumeTime** and **TotalTime** deprecated. Use **InfoTagVideo.getResumeTime()** and **InfoTagVideo.getResumeTimeTotal()** instead.
      ///
      /// **Example:**
      /// ~~~~~~~~~~~~~{.py}
      /// ...
      /// AspectRatio = listitem.getProperty('AspectRatio')
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
      /// listitem.setPath(path='/path/to/some/file.ext')
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
      /// Enable or disable content lookup for item.
      ///
      /// If disabled, HEAD requests to e.g determine mime type will not be sent.
      ///
      /// @param enable  bool to enable content lookup
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v16 New function added.
      ///
      setContentLookup(...);
#else
      void setContentLookup(bool enable);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ setSubtitles(subtitleFiles) }
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
      ///-----------------------------------------------------------------------
      /// @python_v14 New function added.
      ///
      setSubtitles(...);
#else
      void setSubtitles(const std::vector<String>& subtitleFiles);
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getPath() }
      /// Returns the path of this listitem.
      ///
      /// @return [string] filename
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v17 New function added.
      ///
      ///
      getPath();
#else
      String getPath();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getVideoInfoTag() }
      /// Returns the VideoInfoTag for this item.
      ///
      /// @return     video info tag
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v15 New function added.
      ///
      getVideoInfoTag();
#else
      xbmc::InfoTagVideo* getVideoInfoTag();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getMusicInfoTag() }
      /// Returns the MusicInfoTag for this item.
      ///
      /// @return     music info tag
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v15 New function added.
      ///
      getMusicInfoTag();
#else
      xbmc::InfoTagMusic* getMusicInfoTag();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getPictureInfoTag() }
      /// Returns the InfoTagPicture for this item.
      ///
      /// @return     picture info tag
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getPictureInfoTag();
#else
      xbmc::InfoTagPicture* getPictureInfoTag();
#endif

#ifdef DOXYGEN_SHOULD_USE_THIS
      ///
      /// \ingroup python_xbmcgui_listitem
      /// @brief \python_func{ getGameInfoTag() }
      /// Returns the InfoTagGame for this item.
      ///
      /// @return     game info tag
      ///
      ///
      ///-----------------------------------------------------------------------
      /// @python_v20 New function added.
      ///
      getGameInfoTag();
#else
      xbmc::InfoTagGame* getGameInfoTag();
#endif

private:
      std::vector<std::string> getStringArray(const InfoLabelValue& alt,
                                              const std::string& tag,
                                              std::string value,
                                              const std::string& separator);
      std::vector<std::string> getVideoStringArray(const InfoLabelValue& alt,
                                                   const std::string& tag,
                                                   std::string value = "");
      std::vector<std::string> getMusicStringArray(const InfoLabelValue& alt,
                                                   const std::string& tag,
                                                   std::string value = "");

      CVideoInfoTag* GetVideoInfoTag();
      const CVideoInfoTag* GetVideoInfoTag() const;

      MUSIC_INFO::CMusicInfoTag* GetMusicInfoTag();
      const MUSIC_INFO::CMusicInfoTag* GetMusicInfoTag() const;

      void setTitleRaw(std::string title);
      void setPathRaw(const std::string& path);
      void setCountRaw(int count);
      void setSizeRaw(int64_t size);
      void setDateTimeRaw(const std::string& dateTime);
      void setIsFolderRaw(bool isFolder);
      void setStartOffsetRaw(double startOffset);
      void setMimeTypeRaw(const std::string& mimetype);
      void setSpecialSortRaw(std::string specialSort);
      void setContentLookupRaw(bool enable);
      void addArtRaw(std::string type, const std::string& url);
      void addPropertyRaw(std::string type, const CVariant& value);
      void addSubtitlesRaw(const std::vector<std::string>& subtitles);
    };

#ifndef DOXYGEN_SHOULD_SKIP_THIS
    typedef std::vector<ListItem*> ListItemList;
#endif
  }
}
