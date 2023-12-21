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
#include <memory>
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
      item = std::make_shared<CFileItem>();
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
      CLog::Log(
          LOGWARNING,
          "ListItem.setUniqueIDs() is deprecated and might be removed in future Kodi versions. "
          "Please use InfoTagVideo.setUniqueIDs().");

      if (!item)
        return;

      std::map<String, String> uniqueIDs;
      for (const auto& it : dictionary)
        uniqueIDs.emplace(it.first, it.second);

      {
        XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
        xbmc::InfoTagVideo::setUniqueIDsRaw(GetVideoInfoTag(), uniqueIDs, defaultrating);
      }
    }

    void ListItem::setRating(const std::string& type,
                             float rating,
                             int votes /* = 0 */,
                             bool defaultt /* = false */)
    {
      CLog::Log(LOGWARNING,
                "ListItem.setRating() is deprecated and might be removed in future Kodi versions. "
                "Please use InfoTagVideo.setRating().");

      if (!item) return;

      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      xbmc::InfoTagVideo::setRatingRaw(GetVideoInfoTag(), rating, votes, type, defaultt);
    }

    void ListItem::addSeason(int number, std::string name /* = "" */)
    {
      CLog::Log(LOGWARNING,
                "ListItem.addSeason() is deprecated and might be removed in future Kodi versions. "
                "Please use InfoTagVideo.addSeason().");

      if (!item) return;

      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      xbmc::InfoTagVideo::addSeasonRaw(GetVideoInfoTag(), number, std::move(name));
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
        item->SetMimeType(value);
      }
      else if (lowerKey == "totaltime")
      {
        CLog::Log(LOGWARNING,
                  "\"{}\" in ListItem.setProperty() is deprecated and might be removed in future "
                  "Kodi versions. Please use InfoTagVideo.setResumePoint().",
                  lowerKey);

        CBookmark resumePoint(GetVideoInfoTag()->GetResumePoint());
        resumePoint.totalTimeInSeconds = atof(value.c_str());
        GetVideoInfoTag()->SetResumePoint(resumePoint);
      }
      else if (lowerKey == "resumetime")
      {
        CLog::Log(LOGWARNING,
                  "\"{}\" in ListItem.setProperty() is deprecated and might be removed in future "
                  "Kodi versions. Please use InfoTagVideo.setResumePoint().",
                  lowerKey);

        xbmc::InfoTagVideo::setResumePointRaw(GetVideoInfoTag(), atof(value.c_str()));
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
        value = StringUtils::Format("{:f}", CUtil::ConvertMilliSecsToSecs(item->GetStartOffset()));
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
      CLog::Log(LOGWARNING,
                "ListItem.getVotes() is deprecated and might be removed in future Kodi versions. "
                "Please use InfoTagVideo.getVotesAsInt().");

      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      return GetVideoInfoTag()->GetRating(key).votes;
    }

    void ListItem::setPath(const String& path)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      setPathRaw(path);
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

      bool hasDeprecatedInfoLabel = false;
      if (StringUtils::CompareNoCase(type, "video") == 0)
      {
        using InfoTagVideo = xbmc::InfoTagVideo;
        auto videotag = GetVideoInfoTag();
        for (const auto& it : infoLabels)
        {
          const auto key = StringUtils::ToLower(it.first);
          const InfoLabelValue& alt = it.second;
          const String value(alt.which() == first ? alt.former() : emptyString);

          if (key == "count")
            setCountRaw(strtol(value.c_str(), nullptr, 10));
          else if (key == "size")
            setSizeRaw(static_cast<int64_t>(strtoll(value.c_str(), nullptr, 10)));
          else if (key == "overlay")
          {
            long overlay = strtol(value.c_str(), nullptr, 10);
            if (overlay >= 0 && overlay <= 8)
              item->SetOverlayImage(static_cast<CGUIListItem::GUIIconOverlay>(overlay));
          }
          else if (key == "date")
            setDateTimeRaw(value);
          else
          {
            hasDeprecatedInfoLabel = true;

            if (key == "dbid")
              InfoTagVideo::setDbIdRaw(videotag, strtol(value.c_str(), nullptr, 10));
            else if (key == "year")
              InfoTagVideo::setYearRaw(videotag, strtol(value.c_str(), nullptr, 10));
            else if (key == "episode")
              InfoTagVideo::setEpisodeRaw(videotag, strtol(value.c_str(), nullptr, 10));
            else if (key == "season")
              InfoTagVideo::setSeasonRaw(videotag, strtol(value.c_str(), nullptr, 10));
            else if (key == "sortepisode")
              InfoTagVideo::setSortEpisodeRaw(videotag, strtol(value.c_str(), nullptr, 10));
            else if (key == "sortseason")
              InfoTagVideo::setSortSeasonRaw(videotag, strtol(value.c_str(), nullptr, 10));
            else if (key == "episodeguide")
              InfoTagVideo::setEpisodeGuideRaw(videotag, value);
            else if (key == "showlink")
              InfoTagVideo::setShowLinksRaw(videotag, getVideoStringArray(alt, key, value));
            else if (key == "top250")
              InfoTagVideo::setTop250Raw(videotag, strtol(value.c_str(), nullptr, 10));
            else if (key == "setid")
              InfoTagVideo::setSetIdRaw(videotag, strtol(value.c_str(), nullptr, 10));
            else if (key == "tracknumber")
              InfoTagVideo::setTrackNumberRaw(videotag, strtol(value.c_str(), nullptr, 10));
            else if (key == "rating")
              InfoTagVideo::setRatingRaw(videotag,
                                         static_cast<float>(strtod(value.c_str(), nullptr)));
            else if (key == "userrating")
              InfoTagVideo::setUserRatingRaw(videotag, strtol(value.c_str(), nullptr, 10));
            else if (key == "watched") // backward compat - do we need it?
              InfoTagVideo::setPlaycountRaw(videotag, strtol(value.c_str(), nullptr, 10));
            else if (key == "playcount")
              InfoTagVideo::setPlaycountRaw(videotag, strtol(value.c_str(), nullptr, 10));
            else if (key == "cast" || key == "castandrole")
            {
              if (alt.which() != second)
                throw WrongTypeException("When using \"cast\" or \"castandrole\" you need to "
                                         "supply a list of tuples for the value in the dictionary");

              std::vector<SActorInfo> cast;
              cast.reserve(alt.later().size());
              for (const auto& castEntry : alt.later())
              {
                // castEntry can be a string meaning it's the actor or it can be a tuple meaning it's the
                //  actor and the role.
                const String& actor =
                    castEntry.which() == first ? castEntry.former() : castEntry.later().first();
                SActorInfo info;
                info.strName = actor;
                if (castEntry.which() == second)
                  info.strRole = static_cast<const String&>(castEntry.later().second());
                cast.push_back(std::move(info));
              }
              InfoTagVideo::setCastRaw(videotag, std::move(cast));
            }
            else if (key == "artist")
            {
              if (alt.which() != second)
                throw WrongTypeException("When using \"artist\" you need to supply a list of "
                                         "strings for the value in the dictionary");

              std::vector<String> artists;
              artists.reserve(alt.later().size());
              for (const auto& castEntry : alt.later())
              {
                auto actor =
                    castEntry.which() == first ? castEntry.former() : castEntry.later().first();
                artists.push_back(std::move(actor));
              }
              InfoTagVideo::setArtistsRaw(videotag, artists);
            }
            else if (key == "genre")
              InfoTagVideo::setGenresRaw(videotag, getVideoStringArray(alt, key, value));
            else if (key == "country")
              InfoTagVideo::setCountriesRaw(videotag, getVideoStringArray(alt, key, value));
            else if (key == "director")
              InfoTagVideo::setDirectorsRaw(videotag, getVideoStringArray(alt, key, value));
            else if (key == "mpaa")
              InfoTagVideo::setMpaaRaw(videotag, value);
            else if (key == "plot")
              InfoTagVideo::setPlotRaw(videotag, value);
            else if (key == "plotoutline")
              InfoTagVideo::setPlotOutlineRaw(videotag, value);
            else if (key == "title")
              InfoTagVideo::setTitleRaw(videotag, value);
            else if (key == "originaltitle")
              InfoTagVideo::setOriginalTitleRaw(videotag, value);
            else if (key == "sorttitle")
              InfoTagVideo::setSortTitleRaw(videotag, value);
            else if (key == "duration")
              InfoTagVideo::setDurationRaw(videotag, strtol(value.c_str(), nullptr, 10));
            else if (key == "studio")
              InfoTagVideo::setStudiosRaw(videotag, getVideoStringArray(alt, key, value));
            else if (key == "tagline")
              InfoTagVideo::setTagLineRaw(videotag, value);
            else if (key == "writer" || key == "credits")
              InfoTagVideo::setWritersRaw(videotag, getVideoStringArray(alt, key, value));
            else if (key == "tvshowtitle")
              InfoTagVideo::setTvShowTitleRaw(videotag, value);
            else if (key == "premiered")
              InfoTagVideo::setPremieredRaw(videotag, value);
            else if (key == "status")
              InfoTagVideo::setTvShowStatusRaw(videotag, value);
            else if (key == "set")
              InfoTagVideo::setSetRaw(videotag, value);
            else if (key == "setoverview")
              InfoTagVideo::setSetOverviewRaw(videotag, value);
            else if (key == "tag")
              InfoTagVideo::setTagsRaw(videotag, getVideoStringArray(alt, key, value));
            else if (key == "videoassettitle")
              InfoTagVideo::setVideoAssetTitleRaw(videotag, value);
            else if (key == "imdbnumber")
              InfoTagVideo::setIMDBNumberRaw(videotag, value);
            else if (key == "code")
              InfoTagVideo::setProductionCodeRaw(videotag, value);
            else if (key == "aired")
              InfoTagVideo::setFirstAiredRaw(videotag, value);
            else if (key == "lastplayed")
              InfoTagVideo::setLastPlayedRaw(videotag, value);
            else if (key == "album")
              InfoTagVideo::setAlbumRaw(videotag, value);
            else if (key == "votes")
              InfoTagVideo::setVotesRaw(videotag, StringUtils::ReturnDigits(value));
            else if (key == "trailer")
              InfoTagVideo::setTrailerRaw(videotag, value);
            else if (key == "path")
              InfoTagVideo::setPathRaw(videotag, value);
            else if (key == "filenameandpath")
              InfoTagVideo::setFilenameAndPathRaw(videotag, value);
            else if (key == "dateadded")
              InfoTagVideo::setDateAddedRaw(videotag, value);
            else if (key == "mediatype")
              InfoTagVideo::setMediaTypeRaw(videotag, value);
            else
              CLog::Log(LOGERROR, "NEWADDON Unknown Video Info Key \"{}\"", key);
          }
        }

        if (hasDeprecatedInfoLabel)
        {
          CLog::Log(
            LOGWARNING,
            "Setting most video properties through ListItem.setInfo() is deprecated and might be "
            "removed in future Kodi versions. Please use the respective setter in InfoTagVideo.");
        }
      }
      else if (StringUtils::CompareNoCase(type, "music") == 0)
      {
        String mediaType;
        int dbId = -1;

        using InfoTagMusic = xbmc::InfoTagMusic;
        auto musictag = GetMusicInfoTag();
        for (const auto& it : infoLabels)
        {
          const auto key = StringUtils::ToLower(it.first);
          const auto& alt = it.second;
          const String value(alt.which() == first ? alt.former() : emptyString);

          //! @todo add the rest of the infolabels
          if (key == "count")
            setCountRaw(strtol(value.c_str(), nullptr, 10));
          else if (key == "size")
            setSizeRaw(static_cast<int64_t>(strtoll(value.c_str(), nullptr, 10)));
          else if (key == "date")
            setDateTimeRaw(value);
          else
          {
            hasDeprecatedInfoLabel = true;

            if (key == "dbid")
              dbId = static_cast<int>(strtol(value.c_str(), NULL, 10));
            else if (key == "mediatype")
              mediaType = value;
            else if (key == "tracknumber")
              InfoTagMusic::setTrackRaw(musictag, strtol(value.c_str(), NULL, 10));
            else if (key == "discnumber")
              InfoTagMusic::setDiscRaw(musictag, strtol(value.c_str(), nullptr, 10));
            else if (key == "duration")
              InfoTagMusic::setDurationRaw(musictag, strtol(value.c_str(), nullptr, 10));
            else if (key == "year")
              InfoTagMusic::setYearRaw(musictag, strtol(value.c_str(), nullptr, 10));
            else if (key == "listeners")
              InfoTagMusic::setListenersRaw(musictag, strtol(value.c_str(), nullptr, 10));
            else if (key == "playcount")
              InfoTagMusic::setPlayCountRaw(musictag, strtol(value.c_str(), nullptr, 10));
            else if (key == "genre")
              InfoTagMusic::setGenresRaw(musictag, getMusicStringArray(alt, key, value));
            else if (key == "album")
              InfoTagMusic::setAlbumRaw(musictag, value);
            else if (key == "artist")
              InfoTagMusic::setArtistRaw(musictag, value);
            else if (key == "title")
              InfoTagMusic::setTitleRaw(musictag, value);
            else if (key == "rating")
              InfoTagMusic::setRatingRaw(musictag,
                                         static_cast<float>(strtod(value.c_str(), nullptr)));
            else if (key == "userrating")
              InfoTagMusic::setUserRatingRaw(musictag, strtol(value.c_str(), nullptr, 10));
            else if (key == "lyrics")
              InfoTagMusic::setLyricsRaw(musictag, value);
            else if (key == "lastplayed")
              InfoTagMusic::setLastPlayedRaw(musictag, value);
            else if (key == "musicbrainztrackid")
              InfoTagMusic::setMusicBrainzTrackIDRaw(musictag, value);
            else if (key == "musicbrainzartistid")
              InfoTagMusic::setMusicBrainzArtistIDRaw(musictag,
                                                      getMusicStringArray(alt, key, value));
            else if (key == "musicbrainzalbumid")
              InfoTagMusic::setMusicBrainzAlbumIDRaw(musictag, value);
            else if (key == "musicbrainzalbumartistid")
              InfoTagMusic::setMusicBrainzAlbumArtistIDRaw(musictag,
                                                           getMusicStringArray(alt, key, value));
            else if (key == "comment")
              InfoTagMusic::setCommentRaw(musictag, value);
            else
              CLog::Log(LOGERROR, "NEWADDON Unknown Music Info Key \"{}\"", key);
          }

          // This should probably be set outside of the loop but since the original
          //  implementation set it inside of the loop, I'll leave it that way. - Jim C.
          musictag->SetLoaded(true);
        }

        if (dbId > 0 && !mediaType.empty())
          InfoTagMusic::setDbIdRaw(musictag, dbId, mediaType);

        if (hasDeprecatedInfoLabel)
        {
          CLog::Log(
              LOGWARNING,
              "Setting most music properties through ListItem.setInfo() is deprecated and might be "
              "removed in future Kodi versions. Please use the respective setter in InfoTagMusic.");
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
            hasDeprecatedInfoLabel = true;

            String exifkey = key;
            if (!StringUtils::StartsWithNoCase(exifkey, "exif:") || exifkey.length() < 6)
            {
              CLog::Log(LOGWARNING, "ListItem.setInfo: unknown pictures info key \"{}\"", key);
              continue;
            }

            exifkey = StringUtils::Mid(exifkey, 5);
            if (exifkey == "resolution")
              xbmc::InfoTagPicture::setResolutionRaw(item->GetPictureInfoTag(), value);
            else if (exifkey == "exiftime")
              xbmc::InfoTagPicture::setDateTimeTakenRaw(item->GetPictureInfoTag(), value);
            else
              CLog::Log(LOGWARNING, "ListItem.setInfo: unknown pictures info key \"{}\"", key);
          }
        }

        if (hasDeprecatedInfoLabel)
        {
          CLog::Log(LOGWARNING, "Setting most picture properties through ListItem.setInfo() is "
                                "deprecated and might be removed in future Kodi versions. Please "
                                "use the respective setter in InfoTagPicture.");
        }
      }
      else if (StringUtils::EqualsNoCase(type, "game"))
      {
        auto gametag = item->GetGameInfoTag();
        for (const auto& it : infoLabels)
        {
          const auto key = StringUtils::ToLower(it.first);
          const auto& alt = it.second;
          const String value(alt.which() == first ? alt.former() : emptyString);

          if (key == "title")
          {
            setTitleRaw(value);
            xbmc::InfoTagGame::setTitleRaw(gametag, value);
          }
          else if (key == "platform")
            xbmc::InfoTagGame::setPlatformRaw(gametag, value);
          else if (key == "genres")
            xbmc::InfoTagGame::setGenresRaw(gametag, getStringArray(alt, key, value, ","));
          else if (key == "publisher")
            xbmc::InfoTagGame::setPublisherRaw(gametag, value);
          else if (key == "developer")
            xbmc::InfoTagGame::setDeveloperRaw(gametag, value);
          else if (key == "overview")
            xbmc::InfoTagGame::setOverviewRaw(gametag, value);
          else if (key == "year")
            xbmc::InfoTagGame::setYearRaw(gametag, strtoul(value.c_str(), nullptr, 10));
          else if (key == "gameclient")
            xbmc::InfoTagGame::setGameClientRaw(gametag, value);
        }

        if (!infoLabels.empty())
        {
          CLog::Log(
              LOGWARNING,
              "Setting game properties through ListItem.setInfo() is deprecated and might be "
              "removed in future Kodi versions. Please use the respective setter in InfoTagGame.");
        }
      }
      else
        CLog::Log(LOGWARNING, "ListItem.setInfo: unknown \"type\" parameter value: {}", type);
    } // end ListItem::setInfo

    void ListItem::setCast(const std::vector<Properties>& actors)
    {
      CLog::Log(LOGWARNING,
                "ListItem.setCast() is deprecated and might be removed in future Kodi versions. "
                "Please use InfoTagVideo.setCast().");

      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      std::vector<SActorInfo> cast;
      cast.reserve(actors.size());
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
        cast.push_back(std::move(info));
      }
      xbmc::InfoTagVideo::setCastRaw(GetVideoInfoTag(), std::move(cast));
    }

    void ListItem::setAvailableFanart(const std::vector<Properties>& images)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      auto infoTag = GetVideoInfoTag();
      infoTag->m_fanart.Clear();
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
        infoTag->m_fanart.AddFanart(image, preview, colors);
      }
      infoTag->m_fanart.Pack();
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
      CLog::Log(LOGWARNING, "ListItem.addAvailableArtwork() is deprecated and might be removed in "
                            "future Kodi versions. Please use InfoTagVideo.addAvailableArtwork().");

      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      xbmc::InfoTagVideo::addAvailableArtworkRaw(GetVideoInfoTag(), url, art_type, preview,
                                                 referrer, cache, post, isgz, season);
    }

    void ListItem::addStreamInfo(const char* cType, const Properties& dictionary)
    {
      CLog::Log(
          LOGWARNING,
          "ListItem.addStreamInfo() is deprecated and might be removed in future Kodi versions. "
          "Please use InfoTagVideo.addVideoStream(), InfoTagVideo.addAudioStream() and "
          "InfoTagVideo.addSubtitleStream().");

      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);

      auto infoTag = GetVideoInfoTag();
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
        xbmc::InfoTagVideo::addStreamRaw(infoTag, video);
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
        xbmc::InfoTagVideo::addStreamRaw(infoTag, audio);
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
        xbmc::InfoTagVideo::addStreamRaw(infoTag, subtitle);
      }
      xbmc::InfoTagVideo::finalizeStreamsRaw(infoTag);
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
      return new xbmc::InfoTagVideo(GetVideoInfoTag(), m_offscreen);
    }

    xbmc::InfoTagMusic* ListItem::getMusicInfoTag()
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      return new xbmc::InfoTagMusic(GetMusicInfoTag(), m_offscreen);
    }

    xbmc::InfoTagPicture* ListItem::getPictureInfoTag()
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      return new xbmc::InfoTagPicture(item->GetPictureInfoTag(), m_offscreen);
    }

    xbmc::InfoTagGame* ListItem::getGameInfoTag()
    {
      XBMCAddonUtils::GuiLock lock(languageHook, m_offscreen);
      return new xbmc::InfoTagGame(item->GetGameInfoTag(), m_offscreen);
    }

    std::vector<std::string> ListItem::getStringArray(const InfoLabelValue& alt,
                                                      const std::string& tag,
                                                      std::string value,
                                                      const std::string& separator)
    {
      if (alt.which() == first)
      {
        if (value.empty())
          value = alt.former();
        return StringUtils::Split(value, separator);
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

    std::vector<std::string> ListItem::getVideoStringArray(const InfoLabelValue& alt,
                                                           const std::string& tag,
                                                           std::string value /* = "" */)
    {
      return getStringArray(
          alt, tag, std::move(value),
          CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
    }

    std::vector<std::string> ListItem::getMusicStringArray(const InfoLabelValue& alt,
                                                           const std::string& tag,
                                                           std::string value /* = "" */)
    {
      return getStringArray(
          alt, tag, std::move(value),
          CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator);
    }

    CVideoInfoTag* ListItem::GetVideoInfoTag()
    {
      return item->GetVideoInfoTag();
    }

    const CVideoInfoTag* ListItem::GetVideoInfoTag() const
    {
      return item->GetVideoInfoTag();
    }

    MUSIC_INFO::CMusicInfoTag* ListItem::GetMusicInfoTag()
    {
      return item->GetMusicInfoTag();
    }

    const MUSIC_INFO::CMusicInfoTag* ListItem::GetMusicInfoTag() const
    {
      return item->GetMusicInfoTag();
    }

    void ListItem::setTitleRaw(std::string title)
    {
      item->m_strTitle = std::move(title);
    }

    void ListItem::setPathRaw(const std::string& path)
    {
      item->SetPath(path);
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
      item->SetStartOffset(CUtil::ConvertSecsToMilliSecs(startOffset));
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

    void ListItem::addArtRaw(std::string type, const std::string& url)
    {
      StringUtils::ToLower(type);
      item->SetArt(type, url);
    }

    void ListItem::addPropertyRaw(std::string type, const CVariant& value)
    {
      StringUtils::ToLower(type);
      item->SetProperty(type, value);
    }

    void ListItem::addSubtitlesRaw(const std::vector<std::string>& subtitles)
    {
      for (size_t i = 0; i < subtitles.size(); ++i)
        // subtitle:{} index starts from 1
        addPropertyRaw(StringUtils::Format("subtitle:{}", i + 1), subtitles[i]);
    }
  }
}
