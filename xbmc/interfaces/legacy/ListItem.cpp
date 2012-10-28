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

#include <sstream>

#include "ListItem.h"
#include "AddonUtils.h"

#include "video/VideoInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "utils/log.h"
#include "utils/Variant.h"
#include "utils/StringUtils.h"
#include "settings/AdvancedSettings.h"

namespace XBMCAddon
{
  namespace xbmcgui
  {
    ListItem::ListItem(const String& label, 
                       const String& label2,
                       const String& iconImage,
                       const String& thumbnailImage,
                       const String& path) : AddonClass("ListItem")
    {
      item.reset();

      // create CFileItem
      item.reset(new CFileItem());
      if (!item) // not sure if this is really possible
        return;

      if (!label.empty())
        item->SetLabel( label );
      if (!label2.empty())
        item->SetLabel2( label2 );
      if (!iconImage.empty())
        item->SetIconImage( iconImage );
      if (!thumbnailImage.empty())
        item->SetArt("thumb",  thumbnailImage );
      if (!path.empty())
        item->SetPath(path);
    }

    ListItem::~ListItem()
    {
      item.reset();
    }

    String ListItem::getLabel()
    {
      if (!item) return "";

      String ret;
      {
        LOCKGUI;
        ret = item->GetLabel();
      }

      return ret;
    }

    String ListItem::getLabel2()
    {
      if (!item) return "";

      String ret;
      {
        LOCKGUI;
        ret = item->GetLabel2();
      }

      return ret;
    }

    void ListItem::setLabel(const String& label)
    {
      if (!item) return;
      // set label
      {
        LOCKGUI;
        item->SetLabel(label);
      }
    }

    void ListItem::setLabel2(const String& label)
    {
      if (!item) return;
      // set label
      {
        LOCKGUI;
        item->SetLabel2(label);
      }
    }

    void ListItem::setIconImage(const String& iconImage)
    {
      if (!item) return;
      {
        LOCKGUI;
        item->SetIconImage(iconImage);
      }
    }

    void ListItem::setThumbnailImage(const String& thumbFilename)
    {
      if (!item) return;
      {
        LOCKGUI;
        item->SetArt("thumb", thumbFilename);
      }
    }

    void ListItem::select(bool selected)
    {
      if (!item) return;
      {
        LOCKGUI;
        item->Select(selected);
      }
    }


    bool ListItem::isSelected()
    {
      if (!item) return false;

      bool ret;
      {
        LOCKGUI;
        ret = item->IsSelected();
      }

      return ret;
    }

    void ListItem::setProperty(const char * key, const String& value)
    {
      LOCKGUI;
      CStdString lowerKey = key;
      if (lowerKey.CompareNoCase("startoffset") == 0)
      { // special case for start offset - don't actually store in a property,
        // we store it in item.m_lStartOffset instead
        item->m_lStartOffset = (int)(atof(value.c_str()) * 75.0); // we store the offset in frames, or 1/75th of a second
      }
      else if (lowerKey.CompareNoCase("mimetype") == 0)
      { // special case for mime type - don't actually stored in a property,
        item->SetMimeType(value.c_str());
      }
      else if (lowerKey.CompareNoCase("totaltime") == 0)
        item->GetVideoInfoTag()->m_resumePoint.totalTimeInSeconds = (float)atof(value.c_str());
      else if (lowerKey.CompareNoCase("resumetime") == 0)
        item->GetVideoInfoTag()->m_resumePoint.timeInSeconds = (float)atof(value.c_str());
      else if (lowerKey.CompareNoCase("specialsort") == 0)
      {
        if (value == "bottom")
          item->SetSpecialSort(SortSpecialOnBottom);
        else if (value == "top")
          item->SetSpecialSort(SortSpecialOnTop);
      }
      else if (lowerKey.CompareNoCase("fanart_image") == 0)
        item->SetArt("fanart", value);
      else
        item->SetProperty(lowerKey.ToLower(), value.c_str());
    }

