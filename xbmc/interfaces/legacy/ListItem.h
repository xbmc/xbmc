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

#pragma once

#include <map>
#include <vector>

#include "cores/playercorefactory/PlayerCoreFactory.h"

#include "AddonClass.h"
#include "Dictionary.h"
#include "CallbackHandler.h"
#include "ListItem.h"
#include "music/tags/MusicInfoTag.h"
#include "FileItem.h"
#include "AddonString.h"
#include "Tuple.h"
#include "commons/Exception.h"

#include <map>

namespace XBMCAddon
{
  namespace xbmcgui
  {
    XBMCCOMMONS_STANDARD_EXCEPTION(ListItemException);

    class ListItem : public AddonClass
    {
    public:
      CFileItemPtr item;

      ListItem(const String& label = emptyString, 
               const String& label2 = emptyString,
               const String& iconImage = emptyString,
               const String& thumbnailImage = emptyString,
               const String& path = emptyString);

#ifndef SWIG
      inline ListItem(CFileItemPtr pitem) : AddonClass("ListItem"), item(pitem) {}
#endif

      virtual ~ListItem();

      static inline ListItem* fromString(const String& str) 
      { 
        ListItem* ret = new ListItem();
        ret->item.reset(new CFileItem(str));
        return ret;
      }

      /**
       * getLabel() -- Returns the listitem label.
       * 
       * example:
       *   - label = self.list.getSelectedItem().getLabel()
       */
      String getLabel();

      /**
       * getLabel2() -- Returns the listitem label.
       * 
       * example:
       *   - label = self.list.getSelectedItem().getLabel2()
       */
      String getLabel2();

      /**
       * setLabel(label) -- Sets the listitem's label.
       * 
       * label          : string or unicode - text string.
       * 
       * example:
       *   - self.list.getSelectedItem().setLabel('Casino Royale')
       */
      void setLabel(const String& label);

      /**
       * setLabel2(label) -- Sets the listitem's label2.
       * 
       * label          : string or unicode - text string.
       * 
       * example:
       *   - self.list.getSelectedItem().setLabel2('Casino Royale')
       */
      void setLabel2(const String& label);

      /**
       * setIconImage(icon) -- Sets the listitem's icon image.
       * 
       * icon            : string - image filename.
       * 
       * example:
       *   - self.list.getSelectedItem().setIconImage('emailread.png')
       */
      void setIconImage(const String& iconImage);

      /**
       * setThumbnailImage(thumbFilename) -- Sets the listitem's thumbnail image.
       * 
       * thumb           : string - image filename.
       * 
       * example:
       *   - self.list.getSelectedItem().setThumbnailImage('emailread.png')
       */
      void setThumbnailImage(const String& thumbFilename);

      /**
       * select(selected) -- Sets the listitem's selected status.
       * 
       * selected        : bool - True=selected/False=not selected
       * 
       * example:
       *   - self.list.getSelectedItem().select(True)
       */
      void select(bool selected);

      /**
       * isSelected() -- Returns the listitem's selected status.
       * 
       * example:
       *   - is = self.list.getSelectedItem().isSelected()
       */
      bool isSelected();

      /**
       * setInfo(type, infoLabels) -- Sets the listitem's infoLabels.
       * 
       * type              : string - type of media(video/music/pictures).
       * infoLabels        : dictionary - pairs of { label: value }.
       * 
       * *Note, To set pictures exif info, prepend 'exif:' to the label. Exif values must be passed
       *        as strings, separate value pairs with a comma. (eg. {'exif:resolution': '720,480'}
       *        See CPictureInfoTag::TranslateString in PictureInfoTag.cpp for valid strings.
       * 
       *        You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * General Values that apply to all types:
       *     count         : integer (12) - can be used to store an id for later, or for sorting purposes
       *     size          : long (1024) - size in bytes
       *     date          : string (%d.%m.%Y / 01.01.2009) - file date
       * 
       * Video Values:
       *     genre         : string (Comedy)
       *     year          : integer (2009)
       *     episode       : integer (4)
       *     season        : integer (1)
       *     top250        : integer (192)
       *     tracknumber   : integer (3)
       *     rating        : float (6.4) - range is 0..10
       *     watched       : depreciated - use playcount instead
       *     playcount     : integer (2) - number of times this item has been played
       *     overlay       : integer (2) - range is 0..8.  See GUIListItem.h for values
       *     cast          : list (Michal C. Hall)
       *     castandrole   : list (Michael C. Hall|Dexter)
       *     director      : string (Dagur Kari)
       *     mpaa          : string (PG-13)
       *     plot          : string (Long Description)
       *     plotoutline   : string (Short Description)
       *     title         : string (Big Fan)
       *     originaltitle : string (Big Fan)
       *     duration      : string (3:18)
       *     studio        : string (Warner Bros.)
       *     tagline       : string (An awesome movie) - short description of movie
       *     writer        : string (Robert D. Siegel)
       *     tvshowtitle   : string (Heroes)
       *     premiered     : string (2005-03-04)
       *     status        : string (Continuing) - status of a TVshow
       *     code          : string (tt0110293) - IMDb code
       *     aired         : string (2008-12-07)
       *     credits       : string (Andy Kaufman) - writing credits
       *     lastplayed    : string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
       *     album         : string (The Joshua Tree)
       *     artist        : list (['U2'])
       *     votes         : string (12345 votes)
       *     trailer       : string (/home/user/trailer.avi)
       *     dateadded     : string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
       * 
       * Music Values:
       *     tracknumber   : integer (8)
       *     duration      : integer (245) - duration in seconds
       *     year          : integer (1998)
       *     genre         : string (Rock)
       *     album         : string (Pulse)
       *     artist        : string (Muse)
       *     title         : string (American Pie)
       *     rating        : string (3) - single character between 0 and 5
       *     lyrics        : string (On a dark desert highway...)
       *     playcount     : integer (2) - number of times this item has been played
       *     lastplayed    : string (%Y-%m-%d %h:%m:%s = 2009-04-05 23:16:04)
       * 
       * Picture Values:
       *     title         : string (In the last summer-1)
       *     picturepath   : string (/home/username/pictures/img001.jpg)
       *     exif*         : string (See CPictureInfoTag::TranslateString in PictureInfoTag.cpp for valid strings)
       * 
       * example:
       *   - self.list.getSelectedItem().setInfo('video', { 'Genre': 'Comedy' })\n
       */
      void setInfo(const char* type, const Dictionary& infoLabels);

