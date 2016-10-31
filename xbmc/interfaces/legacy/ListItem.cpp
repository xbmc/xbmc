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

#include <cstdlib>
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
                       const String& path)
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

    void ListItem::setArt(const Properties& dictionary)
    {
      if (!item) return;
      {
        LOCKGUI;
        for (Properties::const_iterator it = dictionary.begin(); it != dictionary.end(); ++it)
        {
          std::string artName = it->first;
          StringUtils::ToLower(artName);
          if (artName == "icon")
            item->SetIconImage(it->second);
          else
            item->SetArt(artName, it->second);
        }
      }
    }

    void ListItem::setUniqueIDs(const Properties& dictionary)
    {
      if (!item) return;

      LOCKGUI;
      CVideoInfoTag& vtag = *item->GetVideoInfoTag();
      for (Properties::const_iterator it = dictionary.begin(); it != dictionary.end(); ++it)
        vtag.SetUniqueID(it->second, it->first);
    }

    void ListItem::setRating(std::string type, float rating, int votes /* = 0 */, bool defaultt /* = false */)
    {
      if (!item) return;

      LOCKGUI;
      item->GetVideoInfoTag()->SetRating(rating, votes, type, defaultt);
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
      String lowerKey = key;
      StringUtils::ToLower(lowerKey);
      if (lowerKey == "startoffset")
      { // special case for start offset - don't actually store in a property,
        // we store it in item.m_lStartOffset instead
        item->m_lStartOffset = (int)(atof(value.c_str()) * 75.0); // we store the offset in frames, or 1/75th of a second
      }
      else if (lowerKey == "mimetype")
      { // special case for mime type - don't actually stored in a property,
        item->SetMimeType(value.c_str());
      }
      else if (lowerKey == "totaltime")
        item->GetVideoInfoTag()->m_resumePoint.totalTimeInSeconds = (float)atof(value.c_str());
      else if (lowerKey == "resumetime")
        item->GetVideoInfoTag()->m_resumePoint.timeInSeconds = (float)atof(value.c_str());
      else if (lowerKey == "specialsort")
      {
        if (value == "bottom")
          item->SetSpecialSort(SortSpecialOnBottom);
        else if (value == "top")
          item->SetSpecialSort(SortSpecialOnTop);
      }
      else if (lowerKey == "fanart_image")
        item->SetArt("fanart", value);
      else
        item->SetProperty(lowerKey, value);
    }

    String ListItem::getProperty(const char* key)
    {
      LOCKGUI;
      String lowerKey = key;
      StringUtils::ToLower(lowerKey);
      std::string value;
      if (lowerKey == "startoffset")
      { // special case for start offset - don't actually store in a property,
        // we store it in item.m_lStartOffset instead
        value = StringUtils::Format("%f", item->m_lStartOffset / 75.0);
      }
      else if (lowerKey == "totaltime")
        value = StringUtils::Format("%f", item->GetVideoInfoTag()->m_resumePoint.totalTimeInSeconds);
      else if (lowerKey == "resumetime")
        value = StringUtils::Format("%f", item->GetVideoInfoTag()->m_resumePoint.timeInSeconds);
      else if (lowerKey == "fanart_image")
        value = item->GetArt("fanart");
      else
        value = item->GetProperty(lowerKey).asString();

      return value;
    }

    String ListItem::getArt(const char* key)
    {
      LOCKGUI;
      return item->GetArt(key);
    }

    String ListItem::getUniqueID(const char* key)
    {
      LOCKGUI;
      return item->GetVideoInfoTag()->GetUniqueID(key);
    }

    float ListItem::getRating(const char* key)
    {
      LOCKGUI;
      return item->GetVideoInfoTag()->GetRating(key).rating;
    }

    int ListItem::getVotes(const char* key)
    {
      LOCKGUI;
      return item->GetVideoInfoTag()->GetRating(key).votes;
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

    void ListItem::setContentLookup(bool enable)
    {
      LOCKGUI;
      item->SetContentLookup(enable);
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
      {
        std::ostringstream oss;
        oss << item->GetVideoInfoTag()->GetDuration();
        return oss.str();
      }
      return "0";
    }

    String ListItem::getfilename()
    {
      return item->GetPath();
    }

    String ListItem::getPath()
    {
      LOCKGUI;
      return item->GetPath();
    }

    void ListItem::setInfo(const char* type, const InfoLabelDict& infoLabels)
    {
      LOCKGUI;

      if (strcmpi(type, "video") == 0)
      {
        for (InfoLabelDict::const_iterator it = infoLabels.begin(); it != infoLabels.end(); ++it)
        {
          String key = it->first;
          StringUtils::ToLower(key);

          const InfoLabelValue& alt = it->second;
          const String value(alt.which() == first ? alt.former() : emptyString);

          if (key == "dbid")
            item->GetVideoInfoTag()->m_iDbId = strtol(value.c_str(), NULL, 10);
          else if (key == "year")
            item->GetVideoInfoTag()->SetYear(strtol(value.c_str(), NULL, 10));
          else if (key == "episode")
            item->GetVideoInfoTag()->m_iEpisode = strtol(value.c_str(), NULL, 10);
          else if (key == "season")
            item->GetVideoInfoTag()->m_iSeason = strtol(value.c_str(), NULL, 10);
          else if (key == "top250")
            item->GetVideoInfoTag()->m_iTop250 = strtol(value.c_str(), NULL, 10);
          else if (key == "setid")
            item->GetVideoInfoTag()->m_iSetId = strtol(value.c_str(), NULL, 10);
          else if (key == "tracknumber")
            item->GetVideoInfoTag()->m_iTrack = strtol(value.c_str(), NULL, 10);
          else if (key == "count")
            item->m_iprogramCount = strtol(value.c_str(), NULL, 10);
          else if (key == "rating")
            item->GetVideoInfoTag()->SetRating((float)strtod(value.c_str(), NULL));
          else if (key == "userrating")
            item->GetVideoInfoTag()->m_iUserRating = strtol(value.c_str(), NULL, 10);
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
          else if (key == "cast" || key == "castandrole")
          {
            if (alt.which() != second)
              throw WrongTypeException("When using \"cast\" or \"castandrole\" you need to supply a list of tuples for the value in the dictionary");

            item->GetVideoInfoTag()->m_cast.clear();
            const std::vector<InfoLabelStringOrTuple>& listValue = alt.later();
            for (std::vector<InfoLabelStringOrTuple>::const_iterator viter = listValue.begin(); viter != listValue.end(); ++viter)
            {
              const InfoLabelStringOrTuple& castEntry = *viter;
              // castEntry can be a string meaning it's the actor or it can be a tuple meaning it's the 
              //  actor and the role.
              const String& actor = castEntry.which() == first ? castEntry.former() : castEntry.later().first();
              SActorInfo info;
              info.strName = actor;
              if (castEntry.which() == second)
                info.strRole = (const String&)(castEntry.later().second());
              item->GetVideoInfoTag()->m_cast.push_back(info);
            }
          }
          else if (key == "artist")
          {
            if (alt.which() != second)
              throw WrongTypeException("When using \"artist\" you need to supply a list of strings for the value in the dictionary");
            
            item->GetVideoInfoTag()->m_artist.clear();

            const std::vector<InfoLabelStringOrTuple>& listValue = alt.later();
            for (std::vector<InfoLabelStringOrTuple>::const_iterator viter = listValue.begin(); viter != listValue.end(); ++viter)
            {
              
              const InfoLabelStringOrTuple& castEntry = *viter;
              const String& actor = castEntry.which() == first ? castEntry.former() : castEntry.later().first();
              item->GetVideoInfoTag()->m_artist.push_back(actor);
            }
          }
          else if (key == "genre")
            item->GetVideoInfoTag()->m_genre = StringUtils::Split(value, g_advancedSettings.m_videoItemSeparator);
          else if (key == "country")
            item->GetVideoInfoTag()->m_country = StringUtils::Split(value, g_advancedSettings.m_videoItemSeparator);
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
          else if (key == "sorttitle")
            item->GetVideoInfoTag()->m_strSortTitle = value;
          else if (key == "duration")
            item->GetVideoInfoTag()->m_duration = strtol(value.c_str(), NULL, 10);
          else if (key == "studio")
            item->GetVideoInfoTag()->m_studio = StringUtils::Split(value, g_advancedSettings.m_videoItemSeparator);            
          else if (key == "tagline")
            item->GetVideoInfoTag()->m_strTagLine = value;
          else if (key == "writer")
            item->GetVideoInfoTag()->m_writingCredits = StringUtils::Split(value, g_advancedSettings.m_videoItemSeparator);
          else if (key == "tvshowtitle")
            item->GetVideoInfoTag()->m_strShowTitle = value;
          else if (key == "premiered")
          {
            CDateTime premiered;
            premiered.SetFromDateString(value);
            item->GetVideoInfoTag()->SetPremiered(premiered);
          }
          else if (key == "status")
            item->GetVideoInfoTag()->m_strStatus = value;
          else if (key == "set")
            item->GetVideoInfoTag()->m_strSet = value;
          else if (key == "imdbnumber")
            item->GetVideoInfoTag()->SetUniqueID(value);
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
            item->GetVideoInfoTag()->SetVotes(StringUtils::ReturnDigits(value));
          else if (key == "trailer")
            item->GetVideoInfoTag()->m_strTrailer = value;
          else if (key == "date")
          {
            if (value.length() == 10)
            {
              int year = atoi(value.substr(value.size() - 4).c_str());
              int month = atoi(value.substr(3, 4).c_str());
              int day = atoi(value.substr(0, 2).c_str());
              item->m_dateTime.SetDate(year, month, day);
            }
            else
              CLog::Log(LOGERROR,"NEWADDON Invalid Date Format \"%s\"",value.c_str());
          }
          else if (key == "dateadded")
            item->GetVideoInfoTag()->m_dateAdded.SetFromDBDateTime(value.c_str());
          else if (key == "mediatype")
          {
            if (CMediaTypes::IsValidMediaType(value))
              item->GetVideoInfoTag()->m_type = value;
            else
              CLog::Log(LOGWARNING, "Invalid media type \"%s\"", value.c_str());
          }
        }
      }
      else if (strcmpi(type, "music") == 0)
      {
        for (InfoLabelDict::const_iterator it = infoLabels.begin(); it != infoLabels.end(); ++it)
        {
          String key = it->first;
          StringUtils::ToLower(key);

          const InfoLabelValue& alt = it->second;
          const String value(alt.which() == first ? alt.former() : emptyString);

          //! @todo add the rest of the infolabels
          if (key == "tracknumber")
            item->GetMusicInfoTag()->SetTrackNumber(strtol(value.c_str(), NULL, 10));
          else if (key == "discnumber")
            item->GetMusicInfoTag()->SetDiscNumber(strtol(value.c_str(), NULL, 10));
          else if (key == "count")
            item->m_iprogramCount = strtol(value.c_str(), NULL, 10);
          else if (key == "size")
            item->m_dwSize = (int64_t)strtoll(value.c_str(), NULL, 10);
          else if (key == "duration")
            item->GetMusicInfoTag()->SetDuration(strtol(value.c_str(), NULL, 10));
          else if (key == "year")
            item->GetMusicInfoTag()->SetYear(strtol(value.c_str(), NULL, 10));
          else if (key == "listeners")
            item->GetMusicInfoTag()->SetListeners(strtol(value.c_str(), NULL, 10));
          else if (key == "playcount")
            item->GetMusicInfoTag()->SetPlayCount(strtol(value.c_str(), NULL, 10));
          else if (key == "genre")
            item->GetMusicInfoTag()->SetGenre(value);
          else if (key == "album")
            item->GetMusicInfoTag()->SetAlbum(value);
          else if (key == "artist")
            item->GetMusicInfoTag()->SetArtist(value);
          else if (key == "title")
            item->GetMusicInfoTag()->SetTitle(value);
          else if (key == "rating")
            item->GetMusicInfoTag()->SetRating((float)strtod(value.c_str(), NULL));
          else if (key == "userrating")
            item->GetMusicInfoTag()->SetUserrating(strtol(value.c_str(), NULL, 10));
          else if (key == "lyrics")
            item->GetMusicInfoTag()->SetLyrics(value);
          else if (key == "lastplayed")
            item->GetMusicInfoTag()->SetLastPlayed(value);
          else if (key == "musicbrainztrackid")
            item->GetMusicInfoTag()->SetMusicBrainzTrackID(value);
          else if (key == "musicbrainzartistid")
            item->GetMusicInfoTag()->SetMusicBrainzArtistID(StringUtils::Split(value, g_advancedSettings.m_musicItemSeparator));
          else if (key == "musicbrainzalbumid")
            item->GetMusicInfoTag()->SetMusicBrainzAlbumID(value);
          else if (key == "musicbrainzalbumartistid")
            item->GetMusicInfoTag()->SetMusicBrainzAlbumArtistID(StringUtils::Split(value, g_advancedSettings.m_musicItemSeparator));
          else if (key == "comment")
            item->GetMusicInfoTag()->SetComment(value);
          else if (key == "mediatype")
          {
            if (CMediaTypes::IsValidMediaType(value))
              item->GetMusicInfoTag()->SetType(value);
            else
              CLog::Log(LOGWARNING, "Invalid media type \"%s\"", value.c_str());
          }
          else if (key == "date")
          {
            if (strlen(value.c_str()) == 10)
            {
              int year = atoi(value.substr(value.size() - 4).c_str());
              int month = atoi(value.substr(3, 4).c_str());
              int day = atoi(value.substr(0, 2).c_str());
              item->m_dateTime.SetDate(year, month, day);
            }
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
        for (InfoLabelDict::const_iterator it = infoLabels.begin(); it != infoLabels.end(); ++it)
        {
          String key = it->first;
          StringUtils::ToLower(key);

          const InfoLabelValue& alt = it->second;
          const String value(alt.which() == first ? alt.former() : emptyString);

          if (key == "count")
            item->m_iprogramCount = strtol(value.c_str(), NULL, 10);
          else if (key == "size")
            item->m_dwSize = (int64_t)strtoll(value.c_str(), NULL, 10);
          else if (key == "title")
            item->m_strTitle = value;
          else if (key == "picturepath")
            item->SetPath(value);
          else if (key == "date")
          {
            if (strlen(value.c_str()) == 10)
            {
              int year = atoi(value.substr(value.size() - 4).c_str());
              int month = atoi(value.substr(3, 4).c_str());
              int day = atoi(value.substr(0, 2).c_str());
              item->m_dateTime.SetDate(year, month, day);
            }
          }
          else
          {
            const String& exifkey = key;
            if (!StringUtils::StartsWithNoCase(exifkey, "exif:") || exifkey.length() < 6) continue;
            int info = CPictureInfoTag::TranslateString(StringUtils::Mid(exifkey,5));
            item->GetPictureInfoTag()->SetInfo(info, value);
          }
        }
      }
    } // end ListItem::setInfo

    void ListItem::setCast(const std::vector<Properties>& actors)
    {
      LOCKGUI;
      item->GetVideoInfoTag()->m_cast.clear();
      for (const auto& dictionary: actors)
      {
        SActorInfo info;
        for (auto it = dictionary.begin(); it != dictionary.end(); ++it)
        {
          const String& key = it->first;
          const String& value = it->second;
          if (key == "name")
            info.strName = value;
          else if (key == "role")
            info.strRole = value;
          else if (key == "thumbnail")
            info.thumbUrl = value;
          else if (key == "order")
            info.order = strtol(value.c_str(), NULL, 10);
        }
        item->GetVideoInfoTag()->m_cast.push_back(std::move(info));
      }
    }

    void ListItem::addStreamInfo(const char* cType, const Properties& dictionary)
    {
      LOCKGUI;

      if (strcmpi(cType, "video") == 0)
      {
        CStreamDetailVideo* video = new CStreamDetailVideo;
        for (Properties::const_iterator it = dictionary.begin(); it != dictionary.end(); ++it)
        {
          const String& key = it->first;
          const String value(it->second.c_str());

          if (key == "codec")
            video->m_strCodec = value;
          else if (key == "aspect")
            video->m_fAspect = (float)atof(value.c_str());
          else if (key == "width")
            video->m_iWidth = strtol(value.c_str(), NULL, 10);
          else if (key == "height")
            video->m_iHeight = strtol(value.c_str(), NULL, 10);
          else if (key == "duration")
            video->m_iDuration = strtol(value.c_str(), NULL, 10);
          else if (key == "stereomode")
            video->m_strStereoMode = value;
          else if (key == "language")
            video->m_strLanguage = value;
        }
        item->GetVideoInfoTag()->m_streamDetails.AddStream(video);
      }
      else if (strcmpi(cType, "audio") == 0)
      {
        CStreamDetailAudio* audio = new CStreamDetailAudio;
        for (Properties::const_iterator it = dictionary.begin(); it != dictionary.end(); ++it)
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
        for (Properties::const_iterator it = dictionary.begin(); it != dictionary.end(); ++it)
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
    {
      for (size_t i = 0; i < items.size(); ++i)
      {
        auto& tuple = items[i];
        if (tuple.GetNumValuesSet() != 2)
          throw ListItemException("Must pass in a list of tuples of pairs of strings. One entry in the list only has %d elements.",tuple.GetNumValuesSet());

        LOCKGUI;
        item->SetProperty(StringUtils::Format("contextmenulabel(%zu)", i), tuple.first());
        item->SetProperty(StringUtils::Format("contextmenuaction(%zu)", i), tuple.second());
      }
    }

    void ListItem::setSubtitles(const std::vector<String>& paths)
    {
      LOCKGUI;
      unsigned int i = 1;
      for (std::vector<String>::const_iterator it = paths.begin(); it != paths.end(); ++it, i++)
      {
        String property = StringUtils::Format("subtitle:%u", i);
        item->SetProperty(property, *it);
      }
    }

    xbmc::InfoTagVideo* ListItem::getVideoInfoTag()
    {
      LOCKGUI;
      if (item->HasVideoInfoTag())
        return new xbmc::InfoTagVideo(*item->GetVideoInfoTag());
      return new xbmc::InfoTagVideo();
    }

    xbmc::InfoTagMusic* ListItem::getMusicInfoTag()
    {
      LOCKGUI;
      if (item->HasMusicInfoTag())
        return new xbmc::InfoTagMusic(*item->GetMusicInfoTag());
      return new xbmc::InfoTagMusic();
    }
  }
}