    String ListItem::getProperty(const char* key)
    {
      LOCKGUI;
      CStdString lowerKey = key;
      CStdString value;
      if (lowerKey.CompareNoCase("startoffset") == 0)
      { // special case for start offset - don't actually store in a property,
        // we store it in item.m_lStartOffset instead
        value.Format("%f", item->m_lStartOffset / 75.0);
      }
      else if (lowerKey.CompareNoCase("totaltime") == 0)
        value.Format("%f", item->GetVideoInfoTag()->m_resumePoint.totalTimeInSeconds);
      else if (lowerKey.CompareNoCase("resumetime") == 0)
        value.Format("%f", item->GetVideoInfoTag()->m_resumePoint.timeInSeconds);
      else if (lowerKey.CompareNoCase("fanart_image") == 0)
        value = item->GetArt("fanart");
      else
        value = item->GetProperty(lowerKey.ToLower()).asString();

      return value.c_str();
    }

    void ListItem::setPath(const String& path)
    {
      LOCKGUI;
      item->SetPath(path);
    }

    void ListItem::setMimeType(const String& mimetype)
    {
      LOCKGUI;
      item->SetMimeType(mimetype);
    }

    String ListItem::getdescription()
    {
      return item->GetLabel();
    }

    String ListItem::getduration()
    {
      if (item->LoadMusicTag())
      {
        std::ostringstream oss;
        oss << item->GetMusicInfoTag()->GetDuration();
        return oss.str();
      }

      if (item->HasVideoInfoTag())
        return item->GetVideoInfoTag()->m_strRuntime;

      return "0";
    }

    String ListItem::getfilename()
    {
      return item->GetPath();
    }

