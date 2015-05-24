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

      /**
       * getLabel() -- Returns the listitem label.\n
       * \n
       * example:
       *   - label = self.list.getSelectedItem().getLabel()
       */
      String getLabel();

      /**
       * getLabel2() -- Returns the listitem label.\n
       * \n
       * example:
       *   - label = self.list.getSelectedItem().getLabel2()
       */
      String getLabel2();

      /**
       * setLabel(label) -- Sets the listitem's label.\n
       * \n
       * label          : string or unicode - text string.\n
       * \n
       * example:
       *   - self.list.getSelectedItem().setLabel('Casino Royale')
       */
      void setLabel(const String& label);

      /**
       * setLabel2(label) -- Sets the listitem's label2.\n
       * \n
       * label          : string or unicode - text string.\n
       * \n
       * example:
       *   - self.list.getSelectedItem().setLabel2('Casino Royale')
       */
      void setLabel2(const String& label);

      /**
       * setIconImage(icon) -- Sets the listitem's icon image.\n
       * \n
       * icon            : string - image filename.\n
       * \n
       * example:
       *   - self.list.getSelectedItem().setIconImage('emailread.png')
       */
      void setIconImage(const String& iconImage);

      /**
       * setThumbnailImage(thumbFilename) -- Sets the listitem's thumbnail image.\n
       * \n
       * thumb           : string - image filename.\n
       * \n
       * example:
       *   - self.list.getSelectedItem().setThumbnailImage('emailread.png')
       */
      void setThumbnailImage(const String& thumbFilename);

      /**
       * setArt(values) -- Sets the listitem's art
       * \n
       * values              : dictionary - pairs of { label: value }.\n
       *
       * - Some default art values (any string possible):
       *     - thumb         : string - image filename
       *     - poster        : string - image filename
       *     - banner        : string - image filename
       *     - fanart        : string - image filename
       *     - clearart      : string - image filename
       *     - clearlogo     : string - image filename
       *     - landscape     : string - image filename
       *
       * example:
       *   - self.list.getSelectedItem().setArt({ 'poster': 'poster.png', 'banner' : 'banner.png' })
       */
      void setArt(const Properties& dictionary);

      /**
       * select(selected) -- Sets the listitem's selected status.\n
       * \n
       * selected        : bool - True=selected/False=not selected\n
       * \n
       * example:
       *   - self.list.getSelectedItem().select(True)
       */
      void select(bool selected);

      /**
       * isSelected() -- Returns the listitem's selected status.\n
       * \n
       * example:
       *   - is = self.list.getSelectedItem().isSelected()
       */
      bool isSelected();

      /**
       * setInfo(type, infoLabels) -- Sets the listitem's infoLabels.\n
       * \n
       * type              : string - type of media(video/music/pictures).\n
       * infoLabels        : dictionary - pairs of { label: value }.\n
       * \n
       * *Note, To set pictures exif info, prepend 'exif:' to the label. Exif values must be passed\n
       *        as strings, separate value pairs with a comma. (eg. {'exif:resolution': '720,480'}\n
       *        See CPictureInfoTag::TranslateString in PictureInfoTag.cpp for valid strings.\n
       * \n
       *        You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * \n
       * - General Values that apply to all types:
       *     - count         : integer (12) - can be used to store an id for later, or for sorting purposes
       *     - size          : long (1024) - size in bytes
       *     - date          : string (%d.%m.%Y / 01.01.2009) - file date
       * - Video Values:
       *     - genre         : string (Comedy)
       *     - year          : integer (2009)
       *     - episode       : integer (4)
       *     - season        : integer (1)
       *     - top250        : integer (192)
       *     - tracknumber   : integer (3)
       *     - rating        : float (6.4) - range is 0..10
       *     - watched       : depreciated - use playcount instead
       *     - playcount     : integer (2) - number of times this item has been played
       *     - overlay       : integer (2) - range is 0..8.  See GUIListItem.h for values
       *     - cast          : list (Michal C. Hall)
       *     - castandrole   : list (Michael C. Hall|Dexter)
       *     - director      : string (Dagur Kari)
       *     - mpaa          : string (PG-13)
       *     - plot          : string (Long Description)
       *     - plotoutline   : string (Short Description)
       *     - title         : string (Big Fan)
       *     - originaltitle : string (Big Fan)
       *     - sorttitle     : string (Big Fan)
       *     - duration      : integer (245) - duration in seconds
       *     - studio        : string (Warner Bros.)
       *     - tagline       : string (An awesome movie) - short description of movie
       *     - writer        : string (Robert D. Siegel)
       *     - tvshowtitle   : string (Heroes)
       *     - premiered     : string (2005-03-04)
       *     - status        : string (Continuing) - status of a TVshow
       *     - code          : string (tt0110293) - IMDb code
       *     - aired         : string (2008-12-07)
       *     - credits       : string (Andy Kaufman) - writing credits
       *     - lastplayed    : string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
       *     - album         : string (The Joshua Tree)
       *     - artist        : list (['U2'])
       *     - votes         : string (12345 votes)
       *     - trailer       : string (/home/user/trailer.avi)
       *     - dateadded     : string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
       * - Music Values:
       *     - tracknumber   : integer (8)
       *     - discnumber    : integer (2)
       *     - duration      : integer (245) - duration in seconds
       *     - year          : integer (1998)
       *     - genre         : string (Rock)
       *     - album         : string (Pulse)
       *     - artist        : string (Muse)
       *     - title         : string (American Pie)
       *     - rating        : string (3) - single character between 0 and 5
       *     - lyrics        : string (On a dark desert highway...)
       *     - playcount     : integer (2) - number of times this item has been played
       *     - lastplayed    : string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
       * - Picture Values:
       *     - title         : string (In the last summer-1)
       *     - picturepath   : string (/home/username/pictures/img001.jpg)
       *     - exif*         : string (See CPictureInfoTag::TranslateString in PictureInfoTag.cpp for valid strings)
       * 
       * example:\n
       *   - self.list.getSelectedItem().setInfo('video', { 'Genre': 'Comedy' })n\n
       */
      void setInfo(const char* type, const InfoLabelDict& infoLabels);

      /**
       * addStreamInfo(type, values) -- Add a stream with details.\n
       * \n
       * type              : string - type of stream(video/audio/subtitle).\n
       * values            : dictionary - pairs of { label: value }.\n
       * 
       * - Video Values:
       *     - codec         : string (h264)
       *     - aspect        : float (1.78)
       *     - width         : integer (1280)
       *     - height        : integer (720)
       *     - duration      : integer (seconds)
       * - Audio Values:
       *     - codec         : string (dts)
       *     - language      : string (en)
       *     - channels      : integer (2)
       * - Subtitle Values:
       *     - language      : string (en)
       * 
       * example:
       *   - self.list.getSelectedItem().addStreamInfo('video', { 'Codec': 'h264', 'Width' : 1280 })
       */
      void addStreamInfo(const char* cType, const Properties& dictionary);

      /**
       * addContextMenuItems([(label, action,)*], replaceItems) -- Adds item(s) to the context menu for media lists.\n
       * \n
       * items               : list - [(label, action,)*] A list of tuples consisting of label and action pairs.
       *   - label           : string or unicode - item's label.
       *   - action          : string or unicode - any built-in function to perform.
       * replaceItems        : [opt] bool - True=only your items will show/False=your items will be added to context menu(Default).
       * \n
       * List of functions - http://kodi.wiki/view/List_of_Built_In_Functions \n
       * \n
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * \n
       * example:
       *   - listitem.addContextMenuItems([('Theater Showtimes', 'RunScript(special://home/scripts/showtimes/default.py,Iron Man)',)])n
       */
      void addContextMenuItems(const std::vector<Tuple<String,String> >& items, bool replaceItems = false);

      /**
       * setProperty(key, value) -- Sets a listitem property, similar to an infolabel.\n
       * \n
       * key            : string - property name.\n
       * value          : string or unicode - value of property.\n
       * \n
       * *Note, Key is NOT case sensitive.\n
       *        You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * \n
       *  Some of these are treated internally by XBMC, such as the 'StartOffset' property, which is\n
       *  the offset in seconds at which to start playback of an item.  Others may be used in the skin\n
       *  to add extra information, such as 'WatchedCount' for tvshow items\n
       * 
       * example:
       *   - self.list.getSelectedItem().setProperty('AspectRatio', '1.85 : 1')
       *   - self.list.getSelectedItem().setProperty('StartOffset', '256.4')
       */
      void setProperty(const char * key, const String& value);

      /**
       * getProperty(key) -- Returns a listitem property as a string, similar to an infolabel.\n
       * \n
       * key            : string - property name.\n
       * \n
       * *Note, Key is NOT case sensitive.\n
       *        You can use the above as keywords for arguments and skip certain optional arguments.\n
       *        Once you use a keyword, all following arguments require the keyword.\n
       * 
       * example:
       *   - AspectRatio = self.list.getSelectedItem().getProperty('AspectRatio')
       */
      String getProperty(const char* key);

      /**
       * setPath(path) -- Sets the listitem's path.\n
       * \n
       * path           : string or unicode - path, activated when item is clicked.\n
       * \n
       * *Note, You can use the above as keywords for arguments.\n
       * 
       * example:
       *   - self.list.getSelectedItem().setPath(path='ActivateWindow(Weather)')
       */
      void setPath(const String& path);

      /**
       * setMimeType(mimetype) -- Sets the listitem's mimetype if known.\n
       * \n
       * mimetype           : string or unicode - mimetype.\n
       * \n
       * *If known prehand, this can avoid xbmc doing HEAD requests to http servers to figure out file type.\n
       */
      void setMimeType(const String& mimetype);

      /**
       * setSubtitles() -- Sets subtitles for this listitem.\n
       *
       * example:
       *   - listitem.setSubtitles(['special://temp/example.srt', 'http://example.com/example.srt'])
       */
      void setSubtitles(const std::vector<String>& subtitleFiles);

      /**
       * getdescription() -- Returns the description of this PlayListItem.\n
       */
      String getdescription();
      
      /**
       * getduration() -- Returns the duration of this PlayListItem\n
       */
      String getduration();

      /**
       * getfilename() -- Returns the filename of this PlayListItem.\n
       */
      String getfilename();

      /**
       * getVideoInfoTag() -- returns the VideoInfoTag for this item.
       */
      xbmc::InfoTagVideo* getVideoInfoTag();

      /**
       * getMusicInfoTag() -- returns the MusicInfoTag for this item.
       */
      xbmc::InfoTagMusic* getMusicInfoTag();
    };

    typedef std::vector<ListItem*> ListItemList;
    
  }
}


