/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InfoTagVideo.h"

#include "AddonUtils.h"
#include "ServiceBroker.h"
#include "interfaces/legacy/Exception.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/StringUtils.h"
#include "utils/log.h"

#include <utility>

namespace XBMCAddon
{
  namespace xbmc
  {
    Actor::Actor(const String& name /* = emptyString */,
                 const String& role /* = emptyString */,
                 int order /* = -1 */,
                 const String& thumbnail /* = emptyString */)
      : m_name(name), m_role(role), m_order(order), m_thumbnail(thumbnail)
    {
      if (m_name.empty())
        throw WrongTypeException("Actor: name property must not be empty");
    }

    SActorInfo Actor::ToActorInfo() const
    {
      SActorInfo actorInfo;
      actorInfo.strName = m_name;
      actorInfo.strRole = m_role;
      actorInfo.order = m_order;
      actorInfo.thumbUrl = CScraperUrl(m_thumbnail);
      if (!actorInfo.thumbUrl.GetFirstThumbUrl().empty())
        actorInfo.thumb = CScraperUrl::GetThumbUrl(actorInfo.thumbUrl.GetFirstUrlByType());

      return actorInfo;
    }

    VideoStreamDetail::VideoStreamDetail(int width /* = 0 */,
                                         int height /* = 0 */,
                                         float aspect /* = 0.0f */,
                                         int duration /* = 0 */,
                                         const String& codec /* = emptyString */,
                                         const String& stereoMode /* = emptyString */,
                                         const String& language /* = emptyString */,
                                         const String& hdrType /* = emptyString */)
      : m_width(width),
        m_height(height),
        m_aspect(aspect),
        m_duration(duration),
        m_codec(codec),
        m_stereoMode(stereoMode),
        m_language(language),
        m_hdrType(hdrType)
    {
    }

    CStreamDetailVideo* VideoStreamDetail::ToStreamDetailVideo() const
    {
      auto streamDetail = new CStreamDetailVideo();
      streamDetail->m_iWidth = m_width;
      streamDetail->m_iHeight = m_height;
      streamDetail->m_fAspect = m_aspect;
      streamDetail->m_iDuration = m_duration;
      streamDetail->m_strCodec = m_codec;
      streamDetail->m_strStereoMode = m_stereoMode;
      streamDetail->m_strLanguage = m_language;
      streamDetail->m_strHdrType = m_hdrType;

      return streamDetail;
    }

    AudioStreamDetail::AudioStreamDetail(int channels /* = -1 */,
                                         const String& codec /* = emptyString */,
                                         const String& language /* = emptyString */)
      : m_channels(channels), m_codec(codec), m_language(language)
    {
    }

    CStreamDetailAudio* AudioStreamDetail::ToStreamDetailAudio() const
    {
      auto streamDetail = new CStreamDetailAudio();
      streamDetail->m_iChannels = m_channels;
      streamDetail->m_strCodec = m_codec;
      streamDetail->m_strLanguage = m_language;

      return streamDetail;
    }

    SubtitleStreamDetail::SubtitleStreamDetail(const String& language /* = emptyString */)
      : m_language(language)
    {
    }

    CStreamDetailSubtitle* SubtitleStreamDetail::ToStreamDetailSubtitle() const
    {
      auto streamDetail = new CStreamDetailSubtitle();
      streamDetail->m_strLanguage = m_language;

      return streamDetail;
    }

    InfoTagVideo::InfoTagVideo(bool offscreen /* = false */)
      : infoTag(new CVideoInfoTag), offscreen(offscreen), owned(true)
    {
    }

    InfoTagVideo::InfoTagVideo(const CVideoInfoTag* tag)
      : infoTag(new CVideoInfoTag(*tag)), offscreen(true), owned(true)
    {
    }

    InfoTagVideo::InfoTagVideo(CVideoInfoTag* tag, bool offscreen /* = false */)
      : infoTag(tag), offscreen(offscreen), owned(false)
    {
    }