    void ListItem::setInfo(const char* type, const Dictionary& infoLabels)
    {
      LOCKGUI;

      if (strcmpi(type, "video") == 0)
      {
        for (Dictionary::const_iterator it = infoLabels.begin(); it != infoLabels.end(); it++)
        {
          CStdString key = it->first;
          key.ToLower();
          const CStdString value(it->second.c_str());

          if (key == "year")
            item->GetVideoInfoTag()->m_iYear = strtol(value.c_str(), NULL, 10);
          else if (key == "episode")
            item->GetVideoInfoTag()->m_iEpisode = strtol(value.c_str(), NULL, 10);
          else if (key == "season")
            item->GetVideoInfoTag()->m_iSeason = strtol(value.c_str(), NULL, 10);
          else if (key == "top250")
            item->GetVideoInfoTag()->m_iTop250 = strtol(value.c_str(), NULL, 10);
          else if (key == "tracknumber")
            item->GetVideoInfoTag()->m_iTrack = strtol(value.c_str(), NULL, 10);
          else if (key == "count")
            item->m_iprogramCount = strtol(value.c_str(), NULL, 10);
          else if (key == "rating")
            item->GetVideoInfoTag()->m_fRating = (float)strtod(value.c_str(), NULL);
          else if (key == "size")
            item->m_dwSize = (int64_t)strtoll(value.c_str(), NULL, 10);
          else if (key == "watched") // backward compat - do we need it?
            item->GetVideoInfoTag()->m_playCount = strtol(value.c_str(), NULL, 10);
          else if (key == "playcount")
            item->GetVideoInfoTag()->m_playCount = strtol(value.c_str(), NULL, 10);
          else if (key == "overlay")
          {
            long overlay = strtol(value.c_str(), NULL, 10);
            if (overlay >= 0 && overlay <= 8)
              item->SetOverlayImage((CGUIListItem::GUIIconOverlay)overlay);
          }
// TODO: This is a dynamic type for the value where a list is expected as the 
//   Dictionary value.
//          else if (key == "cast" || key == "castandrole")
//          {
//            if (!PyObject_TypeCheck(value, &PyList_Type)) continue;
//            item->GetVideoInfoTag()->m_cast.clear();
//            for (int i = 0; i < PyList_Size(value); i++)
//            {
//              PyObject *pTuple = NULL;
//              pTuple = PyList_GetItem(value, i);
//              if (pTuple == NULL) continue;
//              PyObject *pActor = NULL;
//              PyObject *pRole = NULL;
//              if (PyObject_TypeCheck(pTuple, &PyTuple_Type))
//              {
//                if (!PyArg_ParseTuple(pTuple, (char*)"O|O", &pActor, &pRole)) continue;
//              }
//              else
//                pActor = pTuple;
//              SActorInfo info;
//              if (!PyXBMCGetUnicodeString(info.strName, pActor, 1)) continue;
//              if (pRole != NULL)
//                PyXBMCGetUnicodeString(info.strRole, pRole, 1);
//              item->GetVideoInfoTag()->m_cast.push_back(info);
//            }
//          }
//          else if (strcmpi(PyString_AsString(key), "artist") == 0)
//          {
//            if (!PyObject_TypeCheck(value, &PyList_Type)) continue;
//            self->item->GetVideoInfoTag()->m_artist.clear();
//            for (int i = 0; i < PyList_Size(value); i++)
//            {
//              PyObject *pActor = PyList_GetItem(value, i);
//              if (pActor == NULL) continue;
//              String actor;
//              if (!PyXBMCGetUnicodeString(actor, pActor, 1)) continue;
//              self->item->GetVideoInfoTag()->m_artist.push_back(actor);
//            }
//          }
          else if (key == "genre")
            item->GetVideoInfoTag()->m_genre = StringUtils::Split(value, g_advancedSettings.m_videoItemSeparator);
          else if (key == "director")
            item->GetVideoInfoTag()->m_director = StringUtils::Split(value, g_advancedSettings.m_videoItemSeparator);
          else if (key == "mpaa")
            item->GetVideoInfoTag()->m_strMPAARating = value;
          else if (key == "plot")
            item->GetVideoInfoTag()->m_strPlot = value;
          else if (key == "plotoutline")
            item->GetVideoInfoTag()->m_strPlotOutline = value;
          else if (key == "title")
            item->GetVideoInfoTag()->m_strTitle = value;
          else if (key == "originaltitle")
            item->GetVideoInfoTag()->m_strOriginalTitle = value;
          else if (key == "duration")
            item->GetVideoInfoTag()->m_strRuntime = value;
          else if (key == "studio")
            item->GetVideoInfoTag()->m_studio = StringUtils::Split(value, g_advancedSettings.m_videoItemSeparator);            
          else if (key == "tagline")
            item->GetVideoInfoTag()->m_strTagLine = value;
          else if (key == "writer")
            item->GetVideoInfoTag()->m_writingCredits = StringUtils::Split(value, g_advancedSettings.m_videoItemSeparator);
          else if (key == "tvshowtitle")
            item->GetVideoInfoTag()->m_strShowTitle = value;
          else if (key == "premiered")
            item->GetVideoInfoTag()->m_premiered.SetFromDateString(value);
          else if (key == "status")
            item->GetVideoInfoTag()->m_strStatus = value;
          else if (key == "code")
            item->GetVideoInfoTag()->m_strProductionCode = value;
          else if (key == "aired")
            item->GetVideoInfoTag()->m_firstAired.SetFromDateString(value);
          else if (key == "credits")
            item->GetVideoInfoTag()->m_writingCredits = StringUtils::Split(value, g_advancedSettings.m_videoItemSeparator);
          else if (key == "lastplayed")
            item->GetVideoInfoTag()->m_lastPlayed.SetFromDBDateTime(value);
          else if (key == "album")
            item->GetVideoInfoTag()->m_strAlbum = value;
          else if (key == "votes")
            item->GetVideoInfoTag()->m_strVotes = value;
          else if (key == "trailer")
            item->GetVideoInfoTag()->m_strTrailer = value;
          else if (key == "date")
          {
            if (value.length() == 10)
              item->m_dateTime.SetDate(atoi(value.Right(4).c_str()), atoi(value.Mid(3,4).c_str()), atoi(value.Left(2).c_str()));
            else
              CLog::Log(LOGERROR,"NEWADDON Invalid Date Format \"%s\"",value.c_str());
          }
          else if (key == "dateadded")
            item->GetVideoInfoTag()->m_dateAdded.SetFromDBDateTime(value.c_str());
        }
      }
      else if (strcmpi(type, "music") == 0)
      {
        for (Dictionary::const_iterator it = infoLabels.begin(); it != infoLabels.end(); it++)
        {
          CStdString key = it->first;

          key.ToLower();
          const CStdString value(it->second.c_str());

          // TODO: add the rest of the infolabels
          if (key == "tracknumber")
            item->GetMusicInfoTag()->SetTrackNumber(strtol(value, NULL, 10));
          else if (key == "count")
            item->m_iprogramCount = strtol(value, NULL, 10);
          else if (key == "size")
            item->m_dwSize = (int64_t)strtoll(value, NULL, 10);
          else if (key == "duration")
            item->GetMusicInfoTag()->SetDuration(strtol(value, NULL, 10));
          else if (key == "year")
            item->GetMusicInfoTag()->SetYear(strtol(value, NULL, 10));
          else if (key == "listeners")
            item->GetMusicInfoTag()->SetListeners(strtol(value, NULL, 10));
          else if (key == "playcount")
            item->GetMusicInfoTag()->SetPlayCount(strtol(value, NULL, 10));
          else if (key == "genre")
            item->GetMusicInfoTag()->SetGenre(value);
          else if (key == "album")
            item->GetMusicInfoTag()->SetAlbum(value);
          else if (key == "artist")
            item->GetMusicInfoTag()->SetArtist(value);
          else if (key == "title")
            item->GetMusicInfoTag()->SetTitle(value);
          else if (key == "rating")
            item->GetMusicInfoTag()->SetRating(*value);
          else if (key == "lyrics")
            item->GetMusicInfoTag()->SetLyrics(value);
          else if (key == "lastplayed")
            item->GetMusicInfoTag()->SetLastPlayed(value);
          else if (key == "musicbrainztrackid")
            item->GetMusicInfoTag()->SetMusicBrainzTrackID(value);
          else if (key == "musicbrainzartistid")
            item->GetMusicInfoTag()->SetMusicBrainzArtistID(value);
          else if (key == "musicbrainzalbumid")
            item->GetMusicInfoTag()->SetMusicBrainzAlbumID(value);
          else if (key == "musicbrainzalbumartistid")
            item->GetMusicInfoTag()->SetMusicBrainzAlbumArtistID(value);
          else if (key == "musicbrainztrmid")
            item->GetMusicInfoTag()->SetMusicBrainzTRMID(value);
          else if (key == "comment")
            item->GetMusicInfoTag()->SetComment(value);
          else if (key == "date")
          {
            if (strlen(value) == 10)
              item->m_dateTime.SetDate(atoi(value.Right(4)), atoi(value.Mid(3,4)), atoi(value.Left(2)));
          }
          else
            CLog::Log(LOGERROR,"NEWADDON Unknown Music Info Key \"%s\"", key.c_str());

          // This should probably be set outside of the loop but since the original
          //  implementation set it inside of the loop, I'll leave it that way. - Jim C.
          item->GetMusicInfoTag()->SetLoaded(true);
        }
      }
      else if (strcmpi(type,"pictures") == 0)
      {
        for (Dictionary::const_iterator it = infoLabels.begin(); it != infoLabels.end(); it++)
        {
          CStdString key = it->first;
          key.ToLower();
          const CStdString value(it->second.c_str());

          if (key == "count")
            item->m_iprogramCount = strtol(value, NULL, 10);
          else if (key == "size")
            item->m_dwSize = (int64_t)strtoll(value, NULL, 10);
          else if (key == "title")
            item->m_strTitle = value;
          else if (key == "picturepath")
            item->SetPath(value);
          else if (key == "date")
          {
            if (strlen(value) == 10)
              item->m_dateTime.SetDate(atoi(value.Right(4)), atoi(value.Mid(3,4)), atoi(value.Left(2)));
          }
          else
          {
            const CStdString& exifkey = key;
            if (!exifkey.Left(5).Equals("exif:") || exifkey.length() < 6) continue;
            int info = CPictureInfoTag::TranslateString(exifkey.Mid(5));
            item->GetPictureInfoTag()->SetInfo(info, value);
          }

          // This should probably be set outside of the loop but since the original
          //  implementation set it inside of the loop, I'll leave it that way. - Jim C.
          item->GetPictureInfoTag()->SetLoaded(true);
        }
      }
    } // end ListItem::setInfo

