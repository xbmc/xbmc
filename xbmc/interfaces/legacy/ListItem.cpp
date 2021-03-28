/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ListItem.h"

#include "AddonUtils.h"
#include "ServiceBroker.h"
#include "Util.h"
#include "games/GameTypes.h"
#include "games/tags/GameInfoTag.h"
#include "music/tags/MusicInfoTag.h"
#include "pictures/PictureInfoTag.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/Variant.h"
#include "utils/log.h"
#include "video/VideoInfoTag.h"

#include <cstdlib>
#include <sstream>
#include <utility>

namespace XBMCAddon
{
  namespace xbmcgui
  {
    ListItem::ListItem(const String& label,
                       const String& label2,
                       const String& path,
                       bool offscreen) :
      m_offscreen(offscreen)
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
        XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
        ret = item->GetLabel();
      }

      return ret;
    }

    String ListItem::getLabel2()
    {
      if (!item) return "";

      String ret;
      {
        XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
        ret = item->GetLabel2();
      }

      return ret;
    }

    void ListItem::setLabel(const String& label)
    {
      if (!item) return;
      // set label
      {
        XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
        item->SetLabel(label);
      }
    }

    void ListItem::setLabel2(const String& label)
    {
      if (!item) return;
      // set label
      {
        XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
        item->SetLabel2(label);
      }
    }

    String ListItem::getDateTime()
    {
      if (!item)
        return "";

      String ret;
      {
        XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
        if (item->m_dateTime.IsValid())
          ret = item->m_dateTime.GetAsW3CDateTime();
      }

      return ret;
    }

    void ListItem::setDateTime(const String& dateTime)
    {
      if (!item)
        return;
      // set datetime
      {
        XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
        setDateTimeRaw(dateTime);
      }
    }

    void ListItem::setArt(const Properties& dictionary)
    {
      if (!item) return;
      {
        XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
        for (const auto& it : dictionary)
          addArtRaw(it.first, it.second);
      }
    }

    void ListItem::setIsFolder(bool isFolder)
    {
      if (!item)
        return;

      {
        XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
        setIsFolderRaw(isFolder);
      }
    }

    void ListItem::setUniqueIDs(const Properties& dictionary, const String& defaultrating /* = "" */)
    {
      if (!item) return;

      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      CVideoInfoTag& vtag = *GetVideoInfoTag();
      for (const auto& it : dictionary)
        vtag.SetUniqueID(it.second, it.first, it.first == defaultrating);
    }

    void ListItem::setRating(const std::string& type,
                             float rating,
                             int votes /* = 0 */,
                             bool defaultt /* = false */)
    {
      if (!item) return;

      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      GetVideoInfoTag()->SetRating(rating, votes, type, defaultt);
    }

    void ListItem::addSeason(int number, std::string name /* = "" */)
    {
      if (!item) return;

      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      GetVideoInfoTag()->m_namedSeasons[number] = std::move(name);
    }

    void ListItem::select(bool selected)
    {
      if (!item) return;
      {
        XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
        item->Select(selected);
      }
    }


    bool ListItem::isSelected()
    {
      if (!item) return false;

      bool ret;
      {
        XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
        ret = item->IsSelected();
      }

      return ret;
    }

    void ListItem::setProperty(const char * key, const String& value)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      String lowerKey = key;
      StringUtils::ToLower(lowerKey);
      if (lowerKey == "startoffset")
      { // special case for start offset - don't actually store in a property
        setStartOffsetRaw(strtod(value.c_str(), nullptr));
      }
      else if (lowerKey == "mimetype")
      { // special case for mime type - don't actually stored in a property,
        item->SetMimeType(value.c_str());
      }
      else if (lowerKey == "totaltime")
      {
        CBookmark resumePoint(GetVideoInfoTag()->GetResumePoint());
        resumePoint.totalTimeInSeconds = atof(value.c_str());
        GetVideoInfoTag()->SetResumePoint(resumePoint);
      }
      else if (lowerKey == "resumetime")
      {
        CBookmark resumePoint(GetVideoInfoTag()->GetResumePoint());
        resumePoint.timeInSeconds = atof(value.c_str());
        GetVideoInfoTag()->SetResumePoint(resumePoint);
      }
      else if (lowerKey == "specialsort")
        setSpecialSortRaw(value);
      else if (lowerKey == "fanart_image")
        item->SetArt("fanart", value);
      else
        addPropertyRaw(lowerKey, value);
    }

    void ListItem::setProperties(const Properties& dictionary)
    {
      for (const auto& it : dictionary)
        setProperty(it.first.c_str(), it.second);
    }

    String ListItem::getProperty(const char* key)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      String lowerKey = key;
      StringUtils::ToLower(lowerKey);
      std::string value;
      if (lowerKey == "startoffset")
      { // special case for start offset - don't actually store in a property,
        // we store it in item.m_lStartOffset instead
        value = StringUtils::Format("{:f}", CUtil::ConvertMilliSecsToSecs(item->m_lStartOffset));
      }
      else if (lowerKey == "totaltime")
      {
        CLog::Log(LOGWARNING,
                  "\"{}\" in ListItem.getProperty() is deprecated and might be removed in future "
                  "Kodi versions. Please use InfoTagVideo.getResumeTimeTotal().",
                  lowerKey);

        value = StringUtils::Format("{:f}", GetVideoInfoTag()->GetResumePoint().totalTimeInSeconds);
      }
      else if (lowerKey == "resumetime")
      {
        CLog::Log(LOGWARNING,
                  "\"{}\" in ListItem.getProperty() is deprecated and might be removed in future "
                  "Kodi versions. Please use InfoTagVideo.getResumeTime().",
                  lowerKey);

        value = StringUtils::Format("{:f}", GetVideoInfoTag()->GetResumePoint().timeInSeconds);
      }
      else if (lowerKey == "fanart_image")
        value = item->GetArt("fanart");
      else
        value = item->GetProperty(lowerKey).asString();

      return value;
    }

    String ListItem::getArt(const char* key)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      return item->GetArt(key);
    }

    bool ListItem::isFolder() const
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      return item->m_bIsFolder;
    }

    String ListItem::getUniqueID(const char* key)
    {
      CLog::Log(
          LOGWARNING,
          "ListItem.getUniqueID() is deprecated and might be removed in future Kodi versions. "
          "Please use InfoTagVideo.getUniqueID().");

      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      return GetVideoInfoTag()->GetUniqueID(key);
    }

    float ListItem::getRating(const char* key)
    {
      CLog::Log(LOGWARNING,
                "ListItem.getRating() is deprecated and might be removed in future Kodi versions. "
                "Please use InfoTagVideo.getRating().");

      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      return GetVideoInfoTag()->GetRating(key).rating;
    }

    int ListItem::getVotes(const char* key)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      return GetVideoInfoTag()->GetRating(key).votes;
    }

    void ListItem::setPath(const String& path)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      item->SetPath(path);
    }

    void ListItem::setMimeType(const String& mimetype)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      setMimeTypeRaw(mimetype);
    }

    void ListItem::setContentLookup(bool enable)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      setContentLookupRaw(enable);
    }

    String ListItem::getPath()
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      return item->GetPath();
    }

    void ListItem::setInfo(const char* type, const InfoLabelDict& infoLabels)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);

      if (StringUtils::CompareNoCase(type, "video") == 0)
      {
        auto& videotag = *GetVideoInfoTag();
        for (const auto& it : infoLabels)
        {
          const auto key = StringUtils::ToLower(it.first);
          const InfoLabelValue& alt = it.second;
          const String value(alt.which() == first ? alt.former() : emptyString);

          if (key == "dbid")
            videotag.m_iDbId = strtol(value.c_str(), nullptr, 10);
          else if (key == "year")
            videotag.SetYear(strtol(value.c_str(), nullptr, 10));
          else if (key == "episode")
            videotag.m_iEpisode = strtol(value.c_str(), nullptr, 10);
          else if (key == "season")
            videotag.m_iSeason = strtol(value.c_str(), nullptr, 10);
          else if (key == "sortepisode")
            videotag.m_iSpecialSortEpisode = strtol(value.c_str(), nullptr, 10);
          else if (key == "sortseason")
            videotag.m_iSpecialSortSeason = strtol(value.c_str(), nullptr, 10);
          else if (key == "episodeguide")
            videotag.SetEpisodeGuide(value);
          else if (key == "showlink")
            videotag.SetShowLink(getStringArray(alt, key, value));
          else if (key == "top250")
            videotag.m_iTop250 = strtol(value.c_str(), nullptr, 10);
          else if (key == "setid")
            videotag.m_set.id = strtol(value.c_str(), nullptr, 10);
          else if (key == "tracknumber")
            videotag.m_iTrack = strtol(value.c_str(), nullptr, 10);
          else if (key == "count")
            setCountRaw(strtol(value.c_str(), nullptr, 10));
          else if (key == "rating")
            videotag.SetRating(static_cast<float>(strtod(value.c_str(), nullptr)));
          else if (key == "userrating")
            videotag.m_iUserRating = strtol(value.c_str(), nullptr, 10);
          else if (key == "size")
            setSizeRaw(static_cast<int64_t>(strtoll(value.c_str(), nullptr, 10)));
          else if (key == "watched") // backward compat - do we need it?
            videotag.SetPlayCount(strtol(value.c_str(), nullptr, 10));
          else if (key == "playcount")
            videotag.SetPlayCount(strtol(value.c_str(), nullptr, 10));
          else if (key == "overlay")
          {
            long overlay = strtol(value.c_str(), nullptr, 10);
            if (overlay >= 0 && overlay <= 8)
              item->SetOverlayImage(static_cast<CGUIListItem::GUIIconOverlay>(overlay));
          }
          else if (key == "cast" || key == "castandrole")
          {
            if (alt.which() != second)
              throw WrongTypeException("When using \"cast\" or \"castandrole\" you need to supply a list of tuples for the value in the dictionary");

            videotag.m_cast.clear();
            for (const auto& castEntry : alt.later())
            {
              // castEntry can be a string meaning it's the actor or it can be a tuple meaning it's the
              //  actor and the role.
              const String& actor = castEntry.which() == first ? castEntry.former() : castEntry.later().first();
              SActorInfo info;
              info.strName = actor;
              if (castEntry.which() == second)
                info.strRole = static_cast<const String&>(castEntry.later().second());
              videotag.m_cast.push_back(info);
            }
          }
          else if (key == "artist")
          {
            if (alt.which() != second)
              throw WrongTypeException("When using \"artist\" you need to supply a list of strings for the value in the dictionary");

            videotag.m_artist.clear();
            for (const auto& castEntry : alt.later())
            {
              const String& actor = castEntry.which() == first ? castEntry.former() : castEntry.later().first();
              videotag.m_artist.push_back(actor);
            }
          }
          else if (key == "genre")
            videotag.SetGenre(getStringArray(alt, key, value));
          else if (key == "country")
            videotag.SetCountry(getStringArray(alt, key, value));
          else if (key == "director")
            videotag.SetDirector(getStringArray(alt, key, value));
          else if (key == "mpaa")
            videotag.SetMPAARating(value);
          else if (key == "plot")
            videotag.SetPlot(value);
          else if (key == "plotoutline")
            videotag.SetPlotOutline(value);
          else if (key == "title")
            videotag.SetTitle(value);
          else if (key == "originaltitle")
            videotag.SetOriginalTitle(value);
          else if (key == "sorttitle")
            videotag.SetSortTitle(value);
          else if (key == "duration")
            videotag.SetDuration(strtol(value.c_str(), nullptr, 10));
          else if (key == "studio")
            videotag.SetStudio(getStringArray(alt, key, value));
          else if (key == "tagline")
            videotag.SetTagLine(value);
          else if (key == "writer" || key == "credits")
            videotag.SetWritingCredits(getStringArray(alt, key, value));
          else if (key == "tvshowtitle")
            videotag.SetShowTitle(value);
          else if (key == "premiered")
          {
            CDateTime premiered;
            premiered.SetFromDateString(value);
            videotag.SetPremiered(premiered);
          }
          else if (key == "status")
            videotag.SetStatus(value);
          else if (key == "set")
            videotag.SetSet(value);
          else if (key == "setoverview")
            videotag.SetSetOverview(value);
          else if (key == "tag")
            videotag.SetTags(getStringArray(alt, key, value));
          else if (key == "imdbnumber")
            videotag.SetUniqueID(value);
          else if (key == "code")
            videotag.SetProductionCode(value);
          else if (key == "aired")
            videotag.m_firstAired.SetFromDateString(value);
          else if (key == "lastplayed")
            videotag.m_lastPlayed.SetFromDBDateTime(value);
          else if (key == "album")
            videotag.SetAlbum(value);
          else if (key == "votes")
            videotag.SetVotes(StringUtils::ReturnDigits(value));
          else if (key == "trailer")
            videotag.SetTrailer(value);
          else if (key == "path")
            videotag.SetPath(value);
          else if (key == "filenameandpath")
            videotag.SetFileNameAndPath(value);
          else if (key == "date")
            setDateTimeRaw(value);
          else if (key == "dateadded")
            videotag.m_dateAdded.SetFromDBDateTime(value.c_str());
          else if (key == "mediatype")
          {
            if (CMediaTypes::IsValidMediaType(value))
              videotag.m_type = value;
            else
              CLog::Log(LOGWARNING, "Invalid media type \"{}\"", value);
          }
          else
            CLog::Log(LOGERROR, "NEWADDON Unknown Video Info Key \"{}\"", key);
        }
      }
      else if (StringUtils::CompareNoCase(type, "music") == 0)
      {
        std::string type;
        for (auto it = infoLabels.begin(); it != infoLabels.end(); ++it)
        {
          const auto key = StringUtils::ToLower(it->first);
          const auto& alt = it->second;
          const String value(alt.which() == first ? alt.former() : emptyString);

          if (key == "mediatype")
          {
            if (CMediaTypes::IsValidMediaType(value))
            {
              type = value;
              item->GetMusicInfoTag()->SetType(value);
            }
            else
              CLog::Log(LOGWARNING, "Invalid media type \"{}\"", value);
          }
        }
        auto& musictag = *item->GetMusicInfoTag();
        for (const auto& it : infoLabels)
        {
          const auto key = StringUtils::ToLower(it.first);
          const auto& alt = it.second;
          const String value(alt.which() == first ? alt.former() : emptyString);

          //! @todo add the rest of the infolabels
          if (key == "dbid" && !type.empty())
            musictag.SetDatabaseId(static_cast<int>(strtol(value.c_str(), NULL, 10)), type);
          else if (key == "tracknumber")
            musictag.SetTrackNumber(strtol(value.c_str(), NULL, 10));
          else if (key == "discnumber")
            musictag.SetDiscNumber(strtol(value.c_str(), nullptr, 10));
          else if (key == "count")
            setCountRaw(strtol(value.c_str(), nullptr, 10));
          else if (key == "size")
            setSizeRaw(static_cast<int64_t>(strtoll(value.c_str(), nullptr, 10)));
          else if (key == "duration")
            musictag.SetDuration(strtol(value.c_str(), nullptr, 10));
          else if (key == "year")
            musictag.SetYear(strtol(value.c_str(), nullptr, 10));
          else if (key == "listeners")
            musictag.SetListeners(strtol(value.c_str(), nullptr, 10));
          else if (key == "playcount")
            musictag.SetPlayCount(strtol(value.c_str(), nullptr, 10));
          else if (key == "genre")
            musictag.SetGenre(value);
          else if (key == "album")
            musictag.SetAlbum(value);
          else if (key == "artist")
            musictag.SetArtist(value);
          else if (key == "title")
            musictag.SetTitle(value);
          else if (key == "rating")
            musictag.SetRating(static_cast<float>(strtod(value.c_str(), nullptr)));
          else if (key == "userrating")
            musictag.SetUserrating(strtol(value.c_str(), nullptr, 10));
          else if (key == "lyrics")
            musictag.SetLyrics(value);
          else if (key == "lastplayed")
            musictag.SetLastPlayed(value);
          else if (key == "musicbrainztrackid")
            musictag.SetMusicBrainzTrackID(value);
          else if (key == "musicbrainzartistid")
            musictag.SetMusicBrainzArtistID(StringUtils::Split(value, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
          else if (key == "musicbrainzalbumid")
            musictag.SetMusicBrainzAlbumID(value);
          else if (key == "musicbrainzalbumartistid")
            musictag.SetMusicBrainzAlbumArtistID(StringUtils::Split(value, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator));
          else if (key == "comment")
            musictag.SetComment(value);
          else if (key == "date")
            setDateTimeRaw(value);
          else if (key != "mediatype")
            CLog::Log(LOGERROR, "NEWADDON Unknown Music Info Key \"{}\"", key);

          // This should probably be set outside of the loop but since the original
          //  implementation set it inside of the loop, I'll leave it that way. - Jim C.
          musictag.SetLoaded(true);
        }
      }
      else if (StringUtils::CompareNoCase(type, "pictures") == 0)
      {
        for (const auto& it : infoLabels)
        {
          const auto key = StringUtils::ToLower(it.first);
          const auto& alt = it.second;
          const String value(alt.which() == first ? alt.former() : emptyString);

          if (key == "count")
            setCountRaw(strtol(value.c_str(), nullptr, 10));
          else if (key == "size")
            setSizeRaw(static_cast<int64_t>(strtoll(value.c_str(), nullptr, 10)));
          else if (key == "title")
            setTitleRaw(value);
          else if (key == "picturepath")
            setPathRaw(value);
          else if (key == "date")
            setDateTimeRaw(value);
          else
          {
            const String& exifkey = key;
            if (!StringUtils::StartsWithNoCase(exifkey, "exif:") || exifkey.length() < 6)
              continue;

            item->GetPictureInfoTag()->SetInfo(StringUtils::Mid(exifkey, 5), value);
          }
        }
      }
      else if (StringUtils::EqualsNoCase(type, "game"))
      {
        auto& gametag = *item->GetGameInfoTag();
        for (const auto& it : infoLabels)
        {
          const auto key = StringUtils::ToLower(it.first);
          const auto& alt = it.second;
          const String value(alt.which() == first ? alt.former() : emptyString);

          if (key == "title")
          {
            setTitleRaw(value);
            gametag.SetTitle(value);
          }
          else if (key == "platform")
            gametag.SetPlatform(value);
          else if (key == "genres")
          {
            if (alt.which() != second)
              throw WrongTypeException("When using \"genres\" you need to supply a list of strings for the value in the dictionary");

            std::vector<std::string> genres;

            for (const auto& genreEntry : alt.later())
            {
              const String& genre = genreEntry.which() == first ? genreEntry.former() : genreEntry.later().first();
              genres.emplace_back(genre);
            }

            gametag.SetGenres(genres);
          }
          else if (key == "publisher")
            gametag.SetPublisher(value);
          else if (key == "developer")
            gametag.SetDeveloper(value);
          else if (key == "overview")
            gametag.SetOverview(value);
          else if (key == "year")
            gametag.SetYear(strtol(value.c_str(), nullptr, 10));
          else if (key == "gameclient")
            gametag.SetGameClient(value);
        }
      }
      else
        CLog::Log(LOGWARNING, "ListItem.setInfo: unknown \"type\" parameter value: {}", type);
    } // end ListItem::setInfo

    void ListItem::setCast(const std::vector<Properties>& actors)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      GetVideoInfoTag()->m_cast.clear();
      for (const auto& dictionary : actors)
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
          {
            info.thumbUrl = CScraperUrl(value);
            if (!info.thumbUrl.GetFirstThumbUrl().empty())
              info.thumb = CScraperUrl::GetThumbUrl(info.thumbUrl.GetFirstUrlByType());
          }
          else if (key == "order")
            info.order = strtol(value.c_str(), nullptr, 10);
        }
        GetVideoInfoTag()->m_cast.push_back(std::move(info));
      }
    }

    void ListItem::setAvailableFanart(const std::vector<Properties>& images)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      GetVideoInfoTag()->m_fanart.Clear();
      for (const auto& dictionary : images)
      {
        std::string image;
        std::string preview;
        std::string colors;
        for (const auto& it : dictionary)
        {
          const String& key = it.first;
          const String& value = it.second;
          if (key == "image")
            image = value;
          else if (key == "preview")
            preview = value;
          else if (key == "colors")
            colors = value;
        }
        GetVideoInfoTag()->m_fanart.AddFanart(image, preview, colors);
      }
      GetVideoInfoTag()->m_fanart.Pack();
    }

    void ListItem::addAvailableArtwork(const std::string& url,
                                       const std::string& art_type,
                                       const std::string& preview,
                                       const std::string& referrer,
                                       const std::string& cache,
                                       bool post,
                                       bool isgz,
                                       int season)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      GetVideoInfoTag()->m_strPictureURL.AddParsedUrl(url, art_type, preview, referrer, cache, post,
                                                      isgz, season);
    }

    void ListItem::addStreamInfo(const char* cType, const Properties& dictionary)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);

      if (StringUtils::CompareNoCase(cType, "video") == 0)
      {
        CStreamDetailVideo* video = new CStreamDetailVideo;
        for (const auto& it : dictionary)
        {
          const String& key = it.first;
          const String value(it.second.c_str());

          if (key == "codec")
            video->m_strCodec = value;
          else if (key == "aspect")
            video->m_fAspect = static_cast<float>(atof(value.c_str()));
          else if (key == "width")
            video->m_iWidth = strtol(value.c_str(), nullptr, 10);
          else if (key == "height")
            video->m_iHeight = strtol(value.c_str(), nullptr, 10);
          else if (key == "duration")
            video->m_iDuration = strtol(value.c_str(), nullptr, 10);
          else if (key == "stereomode")
            video->m_strStereoMode = value;
          else if (key == "language")
            video->m_strLanguage = value;
        }
        GetVideoInfoTag()->m_streamDetails.AddStream(video);
      }
      else if (StringUtils::CompareNoCase(cType, "audio") == 0)
      {
        CStreamDetailAudio* audio = new CStreamDetailAudio;
        for (const auto& it : dictionary)
        {
          const String& key = it.first;
          const String& value = it.second;

          if (key == "codec")
            audio->m_strCodec = value;
          else if (key == "language")
            audio->m_strLanguage = value;
          else if (key == "channels")
            audio->m_iChannels = strtol(value.c_str(), nullptr, 10);
        }
        GetVideoInfoTag()->m_streamDetails.AddStream(audio);
      }
      else if (StringUtils::CompareNoCase(cType, "subtitle") == 0)
      {
        CStreamDetailSubtitle* subtitle = new CStreamDetailSubtitle;
        for (const auto& it : dictionary)
        {
          const String& key = it.first;
          const String& value = it.second;

          if (key == "language")
            subtitle->m_strLanguage = value;
        }
        GetVideoInfoTag()->m_streamDetails.AddStream(subtitle);
      }
      GetVideoInfoTag()->m_streamDetails.DetermineBestStreams();
    } // end ListItem::addStreamInfo

    void ListItem::addContextMenuItems(const std::vector<Tuple<String,String> >& items, bool replaceItems /* = false */)
    {
      for (size_t i = 0; i < items.size(); ++i)
      {
        auto& tuple = items[i];
        if (tuple.GetNumValuesSet() != 2)
          throw ListItemException("Must pass in a list of tuples of pairs of strings. One entry in the list only has %d elements.",tuple.GetNumValuesSet());

        XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
        item->SetProperty(StringUtils::Format("contextmenulabel({})", i), tuple.first());
        item->SetProperty(StringUtils::Format("contextmenuaction({})", i), tuple.second());
      }
    }

    void ListItem::setSubtitles(const std::vector<String>& paths)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      addSubtitlesRaw(paths);
    }

    xbmc::InfoTagVideo* ListItem::getVideoInfoTag()
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      if (item->HasVideoInfoTag())
        return new xbmc::InfoTagVideo(*GetVideoInfoTag());
      return new xbmc::InfoTagVideo();
    }

    xbmc::InfoTagMusic* ListItem::getMusicInfoTag()
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      if (item->HasMusicInfoTag())
        return new xbmc::InfoTagMusic(*item->GetMusicInfoTag());
      return new xbmc::InfoTagMusic();
    }

    std::vector<std::string> ListItem::getStringArray(const InfoLabelValue& alt, const std::string& tag, std::string value)
    {
      if (alt.which() == first)
      {
        if (value.empty())
          value = alt.former();
        return StringUtils::Split(value, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
      }

      std::vector<std::string> els;
      for (const auto& el : alt.later())
      {
        if (el.which() == second)
          throw WrongTypeException("When using \"%s\" you need to supply a string or list of strings for the value in the dictionary", tag.c_str());
        els.emplace_back(el.former());
      }
      return els;
    }

    CVideoInfoTag* ListItem::GetVideoInfoTag()
    {
      return item->GetVideoInfoTag();
    }

    const CVideoInfoTag* ListItem::GetVideoInfoTag() const
    {
      return item->GetVideoInfoTag();
    }

    void ListItem::setCountRaw(int count)
    {
      item->m_iprogramCount = count;
    }

    void ListItem::setSizeRaw(int64_t size)
    {
      item->m_dwSize = size;
    }

    void ListItem::setDateTimeRaw(const std::string& dateTime)
    {
      if (dateTime.length() == 10)
      {
        int year = strtol(dateTime.substr(dateTime.size() - 4).c_str(), nullptr, 10);
        int month = strtol(dateTime.substr(3, 4).c_str(), nullptr, 10);
        int day = strtol(dateTime.substr(0, 2).c_str(), nullptr, 10);
        item->m_dateTime.SetDate(year, month, day);
      }
      else
        item->m_dateTime.SetFromW3CDateTime(dateTime);
    }

    void ListItem::setIsFolderRaw(bool isFolder)
    {
      item->m_bIsFolder = isFolder;
    }

    void ListItem::setStartOffsetRaw(double startOffset)
    {
      // we store the offset in frames, or 1/75th of a second
      item->m_lStartOffset = CUtil::ConvertSecsToMilliSecs(startOffset);
    }

    void ListItem::setMimeTypeRaw(const std::string& mimetype)
    {
      item->SetMimeType(mimetype);
    }

    void ListItem::setSpecialSortRaw(std::string specialSort)
    {
      StringUtils::ToLower(specialSort);

      if (specialSort == "bottom")
        item->SetSpecialSort(SortSpecialOnBottom);
      else if (specialSort == "top")
        item->SetSpecialSort(SortSpecialOnTop);
    }

    void ListItem::setContentLookupRaw(bool enable)
    {
      item->SetContentLookup(enable);
    }

    void ListItem::addArtRaw(std::string type, std::string url)
    {
      StringUtils::ToLower(type);
      item->SetArt(type, url);
    }

    void ListItem::addPropertyRaw(std::string type, CVariant value)
    {
      StringUtils::ToLower(type);
      item->SetProperty(std::move(type), std::move(value));
    }

    void ListItem::addSubtitlesRaw(const std::vector<std::string>& subtitles)
    {
      for (size_t i = 0; i < subtitles.size(); ++i)
        addPropertyRaw(StringUtils::Format("subtitle:{}", i), subtitles[i]);
    }
  }
}