    InfoTagVideo::~InfoTagVideo()
    {
      if (owned)
        delete infoTag;
    }

    int InfoTagVideo::getDbId() const {
      return infoTag->m_iDbId;
    }

    String InfoTagVideo::getDirector() const {
      return StringUtils::Join(infoTag->m_director, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
    }

    std::vector<String> InfoTagVideo::getDirectors() const {
      return infoTag->m_director;
    }

    String InfoTagVideo::getWritingCredits() const {
      return StringUtils::Join(infoTag->m_writingCredits, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
    }

    std::vector<String> InfoTagVideo::getWriters() const {
      return infoTag->m_writingCredits;
    }

    String InfoTagVideo::getGenre() const {
      return StringUtils::Join(infoTag->m_genre, CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_videoItemSeparator);
    }

    std::vector<String> InfoTagVideo::getGenres() const {
      return infoTag->m_genre;
    }

    String InfoTagVideo::getTagLine() const {
      return infoTag->m_strTagLine;
    }

    String InfoTagVideo::getPlotOutline() const {
      return infoTag->m_strPlotOutline;
    }

    String InfoTagVideo::getPlot() const {
      return infoTag->m_strPlot;
    }

    String InfoTagVideo::getPictureURL() const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      infoTag->m_strPictureURL.Parse();
      return infoTag->m_strPictureURL.GetFirstThumbUrl();
    }

    String InfoTagVideo::getTVShowTitle() const {
      return infoTag->m_strShowTitle;
    }

    String InfoTagVideo::getTitle() const {
      return infoTag->m_strTitle;
    }

    String InfoTagVideo::getMediaType() const {
      return infoTag->m_type;
    }

    String InfoTagVideo::getVotes()
    {
      CLog::Log(
          LOGWARNING,
          "InfoTagVideo.getVotes() is deprecated and might be removed in future Kodi versions. "
          "Please use InfoTagVideo.getVotesAsInt().");

      return std::to_string(getVotesAsInt());
    }

    int InfoTagVideo::getVotesAsInt(const String& type /* = "" */) const {
      return infoTag->GetRating(type).votes;
    }

    String InfoTagVideo::getCast() const {
      return infoTag->GetCast(true);
    }

    std::vector<Actor*> InfoTagVideo::getActors() const {
      std::vector<Actor*> actors;
      actors.reserve(infoTag->m_cast.size());

      for (const auto& cast : infoTag->m_cast)
        actors.push_back(new Actor(cast.strName, cast.strRole, cast.order, cast.thumbUrl.GetFirstUrlByType().m_url));

      return actors;
    }

    String InfoTagVideo::getFile() const {
      return infoTag->m_strFile;
    }

    String InfoTagVideo::getPath() const {
      return infoTag->m_strPath;
    }

    String InfoTagVideo::getFilenameAndPath() const {
      return infoTag->m_strFileNameAndPath;
    }

    String InfoTagVideo::getIMDBNumber() const {
      return infoTag->GetUniqueID();
    }

    int InfoTagVideo::getSeason() const {
      return infoTag->m_iSeason;
    }

    int InfoTagVideo::getEpisode() const {
      return infoTag->m_iEpisode;
    }

    int InfoTagVideo::getYear() const {
      return infoTag->GetYear();
    }

    double InfoTagVideo::getRating(const String& type /* = "" */) const {
      return infoTag->GetRating(type).rating;
    }

    int InfoTagVideo::getUserRating() const {
      return infoTag->m_iUserRating;
    }

    int InfoTagVideo::getPlayCount() const {
      return infoTag->GetPlayCount();
    }

    String InfoTagVideo::getLastPlayed() const {
      CLog::Log(LOGWARNING, "InfoTagVideo.getLastPlayed() is deprecated and might be removed in "
                            "future Kodi versions. Please use InfoTagVideo.getLastPlayedAsW3C().");

      return infoTag->m_lastPlayed.GetAsLocalizedDateTime();
    }

    String InfoTagVideo::getLastPlayedAsW3C() const {
      return infoTag->m_lastPlayed.GetAsW3CDateTime();
    }

    String InfoTagVideo::getOriginalTitle() const {
      return infoTag->m_strOriginalTitle;
    }

    String InfoTagVideo::getPremiered() const {
      CLog::Log(LOGWARNING, "InfoTagVideo.getPremiered() is deprecated and might be removed in "
                            "future Kodi versions. Please use InfoTagVideo.getPremieredAsW3C().");

      return infoTag->GetPremiered().GetAsLocalizedDate();
    }

    String InfoTagVideo::getPremieredAsW3C() const {
      return infoTag->GetPremiered().GetAsW3CDate();
    }

    String InfoTagVideo::getFirstAired() const {
      CLog::Log(LOGWARNING, "InfoTagVideo.getFirstAired() is deprecated and might be removed in "
                            "future Kodi versions. Please use InfoTagVideo.getFirstAiredAsW3C().");

      return infoTag->m_firstAired.GetAsLocalizedDate();
    }

    String InfoTagVideo::getFirstAiredAsW3C() const {
      return infoTag->m_firstAired.GetAsW3CDate();
    }

    String InfoTagVideo::getTrailer() const {
      return infoTag->m_strTrailer;
    }

    std::vector<std::string> InfoTagVideo::getArtist() const {
      return infoTag->m_artist;
    }

    String InfoTagVideo::getAlbum() const {
      return infoTag->m_strAlbum;
    }

    int InfoTagVideo::getTrack() const {
      return infoTag->m_iTrack;
    }

    unsigned int InfoTagVideo::getDuration() const {
      return infoTag->GetDuration();
    }

    double InfoTagVideo::getResumeTime() const {
      return infoTag->GetResumePoint().timeInSeconds;
    }

    double InfoTagVideo::getResumeTimeTotal() const {
      return infoTag->GetResumePoint().totalTimeInSeconds;
    }

    String InfoTagVideo::getUniqueID(const char* key) const {
      return infoTag->GetUniqueID(key);
    }

    void InfoTagVideo::setUniqueID(const String& uniqueID,
                                   const String& type /* = "" */,
                                   bool isDefault /* = false */) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setUniqueIDRaw(infoTag, uniqueID, type, isDefault);
    }