    void ListItem::addStreamInfo(const char* cType, const Dictionary& dictionary)
    {
      LOCKGUI;

      String tmp;
      if (strcmpi(cType, "video") == 0)
      {
        CStreamDetailVideo* video = new CStreamDetailVideo;
        for (Dictionary::const_iterator it = dictionary.begin(); it != dictionary.end(); it++)
        {
          const String& key = it->first;
          const CStdString value(it->second.c_str());

          if (key == "codec")
            video->m_strCodec = value;
          else if (key == "aspect")
            video->m_fAspect = (float)atof(value);
          else if (key == "width")
            video->m_iWidth = strtol(value, NULL, 10);
          else if (key == "height")
            video->m_iHeight = strtol(value, NULL, 10);
          else if (key == "duration")
            video->m_iDuration = strtol(value, NULL, 10);
        }
        item->GetVideoInfoTag()->m_streamDetails.AddStream(video);
      }
      else if (strcmpi(cType, "audio") == 0)
      {
        CStreamDetailAudio* audio = new CStreamDetailAudio;
        for (Dictionary::const_iterator it = dictionary.begin(); it != dictionary.end(); it++)
        {
          const String& key = it->first;
          const String& value = it->second;

          if (key == "codec")
            audio->m_strCodec = value;
          else if (key == "language")
            audio->m_strLanguage = value;
          else if (key == "channels")
            audio->m_iChannels = strtol(value.c_str(), NULL, 10);
        }
        item->GetVideoInfoTag()->m_streamDetails.AddStream(audio);
      }
      else if (strcmpi(cType, "subtitle") == 0)
      {
        CStreamDetailSubtitle* subtitle = new CStreamDetailSubtitle;
        for (Dictionary::const_iterator it = dictionary.begin(); it != dictionary.end(); it++)
        {
          const String& key = it->first;
          const String& value = it->second;

          if (key == "language")
            subtitle->m_strLanguage = value;
        }
        item->GetVideoInfoTag()->m_streamDetails.AddStream(subtitle);
      }
    } // end ListItem::addStreamInfo

    void ListItem::addContextMenuItems(const std::vector<Tuple<String,String> >& items, bool replaceItems /* = false */)
      throw (ListItemException)
    {
      int itemCount = 0;
      for (std::vector<Tuple<String,String> >::const_iterator iter = items.begin(); iter < items.end(); iter++, itemCount++)
      {
        Tuple<String,String> tuple = *iter;
        if (tuple.GetNumValuesSet() != 2)
          throw ListItemException("Must pass in a list of tuples of pairs of strings. One entry in the list only has %d elements.",tuple.GetNumValuesSet());
        std::string uText = tuple.first();
        std::string uAction = tuple.second();

        LOCKGUI;
        CStdString property;
        property.Format("contextmenulabel(%i)", itemCount);
        item->SetProperty(property, uText);

        property.Format("contextmenuaction(%i)", itemCount);
        item->SetProperty(property, uAction);
      }

      // set our replaceItems status
      if (replaceItems)
        item->SetProperty("pluginreplacecontextitems", replaceItems);
    } // end addContextMenuItems
  }
}