      /**
       * addStreamInfo(type, values) -- Add a stream with details.
       * 
       * type              : string - type of stream(video/audio/subtitle).
       * values            : dictionary - pairs of { label: value }.
       * 
       * Video Values:
       *     codec         : string (h264)
       *     aspect        : float (1.78)
       *     width         : integer (1280)
       *     height        : integer (720)
       *     duration      : integer (seconds)
       * 
       * Audio Values:
       *     codec         : string (dts)
       *     language      : string (en)
       *     channels      : integer (2)
       * 
       * Subtitle Values:
       *     language      : string (en)
       * 
       * example:
       *   - self.list.getSelectedItem().addStreamInfo('video', { 'Codec': 'h264', 'Width' : 1280 })
       */
      void addStreamInfo(const char* cType, const Dictionary& dictionary);

      /**
       * addContextMenuItems([(label, action,)*], replaceItems) -- Adds item(s) to the context menu for media lists.
       * 
       * items               : list - [(label, action,)*] A list of tuples consisting of label and action pairs.
       *   - label           : string or unicode - item's label.
       *   - action          : string or unicode - any built-in function to perform.
       * replaceItems        : [opt] bool - True=only your items will show/False=your items will be added to context menu(Default).
       * 
       * List of functions - http://wiki.xbmc.org/?title=List_of_Built_In_Functions 
       * 
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - listitem.addContextMenuItems([('Theater Showtimes', 'XBMC.RunScript(special://home/scripts/showtimes/default.py,Iron Man)',)])\n
       */
      void addContextMenuItems(const std::vector<Tuple<String,String> >& items, bool replaceItems = false) throw (ListItemException);

      /**
       * setProperty(key, value) -- Sets a listitem property, similar to an infolabel.
       * 
       * key            : string - property name.
       * value          : string or unicode - value of property.
       * 
       * *Note, Key is NOT case sensitive.
       *        You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       *  Some of these are treated internally by XBMC, such as the 'StartOffset' property, which is
       *  the offset in seconds at which to start playback of an item.  Others may be used in the skin
       *  to add extra information, such as 'WatchedCount' for tvshow items
       * 
       * example:
       *   - self.list.getSelectedItem().setProperty('AspectRatio', '1.85 : 1')
       *   - self.list.getSelectedItem().setProperty('StartOffset', '256.4')
       */
      void setProperty(const char * key, const String& value);

      /**
       * getProperty(key) -- Returns a listitem property as a string, similar to an infolabel.
       * 
       * key            : string - property name.
       * 
       * *Note, Key is NOT case sensitive.
       *        You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - AspectRatio = self.list.getSelectedItem().getProperty('AspectRatio')
       */
      String getProperty(const char* key);

      /**
       * addContextMenuItems([(label, action,)*], replaceItems) -- Adds item(s) to the context menu for media lists.
       * 
       * items               : list - [(label, action,)*] A list of tuples consisting of label and action pairs.
       *   - label           : string or unicode - item's label.
       *   - action          : string or unicode - any built-in function to perform.
       * replaceItems        : [opt] bool - True=only your items will show/False=your items will be added to context menu(Default).
       * 
       * List of functions - http://wiki.xbmc.org/?title=List_of_Built_In_Functions 
       * 
       * *Note, You can use the above as keywords for arguments and skip certain optional arguments.
       *        Once you use a keyword, all following arguments require the keyword.
       * 
       * example:
       *   - listitem.addContextMenuItems([('Theater Showtimes', 'XBMC.RunScript(special://home/scripts/showtimes/default.py,Iron Man)',)])
       */
      //    void addContextMenuItems();

      /**
       * setPath(path) -- Sets the listitem's path.
       * 
       * path           : string or unicode - path, activated when item is clicked.
       * 
       * *Note, You can use the above as keywords for arguments.
       * 
       * example:
       *   - self.list.getSelectedItem().setPath(path='ActivateWindow(Weather)')
       */
      void setPath(const String& path);

      /**
       * setMimeType(mimetype) -- Sets the listitem's mimetype if known.
       * 
       * mimetype           : string or unicode - mimetype.
       * 
       * *If known prehand, this can avoid xbmc doing HEAD requests to http servers to figure out file type.
       */
      void setMimeType(const String& mimetype);

      /**
       * getdescription() -- Returns the description of this PlayListItem.
       */
      String getdescription();
      
      /**
       * getduration() -- Returns the duration of this PlayListItem
       */
      String getduration();

      /**
       * getfilename() -- Returns the filename of this PlayListItem.
       */
      String getfilename();

    };

    typedef std::vector<ListItem*> ListItemList;
    
  }
}