    void InfoTagVideo::setUniqueIDs(const std::map<String, String>& uniqueIDs,
                                    const String& defaultUniqueID /* = "" */) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setUniqueIDsRaw(infoTag, uniqueIDs, defaultUniqueID);
    }

    void InfoTagVideo::setDbId(int dbId) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setDbIdRaw(infoTag, dbId);
    }

    void InfoTagVideo::setYear(int year) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setYearRaw(infoTag, year);
    }

    void InfoTagVideo::setEpisode(int episode) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setEpisodeRaw(infoTag, episode);
    }

    void InfoTagVideo::setSeason(int season) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setSeasonRaw(infoTag, season);
    }

    void InfoTagVideo::setSortEpisode(int sortEpisode) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setSortEpisodeRaw(infoTag, sortEpisode);
    }

    void InfoTagVideo::setSortSeason(int sortSeason) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setSortSeasonRaw(infoTag, sortSeason);
    }

    void InfoTagVideo::setEpisodeGuide(const String& episodeGuide) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setEpisodeGuideRaw(infoTag, episodeGuide);
    }

    void InfoTagVideo::setTop250(int top250) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setTop250Raw(infoTag, top250);
    }

    void InfoTagVideo::setSetId(int setId) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setSetIdRaw(infoTag, setId);
    }

    void InfoTagVideo::setTrackNumber(int trackNumber) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setTrackNumberRaw(infoTag, trackNumber);
    }

    void InfoTagVideo::setRating(float rating,
                                 int votes /* = 0 */,
                                 const String& type /* = "" */,
                                 bool isDefault /* = false */) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setRatingRaw(infoTag, rating, votes, type, isDefault);
    }

    void InfoTagVideo::setRatings(const std::map<String, Tuple<float, int>>& ratings,
                                  const String& defaultRating /* = "" */) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setRatingsRaw(infoTag, ratings, defaultRating);
    }

    void InfoTagVideo::setUserRating(int userRating) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setUserRatingRaw(infoTag, userRating);
    }

    void InfoTagVideo::setPlaycount(int playcount) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setPlaycountRaw(infoTag, playcount);
    }

    void InfoTagVideo::setMpaa(const String& mpaa) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setMpaaRaw(infoTag, mpaa);
    }

    void InfoTagVideo::setPlot(const String& plot) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setPlotRaw(infoTag, plot);
    }

    void InfoTagVideo::setPlotOutline(const String& plotOutline) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setPlotOutlineRaw(infoTag, plotOutline);
    }

    void InfoTagVideo::setTitle(const String& title) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setTitleRaw(infoTag, title);
    }

    void InfoTagVideo::setOriginalTitle(const String& originalTitle) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setOriginalTitleRaw(infoTag, originalTitle);
    }

    void InfoTagVideo::setSortTitle(const String& sortTitle) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setSortTitleRaw(infoTag, sortTitle);
    }

    void InfoTagVideo::setTagLine(const String& tagLine) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setTagLineRaw(infoTag, tagLine);
    }

    void InfoTagVideo::setTvShowTitle(const String& tvshowTitle) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setTvShowTitleRaw(infoTag, tvshowTitle);
    }

    void InfoTagVideo::setTvShowStatus(const String& tvshowStatus) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setTvShowStatusRaw(infoTag, tvshowStatus);
    }

    void InfoTagVideo::setGenres(std::vector<String> genre) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setGenresRaw(infoTag, std::move(genre));
    }

    void InfoTagVideo::setCountries(std::vector<String> countries) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setCountriesRaw(infoTag, std::move(countries));
    }

    void InfoTagVideo::setDirectors(std::vector<String> directors) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setDirectorsRaw(infoTag, std::move(directors));
    }

    void InfoTagVideo::setStudios(std::vector<String> studios) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setStudiosRaw(infoTag, std::move(studios));
    }

    void InfoTagVideo::setWriters(std::vector<String> writers) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setWritersRaw(infoTag, std::move(writers));
    }

    void InfoTagVideo::setDuration(int duration) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setDurationRaw(infoTag, duration);
    }

    void InfoTagVideo::setPremiered(const String& premiered) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setPremieredRaw(infoTag, premiered);
    }

    void InfoTagVideo::setSet(const String& set) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setSetRaw(infoTag, set);
    }

    void InfoTagVideo::setSetOverview(const String& setOverview) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setSetOverviewRaw(infoTag, setOverview);
    }

    void InfoTagVideo::setTags(std::vector<String> tags) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setTagsRaw(infoTag, std::move(tags));
    }

    void InfoTagVideo::setVideoAssetTitle(const String& videoAssetTitle) const {
      setVideoAssetTitleRaw(infoTag, videoAssetTitle);
    }

    void InfoTagVideo::setProductionCode(const String& productionCode) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setProductionCodeRaw(infoTag, productionCode);
    }

    void InfoTagVideo::setFirstAired(const String& firstAired) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setFirstAiredRaw(infoTag, firstAired);
    }

    void InfoTagVideo::setLastPlayed(const String& lastPlayed) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setLastPlayedRaw(infoTag, lastPlayed);
    }

    void InfoTagVideo::setAlbum(const String& album) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setAlbumRaw(infoTag, album);
    }

    void InfoTagVideo::setVotes(int votes) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setVotesRaw(infoTag, votes);
    }

    void InfoTagVideo::setTrailer(const String& trailer) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setTrailerRaw(infoTag, trailer);
    }

    void InfoTagVideo::setPath(const String& path) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setPathRaw(infoTag, path);
    }

    void InfoTagVideo::setFilenameAndPath(const String& filenameAndPath) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setFilenameAndPathRaw(infoTag, filenameAndPath);
    }

    void InfoTagVideo::setIMDBNumber(const String& imdbNumber) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setIMDBNumberRaw(infoTag, imdbNumber);
    }

    void InfoTagVideo::setDateAdded(const String& dateAdded) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setDateAddedRaw(infoTag, dateAdded);
    }

    void InfoTagVideo::setMediaType(const String& mediaType) const {
      setMediaTypeRaw(infoTag, mediaType);
    }

    void InfoTagVideo::setShowLinks(std::vector<String> showLinks) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setShowLinksRaw(infoTag, std::move(showLinks));
    }

    void InfoTagVideo::setArtists(std::vector<String> artists) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setArtistsRaw(infoTag, std::move(artists));
    }

    void InfoTagVideo::setCast(const std::vector<const Actor*>& actors) const {
      std::vector<SActorInfo> cast;
      cast.reserve(actors.size());
      for (const auto& actor : actors)
        cast.push_back(actor->ToActorInfo());

      {
        XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
        setCastRaw(infoTag, std::move(cast));
      }
    }

    void InfoTagVideo::setResumePoint(double time, double totalTime /* = 0.0 */) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setResumePointRaw(infoTag, time, totalTime);
    }

    void InfoTagVideo::addSeason(int number, std::string name /* = "" */) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      addSeasonRaw(infoTag, number, std::move(name));
    }

    void InfoTagVideo::addSeasons(const std::vector<Tuple<int, std::string>>& namedSeasons) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      addSeasonsRaw(infoTag, namedSeasons);
    }

    void InfoTagVideo::addVideoStream(const VideoStreamDetail* stream) const {
      if (stream == nullptr)
        return;

      auto streamDetail = stream->ToStreamDetailVideo();
      {
        XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
        addStreamRaw(infoTag, streamDetail);
      }
    }

    void InfoTagVideo::addAudioStream(const AudioStreamDetail* stream) const {
      if (stream == nullptr)
        return;

      auto streamDetail = stream->ToStreamDetailAudio();
      {
        XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
        addStreamRaw(infoTag, streamDetail);
      }
    }

    void InfoTagVideo::addSubtitleStream(const SubtitleStreamDetail* stream) const {
      if (stream == nullptr)
        return;

      auto streamDetail = stream->ToStreamDetailSubtitle();
      {
        XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
        addStreamRaw(infoTag, streamDetail);
      }
    }

    void InfoTagVideo::addAvailableArtwork(const std::string& url,
                                           const std::string& art_type,
                                           const std::string& preview,
                                           const std::string& referrer,
                                           const std::string& cache,
                                           bool post,
                                           bool isgz,
                                           int season) const {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      addAvailableArtworkRaw(infoTag, url, art_type, preview, referrer, cache, post, isgz, season);
    }

    void InfoTagVideo::setAvailableFanart(const std::vector<Properties>& images)
    {
      XBMCAddonUtils::GuiLock lock(languageHook, offscreen);
      setAvailableFanartRaw(infoTag, images);
    }

    void InfoTagVideo::setDbIdRaw(CVideoInfoTag* infoTag, int dbId)
    {
      infoTag->m_iDbId = dbId;
    }

    void InfoTagVideo::setUniqueIDRaw(CVideoInfoTag* infoTag,
                                      const String& uniqueID,
                                      const String& type /* = "" */,
                                      bool isDefault /* = false */)
    {
      infoTag->SetUniqueID(uniqueID, type, isDefault);
    }

    void InfoTagVideo::setUniqueIDsRaw(CVideoInfoTag* infoTag,
                                       std::map<String, String> uniqueIDs,
                                       const String& defaultUniqueID /* = "" */)
    {
      infoTag->SetUniqueIDs(uniqueIDs);
      auto defaultUniqueIDEntry = uniqueIDs.find(defaultUniqueID);
      if (defaultUniqueIDEntry != uniqueIDs.end())
        infoTag->SetUniqueID(defaultUniqueIDEntry->second, defaultUniqueIDEntry->first, true);
    }

    void InfoTagVideo::setYearRaw(CVideoInfoTag* infoTag, int year)
    {
      infoTag->SetYear(year);
    }

    void InfoTagVideo::setEpisodeRaw(CVideoInfoTag* infoTag, int episode)
    {
      infoTag->m_iEpisode = episode;
    }

    void InfoTagVideo::setSeasonRaw(CVideoInfoTag* infoTag, int season)
    {
      infoTag->m_iSeason = season;
    }

    void InfoTagVideo::setSortEpisodeRaw(CVideoInfoTag* infoTag, int sortEpisode)
    {
      infoTag->m_iSpecialSortEpisode = sortEpisode;
    }

    void InfoTagVideo::setSortSeasonRaw(CVideoInfoTag* infoTag, int sortSeason)
    {
      infoTag->m_iSpecialSortSeason = sortSeason;
    }

    void InfoTagVideo::setEpisodeGuideRaw(CVideoInfoTag* infoTag, const String& episodeGuide)
    {
      infoTag->SetEpisodeGuide(episodeGuide);
    }

    void InfoTagVideo::setTop250Raw(CVideoInfoTag* infoTag, int top250)
    {
      infoTag->m_iTop250 = top250;
    }

    void InfoTagVideo::setSetIdRaw(CVideoInfoTag* infoTag, int setId)
    {
      infoTag->m_set.id = setId;
    }

    void InfoTagVideo::setTrackNumberRaw(CVideoInfoTag* infoTag, int trackNumber)
    {
      infoTag->m_iTrack = trackNumber;
    }

    void InfoTagVideo::setRatingRaw(CVideoInfoTag* infoTag,
                                    float rating,
                                    int votes /* = 0 */,
                                    const std::string& type /* = "" */,
                                    bool isDefault /* = false */)
    {
      infoTag->SetRating(rating, votes, type, isDefault);
    }

    void InfoTagVideo::setRatingsRaw(CVideoInfoTag* infoTag,
                                     const std::map<String, Tuple<float, int>>& ratings,
                                     const String& defaultRating /* = "" */)
    {
      RatingMap ratingMap;
      for (const auto& rating : ratings)
        ratingMap.emplace(rating.first, CRating{rating.second.first(), rating.second.second()});

      infoTag->SetRatings(std::move(ratingMap), defaultRating);
    }

    void InfoTagVideo::setUserRatingRaw(CVideoInfoTag* infoTag, int userRating)
    {
      infoTag->m_iUserRating = userRating;
    }

    void InfoTagVideo::setPlaycountRaw(CVideoInfoTag* infoTag, int playcount)
    {
      infoTag->SetPlayCount(playcount);
    }

    void InfoTagVideo::setMpaaRaw(CVideoInfoTag* infoTag, const String& mpaa)
    {
      infoTag->SetMPAARating(mpaa);
    }

    void InfoTagVideo::setPlotRaw(CVideoInfoTag* infoTag, const String& plot)
    {
      infoTag->SetPlot(plot);
    }

    void InfoTagVideo::setPlotOutlineRaw(CVideoInfoTag* infoTag, const String& plotOutline)
    {
      infoTag->SetPlotOutline(plotOutline);
    }

    void InfoTagVideo::setTitleRaw(CVideoInfoTag* infoTag, const String& title)
    {
      infoTag->SetTitle(title);
    }

    void InfoTagVideo::setOriginalTitleRaw(CVideoInfoTag* infoTag, const String& originalTitle)
    {
      infoTag->SetOriginalTitle(originalTitle);
    }

    void InfoTagVideo::setSortTitleRaw(CVideoInfoTag* infoTag, const String& sortTitle)
    {
      infoTag->SetSortTitle(sortTitle);
    }

    void InfoTagVideo::setTagLineRaw(CVideoInfoTag* infoTag, const String& tagLine)
    {
      infoTag->SetTagLine(tagLine);
    }

    void InfoTagVideo::setTvShowTitleRaw(CVideoInfoTag* infoTag, const String& tvshowTitle)
    {
      infoTag->SetShowTitle(tvshowTitle);
    }

    void InfoTagVideo::setTvShowStatusRaw(CVideoInfoTag* infoTag, const String& tvshowStatus)
    {
      infoTag->SetStatus(tvshowStatus);
    }

    void InfoTagVideo::setGenresRaw(CVideoInfoTag* infoTag, std::vector<String> genre)
    {
      infoTag->SetGenre(std::move(genre));
    }

    void InfoTagVideo::setCountriesRaw(CVideoInfoTag* infoTag, std::vector<String> countries)
    {
      infoTag->SetCountry(std::move(countries));
    }

    void InfoTagVideo::setDirectorsRaw(CVideoInfoTag* infoTag, std::vector<String> directors)
    {
      infoTag->SetDirector(std::move(directors));
    }

    void InfoTagVideo::setStudiosRaw(CVideoInfoTag* infoTag, std::vector<String> studios)
    {
      infoTag->SetStudio(std::move(studios));
    }

    void InfoTagVideo::setWritersRaw(CVideoInfoTag* infoTag, std::vector<String> writers)
    {
      infoTag->SetWritingCredits(std::move(writers));
    }

    void InfoTagVideo::setDurationRaw(CVideoInfoTag* infoTag, int duration)
    {
      infoTag->SetDuration(duration);
    }

    void InfoTagVideo::setPremieredRaw(CVideoInfoTag* infoTag, const String& premiered)
    {
      CDateTime premieredDate;
      premieredDate.SetFromDateString(premiered);
      infoTag->SetPremiered(premieredDate);
    }

    void InfoTagVideo::setSetRaw(CVideoInfoTag* infoTag, const String& set)
    {
      infoTag->SetSet(set);
    }

    void InfoTagVideo::setSetOverviewRaw(CVideoInfoTag* infoTag, const String& setOverview)
    {
      infoTag->SetSetOverview(setOverview);
    }

    void InfoTagVideo::setTagsRaw(CVideoInfoTag* infoTag, std::vector<String> tags)
    {
      infoTag->SetTags(std::move(tags));
    }

    void InfoTagVideo::setVideoAssetTitleRaw(CVideoInfoTag* infoTag, const String& videoAssetTitle)
    {
      infoTag->GetAssetInfo().SetTitle(videoAssetTitle);
    }

    void InfoTagVideo::setProductionCodeRaw(CVideoInfoTag* infoTag, const String& productionCode)
    {
      infoTag->SetProductionCode(productionCode);
    }

    void InfoTagVideo::setFirstAiredRaw(CVideoInfoTag* infoTag, const String& firstAired)
    {
      CDateTime firstAiredDate;
      firstAiredDate.SetFromDateString(firstAired);
      infoTag->m_firstAired = firstAiredDate;
    }

    void InfoTagVideo::setLastPlayedRaw(CVideoInfoTag* infoTag, const String& lastPlayed)
    {
      CDateTime lastPlayedDate;
      lastPlayedDate.SetFromDBDateTime(lastPlayed);
      infoTag->m_lastPlayed = lastPlayedDate;
    }

    void InfoTagVideo::setAlbumRaw(CVideoInfoTag* infoTag, const String& album)
    {
      infoTag->SetAlbum(album);
    }

    void InfoTagVideo::setVotesRaw(CVideoInfoTag* infoTag, int votes)
    {
      infoTag->SetVotes(votes);
    }

    void InfoTagVideo::setTrailerRaw(CVideoInfoTag* infoTag, const String& trailer)
    {
      infoTag->SetTrailer(trailer);
    }

    void InfoTagVideo::setPathRaw(CVideoInfoTag* infoTag, const String& path)
    {
      infoTag->SetPath(path);
    }

    void InfoTagVideo::setFilenameAndPathRaw(CVideoInfoTag* infoTag, const String& filenameAndPath)
    {
      infoTag->SetFileNameAndPath(filenameAndPath);
    }

    void InfoTagVideo::setIMDBNumberRaw(CVideoInfoTag* infoTag, const String& imdbNumber)
    {
      infoTag->SetUniqueID(imdbNumber);
    }

    void InfoTagVideo::setDateAddedRaw(CVideoInfoTag* infoTag, const String& dateAdded)
    {
      CDateTime dateAddedDate;
      dateAddedDate.SetFromDBDateTime(dateAdded);
      infoTag->m_dateAdded = dateAddedDate;
    }

    void InfoTagVideo::setMediaTypeRaw(CVideoInfoTag* infoTag, const String& mediaType)
    {
      if (CMediaTypes::IsValidMediaType(mediaType))
        infoTag->m_type = mediaType;
    }

    void InfoTagVideo::setShowLinksRaw(CVideoInfoTag* infoTag, std::vector<String> showLinks)
    {
      infoTag->SetShowLink(std::move(showLinks));
    }

    void InfoTagVideo::setArtistsRaw(CVideoInfoTag* infoTag, std::vector<String> artists)
    {
      infoTag->m_artist = std::move(artists);
    }

    void InfoTagVideo::setCastRaw(CVideoInfoTag* infoTag, std::vector<SActorInfo> cast)
    {
      infoTag->m_cast = std::move(cast);
    }

    void InfoTagVideo::setResumePointRaw(CVideoInfoTag* infoTag,
                                         double time,
                                         double totalTime /* = 0.0 */)
    {
      auto resumePoint = infoTag->GetResumePoint();
      resumePoint.timeInSeconds = time;
      if (totalTime > 0.0)
        resumePoint.totalTimeInSeconds = totalTime;
      infoTag->SetResumePoint(resumePoint);
    }

    void InfoTagVideo::addSeasonRaw(CVideoInfoTag* infoTag, int number, std::string name /* = "" */)
    {
      infoTag->m_namedSeasons[number] = std::move(name);
    }

    void InfoTagVideo::addSeasonsRaw(CVideoInfoTag* infoTag,
                                     const std::vector<Tuple<int, std::string>>& namedSeasons)
    {
      for (const auto& season : namedSeasons)
        addSeasonRaw(infoTag, season.first(), season.second());
    }

    void InfoTagVideo::addStreamRaw(CVideoInfoTag* infoTag, CStreamDetail* stream)
    {
      infoTag->m_streamDetails.AddStream(stream);
    }

    void InfoTagVideo::finalizeStreamsRaw(CVideoInfoTag* infoTag)
    {
      infoTag->m_streamDetails.DetermineBestStreams();
    }

    void InfoTagVideo::addAvailableArtworkRaw(CVideoInfoTag* infoTag,
                                              const std::string& url,
                                              const std::string& art_type,
                                              const std::string& preview,
                                              const std::string& referrer,
                                              const std::string& cache,
                                              bool post,
                                              bool isgz,
                                              int season)
    {
      infoTag->m_strPictureURL.AddParsedUrl(url, art_type, preview, referrer, cache, post, isgz,
                                            season);
    }

    void InfoTagVideo::setAvailableFanartRaw(CVideoInfoTag* infoTag,
                                             const std::vector<Properties>& images)
    {
      infoTag->m_fanart.Clear();
      for (const auto& dictionary : images)
      {
        auto getValue = [&](std::string_view str) -> const std::string&
        {
          const auto iter = dictionary.find(str);
          if (iter != dictionary.end())
            return iter->second;
          else
            return StringUtils::Empty;
        };

        if (const std::string& image = getValue("image"); !image.empty())
        {
          const std::string& preview = getValue("preview");
          const std::string& colors = getValue("colors");
          infoTag->m_fanart.AddFanart(image, preview, colors);
        }
      }
      infoTag->m_fanart.Pack();
    }
  }
}
