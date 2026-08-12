/*
 *  Copyright (C) 2005-2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */
#include "DiscDirectoryHelper.h"

#include "FileItem.h"
#include "FileItemList.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "dialogs/GUIDialogBusy.h"
#include "dialogs/GUIDialogOK.h"
#include "dialogs/GUIDialogSimpleMenu.h"
#include "resources/LocalizeStrings.h"
#include "resources/ResourcesComponent.h"
#include "settings/AdvancedSettings.h"
#include "settings/Settings.h"
#include "settings/SettingsComponent.h"
#include "settings/VideoVersionsSettings.h"
#include "threads/IRunnable.h"
#include "utils/RegExp.h"
#include "utils/StreamUtils.h"
#include "utils/StringUtils.h"
#include "utils/URIUtils.h"
#include "utils/log.h"
#include "video/VideoDatabase.h"
#include "video/VideoManagerTypes.h"

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <iterator>
#include <map>
#include <memory>
#include <numeric>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <unordered_set>
#include <utility>
#include <vector>

#include <fmt/format.h>
#include <fmt/ranges.h>

using namespace XFILE;
using namespace std::chrono_literals;

using PlaylistMapEntry = std::pair<unsigned int, PlaylistInformation>;
using PlaylistVector = std::vector<std::pair<unsigned int, PlaylistInformation>>;
using PlaylistVectorEntry = std::pair<unsigned int, PlaylistInformation>;

namespace
{
class CGetDirectoryItems : public IRunnable
{
public:
  CGetDirectoryItems(std::string path, CFileItemList& items, CDirectory::CHints hints)
    : m_path(std::move(path)),
      m_items(&items),
      m_hints(std::move(hints))
  {
  }
  void Run() override { m_result = CDirectory::GetDirectory(m_path, *m_items, m_hints); }
  bool m_result{false};

private:
  std::string m_path;
  CFileItemList* m_items;
  CDirectory::CHints m_hints;
};
} // namespace

CDiscDirectoryHelper::CDiscDirectoryHelper()
{
  Reset();

  m_minEpisodeDuration = std::chrono::milliseconds(CServiceBroker::GetSettingsComponent()
                                                       ->GetAdvancedSettings()
                                                       ->m_minimumEpisodePlaylistDuration *
                                                   1000);
}

void CDiscDirectoryHelper::Reset()
{
  m_allEpisodes = AllEpisodes::SINGLE;
  m_isSpecial = IsSpecial::EPISODE;
  m_numEpisodes = 0;
  m_numSpecials = 0;
  m_playAllPlaylists.clear();
  m_playAllPlaylistsMap.clear();
  m_playAllPlaylistEpisodeMap.clear();
  m_groups.clear();
  m_allGroups.clear();
  m_candidatePlaylists.clear();
  m_candidateSpecials.clear();
  m_nthLongestPlaylists.clear();
}

CDiscDirectoryHelper::CDiscDirectoryHelper(StreamDetailsProvider getStreamDetails)
  : CDiscDirectoryHelper()
{
  m_getStreamDetails = std::move(getStreamDetails);
}

void CDiscDirectoryHelper::InitialiseEpisodePlaylistSearch(int episodeIndex,
                                                           const Episodes& episodesOnDisc)
{
  // Need to differentiate between specials and episodes
  m_allEpisodes = episodeIndex == -1 ? AllEpisodes::ALL : AllEpisodes::SINGLE;
  m_isSpecial = m_allEpisodes == AllEpisodes::SINGLE && episodesOnDisc[episodeIndex].iSeason == 0
                    ? IsSpecial::SPECIAL
                    : IsSpecial::EPISODE;

  m_numEpisodes = static_cast<unsigned int>(
      std::ranges::count_if(episodesOnDisc, [](const Episode& e) { return e.iSeason > 0; }));
  m_numSpecials = static_cast<unsigned int>(episodesOnDisc.size()) - m_numEpisodes;

  // If we are looking for a special then we want to find all episodes - to exclude them
  if (m_isSpecial == IsSpecial::SPECIAL && m_numEpisodes > 0)
    m_allEpisodes = AllEpisodes::ALL;

  CLog::LogF(LOGDEBUG, "*** Episode Search Start ***");

  if (m_allEpisodes == AllEpisodes::SINGLE)
  {
    CLog::LogF(LOGDEBUG, "Looking for season {} episode {} duration {}",
               episodesOnDisc[episodeIndex].iSeason, episodesOnDisc[episodeIndex].iEpisode,
               episodesOnDisc[episodeIndex].duration);
  }
  else
    CLog::LogF(LOGDEBUG, "Looking for all episodes on disc");

  // List episodes expected on disc
  for (const auto& e : episodesOnDisc)
  {
    CLog::LogF(LOGDEBUG, "Expected on disc - season {} episode {} duration {}", e.iSeason,
               e.iEpisode, e.duration);
  }
}

namespace
{
void SortEpisodes(Episodes& episodes)
{
  std::ranges::sort(episodes, std::ranges::less{}, [](const Episode& e)
                    { return std::tie(e.iSeason, e.iEpisode, e.iSubepisode); });
}

bool IsPotentialPlayAllPlaylist(const PlaylistInformation& playlistInformation,
                                unsigned int numEpisodes)
{
  return playlistInformation.clips.size() >= numEpisodes &&
         playlistInformation.clips.size() <= numEpisodes + 2;
}

bool ClipQualifies(const ClipInfo& clipInformation,
                   unsigned int clip,
                   const PlaylistInformation& playlistInformation,
                   unsigned int& playAllPlaylistEpisodesStartOffset,
                   bool& allowBeginningOrEnd,
                   bool allowBeginningAndEnd,
                   std::chrono::milliseconds minEpisodeDuration)
{
  // If clip doesn't appear in another playlist (ie. clip should appear in both the play all playlist and the individual episode)
  // or clip is too short this is not a Play All playlist
  // BUT we allow first and/or last clip to be shorter or single (ie start intro/end credits)
  const bool isShort{clipInformation.duration < minEpisodeDuration};
  const bool inSinglePlaylist{clipInformation.playlists.size() == 1};
  if (!isShort && !inSinglePlaylist)
    return true;

  const bool canBeAtBeginningOrEnd{allowBeginningOrEnd || allowBeginningAndEnd};
  if (isShort && canBeAtBeginningOrEnd)
  {
    if (clip == playlistInformation.clips.front())
    {
      playAllPlaylistEpisodesStartOffset = 1;
      allowBeginningOrEnd =
          false; // If allowBeginningOrEnd true and short clip found at beginning, cannot have one at end
      return true;
    }

    if (clip == playlistInformation.clips.back())
      return true;
  }

  return false;
}

bool IsValidSingleEpisodePlaylist(const PlaylistInformation& singleEpisodePlaylistInformation,
                                  unsigned int clip,
                                  const ClipMap& clips,
                                  std::chrono::milliseconds minEpisodeDuration)
{
  // See if potential single episode playlist contains too many clips
  // If there are 3 clips then expect the middle clip to be the main episode clip
  // If there are numEpisodes clips this could be another play all playlist
  if (singleEpisodePlaylistInformation.clips.size() == 1)
    return singleEpisodePlaylistInformation.clips[0] == clip;

  if (singleEpisodePlaylistInformation.clips.size() == 2)
  {
    if (singleEpisodePlaylistInformation.clips[0] == clip)
      return clips.at(singleEpisodePlaylistInformation.clips[1]).duration <
             minEpisodeDuration; // Allow short clip at end
    if (singleEpisodePlaylistInformation.clips[1] == clip)
      return clips.at(singleEpisodePlaylistInformation.clips[0]).duration <
             minEpisodeDuration; // Allow short clip at start
    return false;
  }

  if (singleEpisodePlaylistInformation.clips.size() == 3)
  {
    if (singleEpisodePlaylistInformation.clips[1] != clip)
      return false; // Must be in the middle between short intro and ending clips
    return clips.at(singleEpisodePlaylistInformation.clips[0]).duration < minEpisodeDuration &&
           clips.at(singleEpisodePlaylistInformation.clips[2]).duration < minEpisodeDuration;
  }

  return false;
}

bool CheckClip(const PlaylistMap& playlists,
               const ClipMap& clips,
               unsigned int playlistNumber,
               const ClipInfo& clipInformation,
               unsigned int clip,
               std::chrono::milliseconds minEpisodeDuration,
               std::vector<unsigned int>& playAllPlaylistMap)
{
  for (const auto& singleEpisodePlaylist : clipInformation.playlists)
  {
    // Exclude potential play all playlist we are currently examining
    if (singleEpisodePlaylist == playlistNumber)
      continue;

    const auto& it{playlists.find(singleEpisodePlaylist)};
    if (it == playlists.end())
      return false;

    // Check the playlist could be a single episode
    if (!IsValidSingleEpisodePlaylist(it->second, clip, clips, minEpisodeDuration))
      return false;

    playAllPlaylistMap.emplace_back(singleEpisodePlaylist);
  }

  return true;
}

bool ProcessPlaylistClips(const ClipMap& clips,
                          const PlaylistMap& playlists,
                          unsigned int numEpisodes,
                          unsigned int playlistNumber,
                          unsigned int& playAllPlaylistEpisodesStartOffset,
                          std::chrono::milliseconds minEpisodeDuration,
                          const PlaylistInformation& playlistInformation,
                          std::map<unsigned int, std::vector<unsigned int>>& playAllPlaylistClipMap)
{
  bool allowBeginningOrEnd{playlistInformation.clips.size() == numEpisodes + 1};
  const bool allowBeginningAndEnd{playlistInformation.clips.size() == numEpisodes + 2};

  // Loop through each clip in potential play all playlist (numbering between numEpisodes and numEpisodes+2)
  for (unsigned int clip : playlistInformation.clips)
  {
    const auto& it{clips.find(clip)};
    if (it == clips.end())
      return false;

    // See if the clips qualify (ie. small clips (in addition to numEpisode clips) at start or end)
    const ClipInfo& clipInformation{it->second};
    if (!ClipQualifies(clipInformation, clip, playlistInformation,
                       playAllPlaylistEpisodesStartOffset, allowBeginningOrEnd,
                       allowBeginningAndEnd, minEpisodeDuration))
      return false;

    // A short first/last clip is an extra intro or ending clip, not an episode playlist
    // A short clip elsewhere would fail ClipQualifies
    // Record it with no episode playlists, so UsePlayAllPlaylistMethod skips over it
    if (clipInformation.duration < minEpisodeDuration)
    {
      playAllPlaylistClipMap[clip] = {};
      continue;
    }

    // See if the playlists associated with the clip are valid as a single episodes
    std::vector<unsigned int> playAllPlaylistMap;
    if (!CheckClip(playlists, clips, playlistNumber, clipInformation, clip, minEpisodeDuration,
                   playAllPlaylistMap))
      return false;

    playAllPlaylistClipMap[clip] = std::move(playAllPlaylistMap);
  }

  return true;
}

bool CheckDurationsWithinTolerance(std::chrono::milliseconds episodeDuration,
                                   std::chrono::milliseconds playlistDuration,
                                   int durationTolerancePercent = DURATION_TOLERANCE_PERCENT)
{
  const auto tolerance{(episodeDuration * durationTolerancePercent) / 100};
  return episodeDuration > 0ms && std::chrono::abs(playlistDuration - episodeDuration) <= tolerance;
}

// Whether the clips of a potential play all playlist look like the episodes on the disc.
//
// Having the right number of clips does not make a playlist the episodes - a disc's extras are
// commonly gathered into a play all playlist of their own (examples The Expanse S3D3 and The Last
// of Us S2D1 Bluray, where the featurettes are collected alongside the episodes). Two things
// tell them apart:
//   1) Episodes of one another are of near-equal duration
//   2) Each clip is of a length matching the episode it would be given
//
// The first alone is not enough, as a disc's extras can happen to be of similar lengths, and the
// second alone is not enough, as only one episode's duration is known on a disc's first search.
//
// The tolerance is loose because this only has to reject clips plainly unlike the episodes - a
// disc's episodes can legitimately vary in length, a feature length finale most of all, and a
// scraper commonly gives no more than the broadcast slot.
bool ArePlayAllPlaylistClipsEpisodes(const ClipMap& clips,
                                     const PlaylistInformation& playlistInformation,
                                     const Episodes& episodesOnDisc,
                                     unsigned int numSpecials,
                                     std::chrono::milliseconds minEpisodeDuration)
{
  // Any short clip is an intro or credits rather than an episode (see ProcessPlaylistClips())
  std::vector<std::chrono::milliseconds> durations;
  durations.reserve(playlistInformation.clips.size());
  for (const unsigned int clip : playlistInformation.clips)
  {
    if (const auto it{clips.find(clip)};
        it != clips.end() && it->second.duration >= minEpisodeDuration)
      durations.emplace_back(it->second.duration);
  }

  if (durations.empty())
    return false;

  // Of near-equal duration
  const std::chrono::milliseconds mean{std::accumulate(durations.begin(), durations.end(), 0ms) /
                                       static_cast<int>(durations.size())};
  if (!std::ranges::all_of(durations,
                           [mean](const std::chrono::milliseconds duration) {
                             return CheckDurationsWithinTolerance(
                                 mean, duration, DURATION_TOLERANCE_SCRAPED_PERCENT);
                           }))
    return false;

  // Of a length matching the episode each would be given
  for (size_t index = 0; const std::chrono::milliseconds duration : durations)
  {
    const size_t episode{numSpecials + index++};
    if (episode >= episodesOnDisc.size())
      break;

    const std::chrono::milliseconds scrapedDuration{episodesOnDisc[episode].duration * 1000ms};
    if (scrapedDuration > 0ms && !CheckDurationsWithinTolerance(scrapedDuration, duration,
                                                                DURATION_TOLERANCE_SCRAPED_PERCENT))
      return false;
  }

  return true;
}

bool AnyEpisodeDurationKnown(const Episodes& episodesOnDisc)
{
  return std::ranges::any_of(episodesOnDisc, [](const Episode& e) { return e.duration > 0; });
}

// Whether a playlist's duration is within tolerance of any of the episode durations known for the
// disc.
//
// Episode durations are filled in as a disc is scanned, so on any given search some of them may
// still be zero. Comparing against each known duration in turn (rather than against their average)
// keeps the outcome stable as the scan progresses whereas the average can fluctuate.
bool MatchesAnyEpisodeDuration(const Episodes& episodesOnDisc,
                               std::chrono::milliseconds playlistDuration)
{
  // CheckDurationsWithinTolerance() rejects a zero episode duration, so unknown episodes are
  // ignored here
  return std::ranges::any_of(
      episodesOnDisc, [playlistDuration](const Episode& e)
      { return CheckDurationsWithinTolerance(e.duration * 1000ms, playlistDuration); });
}

// Whether each playlist matches the duration of the episode in the same position, taking the
// playlists in ascending order as the episodes are.
//
// Only the episodes scraped so far have a duration, so the rest are not checked. That keeps the
// outcome the same however far through a disc the scan has got, where a tolerance window derived
// from the durations known so far moves as more of them arrive.
bool PlaylistsMatchEpisodeDurationsInOrder(const PlaylistMap& playlists,
                                           const Episodes& episodesOnDisc)
{
  if (playlists.size() != episodesOnDisc.size())
    return false;

  for (size_t index = 0; const PlaylistInformation& playlist : std::views::values(playlists))
  {
    const std::chrono::milliseconds episodeDuration{episodesOnDisc[index].duration * 1000ms};
    if (episodeDuration > 0ms && !CheckDurationsWithinTolerance(episodeDuration, playlist.duration))
    {
      CLog::LogF(LOGDEBUG, "Playlist {} duration {} does not match episode {} duration {}",
                 playlist.playlist, static_cast<int>(playlist.duration.count() / 1000),
                 episodesOnDisc[index].iEpisode, episodesOnDisc[index].duration);
      return false;
    }
    ++index;
  }

  return true;
}

std::chrono::milliseconds GetAverageEpisodeDuration(const Episodes& episodesOnDisc)
{
  auto nonZeroEpisodes{episodesOnDisc |
                       std::views::filter([](const Episode& e) { return e.duration > 0; })};
  if (nonZeroEpisodes.empty())
    return 0ms;

  const double mean{std::accumulate(nonZeroEpisodes.begin(), nonZeroEpisodes.end(), 0.0,
                                    [](double s, const Episode& e) { return s + e.duration; }) /
                    static_cast<double>(std::ranges::distance(nonZeroEpisodes))};

  const auto [sum, count]{std::accumulate(
      nonZeroEpisodes.begin(), nonZeroEpisodes.end(), std::pair{0.0, 0},
      [=](auto acc, const Episode& e)
      {
        return std::abs(e.duration - mean) <=
                       mean * static_cast<double>(DURATION_TOLERANCE_PERCENT) / 100.0
                   ? std::pair{acc.first + e.duration, acc.second + 1}
                   : acc;
      })};

  return std::chrono::milliseconds(count ? static_cast<long long>(sum / count) : 0) * 1000;
}

// Where an episode starts within a playlist, and how long it runs for
// Zero for both means the episode is the whole playlist
using EpisodeExtent = std::pair<std::chrono::milliseconds, std::chrono::milliseconds>;

// The extent of every episode of a playlist covering several of them, in episode order
using EpisodeExtents = std::vector<EpisodeExtent>;

// Where each episode of a playlist made up of one clip per episode starts, and how long it runs
// for. Returns an empty vector when the clips do not line up with the episodes on disc.
//
// The clips are taken to be in episode order and each is checked against the duration of the
// episode in its position. Only the episodes scraped so far have a duration, so the rest are
// checked only for being long enough to be an episode at all.
EpisodeExtents DivideClipsIntoEpisodes(const PlaylistInformation& information,
                                       const ClipMap& clips,
                                       const Episodes& episodesOnDisc,
                                       std::chrono::milliseconds minEpisodeDuration)
{
  if (information.clips.size() != episodesOnDisc.size())
    return {};

  EpisodeExtents episodes;
  episodes.reserve(information.clips.size());
  std::chrono::milliseconds start{0ms};
  for (size_t index = 0; const unsigned int clip : information.clips)
  {
    const auto clipIt{clips.find(clip)};
    if (clipIt == clips.end())
    {
      CLog::LogF(LOGERROR, "Clip {} missing in clip map", clip);
      return {};
    }

    const std::chrono::milliseconds clipDuration{clipIt->second.duration};
    if (clipDuration < minEpisodeDuration)
    {
      CLog::LogF(LOGDEBUG, "Rejecting playlist {} - clip {} duration {} is too short",
                 information.playlist, clip, static_cast<int>(clipDuration.count() / 1000));
      return {};
    }

    const std::chrono::milliseconds episodeDuration{episodesOnDisc[index].duration * 1000ms};
    if (episodeDuration > 0ms && !CheckDurationsWithinTolerance(episodeDuration, clipDuration))
    {
      CLog::LogF(LOGDEBUG,
                 "Rejecting playlist {} - clip {} duration {} does not match episode {} duration "
                 "{}",
                 information.playlist, clip, static_cast<int>(clipDuration.count() / 1000),
                 episodesOnDisc[index].iEpisode, episodesOnDisc[index].duration);
      return {};
    }

    episodes.emplace_back(start, clipDuration);
    start += clipDuration;
    ++index;
  }

  return episodes;
}

// Look for the number of chapters per episode that divides the playlist into runs of near-equal
// duration, each long enough to be an episode, with too little left over at the end to be another.
// Returns an empty vector when the chapters do not divide up that way.
//
// A chapter is where it starts within the playlist, so an episode of chaptersPerEpisode chapters
// runs from the start of its first chapter to the start of the chapter that begins the next episode
// - or to the end of the playlist for the last of them.
EpisodeExtents DivideChaptersIntoEpisodes(const std::vector<std::chrono::milliseconds>& chapters,
                                          std::chrono::milliseconds playlistDuration,
                                          unsigned int numEpisodes,
                                          std::chrono::milliseconds minEpisodeDuration)
{
  if (numEpisodes < 2 || chapters.size() < numEpisodes)
    return {};

  // Where the given chapter starts within the playlist
  // An index of one past the last chapter returns the end of the playlist
  const auto chapterStart{
      [&chapters, playlistDuration](size_t chapter) -> std::chrono::milliseconds
      { return chapter < chapters.size() ? chapters[chapter] : playlistDuration; }};

  const size_t maxChaptersPerEpisode{chapters.size() / numEpisodes};
  for (size_t chaptersPerEpisode = 1; chaptersPerEpisode <= maxChaptersPerEpisode;
       ++chaptersPerEpisode)
  {
    // Derive the start and duration of each episode from the chapters
    EpisodeExtents episodes;
    episodes.reserve(numEpisodes);
    for (unsigned int episode = 0; episode < numEpisodes; ++episode)
    {
      const std::chrono::milliseconds start{chapterStart(episode * chaptersPerEpisode)};
      const std::chrono::milliseconds end{chapterStart((episode + 1) * chaptersPerEpisode)};
      episodes.emplace_back(start, end - start);
    }

    // Whatever is left over must be too short to be an episode of its own, otherwise this is not
    // the number of chapters per episode
    if (playlistDuration - chapterStart(numEpisodes * chaptersPerEpisode) >= minEpisodeDuration)
      continue;

    // Every run must be long enough to be an episode
    const auto durations{episodes | std::views::values};
    if (std::ranges::any_of(durations, [minEpisodeDuration](const std::chrono::milliseconds d)
                            { return d < minEpisodeDuration; }))
      continue;

    // The runs must be of near-equal duration
    const std::chrono::milliseconds mean{std::accumulate(durations.begin(), durations.end(), 0ms) /
                                         numEpisodes};
    if (std::ranges::all_of(durations, [mean](const std::chrono::milliseconds d)
                            { return CheckDurationsWithinTolerance(mean, d); }))
      return episodes;
  }

  return {};
}

// Divides a playlist covering more than one episode into them by its chapters, for the methods that
// map several episodes onto a single playlist. Returns an empty vector when the playlist covers
// only one episode, or when its chapters give no boundaries between them.
EpisodeExtents DivideMultipleEpisodePlaylist(const PlaylistInformation& information,
                                             int multiple,
                                             std::chrono::milliseconds minEpisodeDuration)
{
  if (multiple < 2)
    return {};

  EpisodeExtents episodes{DivideChaptersIntoEpisodes(information.chapters, information.duration,
                                                     static_cast<unsigned int>(multiple),
                                                     minEpisodeDuration)};
  if (episodes.empty())
  {
    CLog::LogF(LOGDEBUG,
               "Playlist {} covers {} episodes but its {} chapters do not divide into that many "
               "of near-equal duration",
               information.playlist, multiple, information.chapters.size());
  }

  return episodes;
}

// The extent of the offset'th episode of a playlist that has been divided into episodes, or zero
// start and duration where it has not - taken as the episode being the whole playlist.
EpisodeExtent GetEpisodeExtent(const EpisodeExtents& episodes,
                               size_t offset,
                               unsigned int playlist,
                               int numEpisodesInPlaylist)
{
  if (offset >= episodes.size())
    return {0ms, 0ms};

  const auto& [start, duration]{episodes[offset]};
  CLog::LogF(LOGDEBUG, "Episode {} of {} in playlist {} - starts at {} runs for {}", offset + 1,
             numEpisodesInPlaylist, playlist, static_cast<int>(start.count() / 1000),
             static_cast<int>(duration.count() / 1000));

  return {start, duration};
}

// Whether candidateStream offers everything stream does.
//
// The two must have the same language, name and flags - and the candidate must then
// carry it at least as well - ie. no fewer channels and no 'poorer' a codec.
bool IsStreamCovered(const AudioStreamInfo& stream, const AudioStreamInfo& candidateStream)
{
  if (stream.language != candidateStream.language || stream.name != candidateStream.name ||
      stream.flags != candidateStream.flags)
    return false;

  // A channel count of zero means unknown, so only compare them when both are known
  if (stream.channels > 0 && candidateStream.channels > 0 &&
      stream.channels > candidateStream.channels)
    return false;

  return StreamUtils::GetCodecPriority(stream.codecName) <=
         StreamUtils::GetCodecPriority(candidateStream.codecName);
}

// Subtitle streams have no comparable ordering of quality, so they have to match
bool IsStreamCovered(const SubtitleStreamInfo& stream, const SubtitleStreamInfo& candidateStream)
{
  return stream == candidateStream;
}

// Whether every stream of subset has a distinct counterpart in superset that covers it. Streams are
// matched without regard to their position in the stream number table, as a playlist may list the
// same streams in a different order, to present a particular one first.
template<typename T>
bool AreStreamsContained(const std::vector<T>& subset, const std::vector<T>& superset)
{
  if (subset.size() > superset.size())
    return false;

  // A stream can only account for one of subset's streams, so that a playlist offering the same
  // stream twice is not contained in one offering it once
  std::vector<bool> matched(superset.size(), false);
  for (const T& stream : subset)
  {
    bool found{false};
    for (size_t i = 0; i < superset.size() && !found; ++i)
    {
      if (!matched[i] && IsStreamCovered(stream, superset[i]))
      {
        matched[i] = true;
        found = true;
      }
    }

    if (!found)
      return false;
  }

  return true;
}

// Whether playlist offers no audio or subtitle stream that candidate does not also offer, ie. it is
// the same content with a reduced set of streams.
// False when the streams of candidate are unknown, as nothing can then be concluded.
bool IsStreamSubset(const PlaylistInformation& playlist, const PlaylistInformation& candidate)
{
  if (candidate.audioStreams.empty() && candidate.pgStreams.empty())
    return false;

  return AreStreamsContained(playlist.audioStreams, candidate.audioStreams) &&
         AreStreamsContained(playlist.pgStreams, candidate.pgStreams);
}

template<std::ranges::input_range R>
bool ArePlaylistsConsecutive(const R& items)
{
  auto it = std::ranges::begin(items);
  auto end = std::ranges::end(items);

  if (it == end)
    return false; // empty

  auto prev = it->playlist;
  ++it;
  for (; it != end; ++it)
  {
    if (it->playlist != prev + 1)
      return false;
    prev = it->playlist;
  }

  return true;
}
} // namespace

void CDiscDirectoryHelper::StorePlayAllPlaylist(
    unsigned int playlistNumber,
    unsigned int playAllPlaylistEpisodesStartOffset,
    const PlaylistInformation& playlistInformation,
    const std::map<unsigned int, std::vector<unsigned int>>& playAllPlaylistClipMap)
{
  CLog::LogF(LOGDEBUG, "Potential play all playlist {}", playlistNumber);
  m_playAllPlaylists.insert(CandidatePlaylistInformation{
      .playlist = playlistNumber,
      .playAllPlaylistEpisodesStartOffset = playAllPlaylistEpisodesStartOffset,
      .duration = playlistInformation.duration,
      .chapters = static_cast<unsigned int>(playlistInformation.chapters.size()),
      .clips = playlistInformation.clips,
      .languages = playlistInformation.languages});
  m_playAllPlaylistsMap[playlistNumber] = playAllPlaylistClipMap;
}

void CDiscDirectoryHelper::FindPlayAllPlaylists(const ClipMap& clips,
                                                const PlaylistMap& playlists,
                                                const Episodes& episodesOnDisc)
{
  // Look for a potential play all playlist (gives episode order)
  //
  // Assumptions
  //   1) Playlist clip count = number of episodes on disc (+2 for potential separate intro/end credits)
  //   2) Each clip will be in at least one other playlist (the individual episode playlist)
  //   3) Each clip (bar the last) will be at least MIN_EPISODE_DURATION long
  //   4) Each potential individual episode playlist containing a clip from the potential play all playlist
  //      will have at most one other clip before/after
  //   5) The clips look like the episodes (see ArePlayAllPlaylistClipsEpisodes())

  // Only look for play all playlists if enough playlists and more than one episode on disc
  if (m_numEpisodes < 2 || playlists.size() < m_numEpisodes)
    return;

  for (const auto& [playlistNumber, playlistInformation] : playlists)
  {
    if (!IsPotentialPlayAllPlaylist(playlistInformation, m_numEpisodes))
      continue;

    if (!ArePlayAllPlaylistClipsEpisodes(clips, playlistInformation, episodesOnDisc, m_numSpecials,
                                         m_minEpisodeDuration))
    {
      CLog::LogF(LOGDEBUG,
                 "Rejecting potential play all playlist {} - its clips are the wrong length to be "
                 "the episodes",
                 playlistNumber);
      continue;
    }

    std::map<unsigned int, std::vector<unsigned int>> playAllPlaylistClipMap;
    unsigned int playAllPlaylistEpisodesStartOffset{0};
    if (ProcessPlaylistClips(clips, playlists, m_numEpisodes, playlistNumber,
                             playAllPlaylistEpisodesStartOffset, m_minEpisodeDuration,
                             playlistInformation, playAllPlaylistClipMap))
    {
      StorePlayAllPlaylist(playlistNumber, playAllPlaylistEpisodesStartOffset, playlistInformation,
                           playAllPlaylistClipMap);
    }
  }

  if (m_playAllPlaylists.empty())
    CLog::LogF(LOGDEBUG, "No play all playlists found");
}

void CDiscDirectoryHelper::FindGroups(const PlaylistMap& playlists, const Episodes& episodesOnDisc)
{
  // Look for groups of playlists - consecutively numbered playlists where the number of playlists
  //   is at least the number of episodes on disc
  // First generate map of all playlists >= minEpisodeDuration and not a play all playlist
  // Then generate array of all consecutive groups of playlists

  if (m_numEpisodes < 2)
  {
    CLog::LogF(LOGDEBUG, "No group search as single episode or specials only");
    return;
  }

  // Get all playlists(s) >= minEpisodeDuration and not a play all playlist
  PlaylistMap longPlaylists;
  std::ranges::copy_if(
      playlists, std::inserter(longPlaylists, longPlaylists.end()),
      [&](const PlaylistMapEntry& p)
      {
        const auto& [playlist, playlistInformation] = p;

        const auto playAllPlaylistNumbers{
            m_playAllPlaylists | std::views::transform(&CandidatePlaylistInformation::playlist)};

        return playlistInformation.duration >= m_minEpisodeDuration &&
               std::ranges::find(playAllPlaylistNumbers, playlist) == playAllPlaylistNumbers.end();
      });

  for (const auto& [playlist, playlistInformation] : longPlaylists)
  {
    CandidatePlaylistInformation groupPlaylist{
        .playlist = playlist,
        .duration = playlistInformation.duration,
        .chapters = static_cast<unsigned int>(playlistInformation.chapters.size()),
        .clips = playlistInformation.clips,
        .languages = playlistInformation.languages};
    if (!m_groups.empty() && m_groups.back().back().playlist == playlist - 1)
      m_groups.back().emplace_back(groupPlaylist);
    else
      m_groups.emplace_back(std::vector{groupPlaylist}); // New group
  }
  m_allGroups = m_groups;

  // Remove any groups that have less than m_numEpisodes playlists
  std::erase_if(m_groups, [this](const std::vector<CandidatePlaylistInformation>& group)
                { return group.size() < m_numEpisodes; });

  // See if there are exactly numEpisode playlists and no specials, in which case make a group.
  // Assumption has to be playlists match episodes in ascending order
  if (m_groups.empty() && m_numSpecials == 0)
  {
    CLog::LogF(LOGDEBUG, "Looking exact number of non-consecutive playlists");

    // Where there are more playlists than episodes, remove those whose durations are not within
    // 20% of any of the episodes, in the hope of leaving exactly numEpisodes of them
    if (longPlaylists.size() != m_numEpisodes && AnyEpisodeDurationKnown(episodesOnDisc))
    {
      std::erase_if(longPlaylists, [&episodesOnDisc](const PlaylistMapEntry& p)
                    { return !MatchesAnyEpisodeDuration(episodesOnDisc, p.second.duration); });
    }

    // Exactly numEpisodes playlists are the episodes, provided each matches the duration of the
    // episode in its position
    if (longPlaylists.size() == m_numEpisodes &&
        PlaylistsMatchEpisodeDurationsInOrder(longPlaylists, episodesOnDisc))
    {
      std::vector<CandidatePlaylistInformation> group;
      std::ranges::transform(longPlaylists, std::back_inserter(group),
                             [](const auto& PlaylistInformation) -> CandidatePlaylistInformation
                             {
                               const auto& [playlist, playlistInformation] = PlaylistInformation;
                               return {.playlist = playlist,
                                       .duration = playlistInformation.duration,
                                       .chapters = static_cast<unsigned int>(
                                           playlistInformation.chapters.size()),
                                       .clips = playlistInformation.clips,
                                       .languages = playlistInformation.languages};
                             });
      m_groups.emplace_back(std::move(group));
    }
  }

  // Make a group of the numEpisodes longest playlists (sorted by playlist)
  // This assumes the episodes occur in ascending playlist order
  // Exclude any playlists that might be play all playlists
  // Also exclude duplicate durations (added back later if needed)
  // Don't add to m_groups yet as it is only to be used if no other groups are found
  if (playlists.size() >= m_numEpisodes)
  {
    std::set<int64_t> seenDurations;
    std::vector<CandidatePlaylistInformation> candidates;
    candidates.reserve(playlists.size());
    std::ranges::copy_if(std::views::values(playlists) |
                             std::views::transform(
                                 [](const PlaylistInformation& p)
                                 {
                                   return CandidatePlaylistInformation{
                                       .playlist = p.playlist,
                                       .duration = p.duration,
                                       .chapters = static_cast<unsigned int>(p.chapters.size()),
                                       .clips = p.clips,
                                       .languages = p.languages};
                                 }) |
                             std::views::filter([this](const CandidatePlaylistInformation& cpi)
                                                { return !m_playAllPlaylists.contains(cpi); }),
                         std::back_inserter(candidates),
                         [&seenDurations](const CandidatePlaylistInformation& c) noexcept
                         { return seenDurations.insert(c.duration.count()).second; });

    m_nthLongestPlaylists.clear();
    m_nthLongestPlaylists.resize(m_numEpisodes);
    const auto sortResult = std::ranges::partial_sort_copy(
        candidates, m_nthLongestPlaylists,
        [](const CandidatePlaylistInformation& a, const CandidatePlaylistInformation& b)
        { return a.duration > b.duration; });

    // Fewer than m_numEpisodes playlists remain after excluding play-all/duplicate playlists
    if (sortResult.out != m_nthLongestPlaylists.end())
      m_nthLongestPlaylists.clear();
    else
      std::ranges::sort(m_nthLongestPlaylists, [&](const CandidatePlaylistInformation& a,
                                                   const CandidatePlaylistInformation& b)
                        { return a.playlist < b.playlist; });
  }

  if (m_groups.empty())
    CLog::LogF(LOGDEBUG, "No playlist groups found");
  else
    for (const auto& group : m_groups)
      CLog::LogF(LOGDEBUG, "Playlist group found from {} to {}", group.front().playlist,
                 group.back().playlist);
}

void CDiscDirectoryHelper::FindRelaxedPlayAllPlaylists(const PlaylistMap& playlists)
{
  // Look for a potential play all playlist (gives episode order)
  //
  // More relaxed assumptions than FindPlayAllPlaylists. Ignores clips.
  //   1) The playall playlist will be first and the episode playlists will be sequentially numbered
  //      after it as part of a group
  //   2) There will be at most n+1 playlists longer than the minimum episode duration
  //      (the playall playlist and the n episode playlists) in the group. There may be fewer if a
  //      playlist contains more than one episode (eg. a double episode), in which case each
  //      episode playlist must be a whole multiple of the shortest episode playlist and those
  //      multiples must account for all n episodes
  //   3) The sum of the durations of the episode playlists will be within 5% of the duration
  //      of the playall playlist

  // A group of two cannot be distinguished from a pair of episodes, so require at least the
  // playall playlist and two episode playlists
  static constexpr size_t MIN_RELAXED_PLAYALL_GROUP_SIZE{3};

  // Only look for play all playlists if enough playlists and more than one episode on disc and groups found
  if (m_numEpisodes < 2 || playlists.size() < MIN_RELAXED_PLAYALL_GROUP_SIZE || m_allGroups.empty())
    return;

  for (const auto& group : m_allGroups)
  {
    // Group is at most n+1 playlists
    if (group.size() < MIN_RELAXED_PLAYALL_GROUP_SIZE || group.size() > m_numEpisodes + 1)
      continue;

    // Group is consecutively numbered
    if (!ArePlaylistsConsecutive(group))
      continue;

    const unsigned int playAllPlaylist{group.front().playlist};
    const auto playAllPlaylistDuration{
        static_cast<int>(group.front().duration.count() / 1000)}; // For logging

    // Potential episode playlists duration's sum is within 5% of the playall playlist duration
    const std::chrono::milliseconds episodesDuration{std::accumulate(
        std::ranges::next(group.begin()), group.end(), std::chrono::milliseconds{0},
        [](auto acc, const CandidatePlaylistInformation& p) { return acc + p.duration; })};
    if (!CheckDurationsWithinTolerance(episodesDuration, group.front().duration,
                                       DURATION_TOLERANCE_RELAXED_PLAYALLPLAYLIST_PERCENT))
    {
      CLog::LogF(LOGDEBUG,
                 "Rejecting potential play all playlist {} duration {} - the {} following "
                 "playlist(s) in the group total {}",
                 playAllPlaylist, playAllPlaylistDuration, group.size() - 1,
                 static_cast<int>(episodesDuration.count() / 1000));
      continue;
    }

    // Each episode playlist must be a whole multiple of the shortest and the multiples must add
    // up to the number of episodes on disc. For a group of n+1 every multiple will be one.
    std::vector<CandidatePlaylistInformation> episodePlaylists(std::ranges::next(group.begin()),
                                                               group.end());
    if (!CalculateGroupMultiples(episodePlaylists, m_numEpisodes))
    {
      // CalculateGroupMultiples() fails either because a playlist is not a whole multiple of the
      // shortest episode playlist, or because the multiples do not add up to the number of
      // episodes on disc. The multiples are assigned either way, so report which it was.
      if (const auto& notMultiple{
              std::ranges::find(episodePlaylists, 0, &CandidatePlaylistInformation::multiple)};
          notMultiple != episodePlaylists.end())
      {
        CLog::LogF(
            LOGDEBUG,
            "Rejecting potential play all playlist {} duration {} - episode playlist {} "
            "duration {} is not a whole multiple of {} (the average shortest episode "
            "playlist in the group)",
            playAllPlaylist, playAllPlaylistDuration, notMultiple->playlist,
            static_cast<int>(notMultiple->duration.count() / 1000),
            static_cast<int>(CalculateAverageOfShortEpisodes(episodePlaylists).count() / 1000));
      }
      else
      {
        const auto multiples{episodePlaylists |
                             std::views::transform(&CandidatePlaylistInformation::multiple)};
        CLog::LogF(LOGDEBUG,
                   "Rejecting potential play all playlist {} duration {} - the {} following "
                   "playlist(s) in the group account for {} episode(s), not the {} on disc",
                   playAllPlaylist, playAllPlaylistDuration, episodePlaylists.size(),
                   std::accumulate(multiples.begin(), multiples.end(), 0), m_numEpisodes);
      }
      continue;
    }

    CLog::LogF(LOGDEBUG, "Potential play all playlist {} duration {} with {} episode playlist(s)",
               playAllPlaylist, playAllPlaylistDuration, episodePlaylists.size());
    for (const auto& episodePlaylist : episodePlaylists)
      CLog::LogF(LOGDEBUG, "Episode playlist {} duration {} contains {} episode(s)",
                 episodePlaylist.playlist,
                 static_cast<int>(episodePlaylist.duration.count() / 1000),
                 episodePlaylist.multiple);

    m_playAllPlaylistEpisodeMap[playAllPlaylist] = std::move(episodePlaylists);
  }

  if (m_playAllPlaylistEpisodeMap.empty())
    CLog::LogF(LOGDEBUG, "No play all playlists found using the relaxed method");
}

void CDiscDirectoryHelper::UsePlayAllPlaylistMethod(int episodeIndex, const PlaylistMap& playlists)
{
  if (m_playAllPlaylists.empty())
    return;

  CLog::LogF(LOGDEBUG, "Using Play All playlist method");

  // Get the playlist
  const auto& playlistInformation{*m_playAllPlaylists.begin()};
  const unsigned int playAllPlaylist{playlistInformation.playlist};
  CLog::LogF(LOGDEBUG, "Using candidate play all playlist {} duration {}", playAllPlaylist,
             static_cast<int>(playlistInformation.duration.count() / 1000));

  // Find the clip for the episode(s)
  const int episodeOffset{
      episodeIndex - static_cast<int>(m_numSpecials) + // Specials before episodes in episodesOnDisc
      static_cast<int>(
          playlistInformation
              .playAllPlaylistEpisodesStartOffset)}; // Adjust if a short clip at start of play-all playlist
  unsigned int i{0};
  for (const auto& clip : playlistInformation.clips)
  {
    if (m_allEpisodes == AllEpisodes::ALL || std::cmp_equal(i, episodeOffset))
    {
      const auto& it{m_playAllPlaylistsMap.find(playAllPlaylist)};
      if (it == m_playAllPlaylistsMap.end() || !it->second.contains(clip))
      {
        CLog::LogF(LOGERROR, "Clip {} missing in play all playlist map", clip);
        return;
      }

      CLog::LogF(LOGDEBUG, "Clip is {}", clip);

      // Find playlist(s) with that clip from map populated earlier
      const auto& singleEpisodePlaylists{it->second.find(clip)->second};

      for (const auto& singleEpisodePlaylist : singleEpisodePlaylists)
      {
        if (!playlists.contains(singleEpisodePlaylist))
        {
          CLog::LogF(LOGERROR, "Single episode playlist {} missing in playlist map",
                     singleEpisodePlaylist);
          return;
        }
        // Get playlist information
        const PlaylistInformation& singleEpisodePlaylistInformation{
            playlists.find(singleEpisodePlaylist)->second};

        CLog::LogF(LOGDEBUG, "Candidate playlist {} duration {}", singleEpisodePlaylist,
                   static_cast<int>(singleEpisodePlaylistInformation.duration.count() / 1000));

        m_candidatePlaylists.try_emplace(
            singleEpisodePlaylist,
            CandidatePlaylistInformation{
                .playlist = singleEpisodePlaylist,
                .index = i + m_numSpecials - playlistInformation.playAllPlaylistEpisodesStartOffset,
                .duration = singleEpisodePlaylistInformation.duration,
                .chapters =
                    static_cast<unsigned int>(singleEpisodePlaylistInformation.chapters.size()),
                .clips = singleEpisodePlaylistInformation.clips,
                .languages = singleEpisodePlaylistInformation.languages});
      }
    }
    ++i;
  }
}

void CDiscDirectoryHelper::UseRelaxedPlayAllPlaylistMethod(int episodeIndex,
                                                           const PlaylistMap& playlists)
{
  if (m_playAllPlaylistEpisodeMap.empty())
    return;

  CLog::LogF(LOGDEBUG, "Using Relaxed Play All playlist method");

  // Get the playlist
  const auto& [playAllPlaylist, episodes]{*m_playAllPlaylistEpisodeMap.begin()};

  CLog::LogF(LOGDEBUG, "Using candidate play all playlist {}", playAllPlaylist);

  // Start at numSpecials as specials (S00) are before episodes in episodesOnDisc
  unsigned int index{m_numSpecials};
  for (const auto& episodePlaylist : episodes)
  {
    const unsigned int singleEpisodePlaylist{episodePlaylist.playlist};
    if (!playlists.contains(singleEpisodePlaylist))
    {
      CLog::LogF(LOGERROR, "Single episode playlist {} missing in playlist map",
                 singleEpisodePlaylist);
      return;
    }
    // Get playlist information
    const PlaylistInformation& singleEpisodePlaylistInformation{
        playlists.find(singleEpisodePlaylist)->second};

    // Where a playlist covers more than one episode, see if its chapters give the boundaries
    // between them. Only for a single episode, as one covering them all is the whole playlist.
    const EpisodeExtents episodeExtents{
        m_allEpisodes == AllEpisodes::SINGLE
            ? DivideMultipleEpisodePlaylist(singleEpisodePlaylistInformation,
                                            episodePlaylist.multiple, m_minEpisodeDuration)
            : EpisodeExtents{}};

    // A playlist may cover more than one episode (a double/triple episode)
    for (int i = 0; i < episodePlaylist.multiple; ++i)
    {
      if (m_allEpisodes == AllEpisodes::ALL || std::cmp_equal(index, episodeIndex))
      {
        CLog::LogF(LOGDEBUG, "Candidate playlist {} duration {}", singleEpisodePlaylist,
                   static_cast<int>(singleEpisodePlaylistInformation.duration.count() / 1000));

        // The extent of this episode within the playlist, when the chapters gave it
        const auto [episodeStart, episodeDuration]{
            GetEpisodeExtent(episodeExtents, static_cast<size_t>(i), singleEpisodePlaylist,
                             episodePlaylist.multiple)};

        m_candidatePlaylists.try_emplace(
            singleEpisodePlaylist,
            CandidatePlaylistInformation{.playlist = singleEpisodePlaylist,
                                         .index = index,
                                         .duration = singleEpisodePlaylistInformation.duration,
                                         .multiple = episodePlaylist.multiple,
                                         .chapters = static_cast<unsigned int>(
                                             singleEpisodePlaylistInformation.chapters.size()),
                                         .clips = singleEpisodePlaylistInformation.clips,
                                         .languages = singleEpisodePlaylistInformation.languages,
                                         .episodeStart = episodeStart,
                                         .episodeDuration = episodeDuration});
      }
      ++index;
    }
  }
}

void CDiscDirectoryHelper::UseLongOrCommonMethodForSingleEpisode(int episodeIndex,
                                                                 const PlaylistMap& playlists)
{
  // Method 2b - Need to think about the special case of only one episode on disc

  // Sort playlists by length
  PlaylistVector playlists_length;
  playlists_length.reserve(playlists.size());
  playlists_length.assign(playlists.begin(), playlists.end());
  std::ranges::sort(playlists_length,
                    [](const PlaylistVectorEntry& i, const PlaylistVectorEntry& j)
                    {
                      const auto& [i_playlist, i_playlistInformation] = i;
                      const auto& [j_playlist, j_playlistInformation] = j;
                      if (i_playlistInformation.duration == j_playlistInformation.duration)
                        return i_playlist < j_playlist;
                      return i_playlistInformation.duration > j_playlistInformation.duration;
                    });

  // Remove duplicate lengths (each episode may have more than one playlist with different languages)
  const auto& [first, last] = std::ranges::unique(
      playlists_length, {}, [](const PlaylistVectorEntry& i) { return i.second.duration; });
  playlists_length.erase(first, last);

  // See how many unique (different length) episode length(>= minEpisodeDuration) playlists there are
  auto episodeLengthPlaylists{
      playlists_length | std::views::filter([this](const PlaylistVectorEntry& p)
                                            { return p.second.duration >= m_minEpisodeDuration; })};

  // Look for common starting playlists
  constexpr std::array<unsigned int, 5> commonStartingPlaylists = {801, 800, 1, 811, 0};
  auto playlistsView{episodeLengthPlaylists | std::views::keys};
  const auto commonPlaylist{std::ranges::find_first_of(playlistsView, commonStartingPlaylists)};

  if (std::ranges::distance(episodeLengthPlaylists) == 1)
  {
    // If only one long playlist, then assume it's that
    const auto& [playlist, playlistInformation] = *episodeLengthPlaylists.begin();
    CLog::LogF(LOGDEBUG, "Single Episode - found using single long playlist method");
    CLog::LogF(LOGDEBUG, "Candidate playlist {}", playlist);
    m_candidatePlaylists.try_emplace(
        playlist, CandidatePlaylistInformation{
                      .playlist = playlist,
                      .index = m_allEpisodes == AllEpisodes::ALL ? m_numSpecials : episodeIndex,
                      .duration = playlistInformation.duration,
                      .chapters = static_cast<unsigned int>(playlistInformation.chapters.size()),
                      .clips = playlistInformation.clips,
                      .languages = playlistInformation.languages});
  }
  else if (commonPlaylist != playlistsView.end())
  {
    // Found a common playlist, so assume it's that
    const auto& playlistInformation{playlists.at(*commonPlaylist)};
    CLog::LogF(LOGDEBUG, "Single Episode - found using common playlist method");
    CLog::LogF(LOGDEBUG, "Candidate playlist {}", *commonPlaylist);
    m_candidatePlaylists.try_emplace(
        *commonPlaylist,
        CandidatePlaylistInformation{
            .playlist = *commonPlaylist,
            .index = m_allEpisodes == AllEpisodes::ALL ? m_numSpecials : episodeIndex,
            .duration = playlistInformation.duration,
            .chapters = static_cast<unsigned int>(playlistInformation.chapters.size()),
            .clips = playlistInformation.clips,
            .languages = playlistInformation.languages});
  }
}

std::vector<std::vector<CDiscDirectoryHelper::CandidatePlaylistInformation>> CDiscDirectoryHelper::
    GetGroupsWithoutDuplicates(const std::vector<std::vector<CandidatePlaylistInformation>>& groups)
{
  std::vector<std::vector<CandidatePlaylistInformation>> uniqueGroups;
  for (const auto& playlistGroup : groups)
  {
    // A playlist is a duplicate of another in its group when it offers the same clips for the same
    // length
    std::set<std::pair<int64_t, std::vector<unsigned int>>> seenPlaylists;
    auto CandidatePlaylistInformationNotDuplicate{
        [&seenPlaylists](const CandidatePlaylistInformation& c)
        { return seenPlaylists.insert({c.duration.count(), c.clips}).second; }};

    std::ranges::copy_if(playlistGroup, std::back_inserter(uniqueGroups.emplace_back()),
                         CandidatePlaylistInformationNotDuplicate);
  }
  return uniqueGroups;
}

void CDiscDirectoryHelper::GetPlaylistsFromGroup(
    int episodeIndex, const std::vector<CandidatePlaylistInformation>& group)
{
  const int episodeOffset{episodeIndex - static_cast<int>(m_numSpecials)};
  for (unsigned int i = 0; i < m_numEpisodes; ++i)
  {
    if (m_allEpisodes != AllEpisodes::ALL &&
        std::cmp_not_equal(i, episodeOffset)) // Specials before episodes in episodesOnDisc
      continue;

    m_candidatePlaylists.try_emplace(group[i].playlist,
                                     CandidatePlaylistInformation{.playlist = group[i].playlist,
                                                                  .index = i + m_numSpecials,
                                                                  .duration = group[i].duration,
                                                                  .chapters = group[i].chapters,
                                                                  .clips = group[i].clips,
                                                                  .languages = group[i].languages});
    CLog::LogF(LOGDEBUG, "Candidate playlist {}", group[i].playlist);
  }
}

bool CDiscDirectoryHelper::CheckGroupDurations(
    const std::vector<CandidatePlaylistInformation>& group,
    const Episodes& episodesOnDisc,
    int durationTolerancePercent) const
{
  return std::ranges::all_of(std::views::iota(0u, m_numEpisodes),
                             [&](const unsigned i)
                             {
                               const auto& episode = episodesOnDisc[i + m_numSpecials];
                               return CheckDurationsWithinTolerance(episode.duration * 1000ms,
                                                                    group[i].duration,
                                                                    durationTolerancePercent);
                             });
}

bool CDiscDirectoryHelper::CheckGroupDurations(
    const std::vector<CandidatePlaylistInformation>& groupA,
    const std::vector<CandidatePlaylistInformation>& groupB,
    int durationTolerancePercent) const
{
  return std::ranges::all_of(std::views::iota(0u, m_numEpisodes),
                             [&](const unsigned i)
                             {
                               return CheckDurationsWithinTolerance(groupA[i].duration,
                                                                    groupB[i].duration,
                                                                    durationTolerancePercent);
                             });
}

// Whether each playlist of a group is of a length consistent with the episodes it would cover.
//
// A group here has fewer playlists than there are episodes, as a playlist may cover several of
// them, so the playlists cannot be compared with the episodes one for one as CheckGroupDurations()
// does. Divide each playlist's duration by the number of episodes it covers instead, and compare
// that with each of them. Only the check episodes scraped so far (no duration otherwise)
bool CDiscDirectoryHelper::CheckGroupMultipleDurations(
    const std::vector<CandidatePlaylistInformation>& group, const Episodes& episodesOnDisc) const
{
  size_t index{m_numSpecials}; // Specials (S00) are before episodes in episodesOnDisc
  for (const auto& playlist : group)
  {
    if (playlist.multiple < 1)
      return false;

    const std::chrono::milliseconds episodeFromPlaylistDuration{playlist.duration /
                                                                playlist.multiple};
    for (int i = 0; i < playlist.multiple; ++i, ++index)
    {
      if (index >= episodesOnDisc.size())
        return false;

      const std::chrono::milliseconds scrapedDuration{episodesOnDisc[index].duration * 1000ms};
      if (scrapedDuration > 0ms &&
          !CheckDurationsWithinTolerance(scrapedDuration, episodeFromPlaylistDuration,
                                         DURATION_TOLERANCE_SCRAPED_PERCENT))
      {
        CLog::LogF(LOGDEBUG,
                   "Rejecting group - playlist {} covers {} episode(s) at {} each, which does not "
                   "match episode {} duration {}",
                   playlist.playlist, playlist.multiple,
                   static_cast<int>(episodeFromPlaylistDuration.count() / 1000),
                   episodesOnDisc[index].iEpisode, episodesOnDisc[index].duration);
        return false;
      }
    }
  }

  return true;
}

// Decide if the group identified is likely to contain the episodes
// We might be in a position where we have a group of shorter playlists but the actual episode (longer)
// playlists are not in a group
bool CDiscDirectoryHelper::CheckGroup(const std::vector<CandidatePlaylistInformation>& group,
                                      const Episodes& episodesOnDisc) const
{
  // If we have episode durations in episodesOnDisc
  if (std::ranges::all_of(episodesOnDisc | std::views::drop(m_numSpecials),
                          [](const Episode& e) { return e.duration > 0; }))
  {
    // First check they are within tolerance of the durations in the group
    // This makes it likely the group represents the episodes
    if (CheckGroupDurations(group, episodesOnDisc))
      return true;

    // Now see if the durations of the longest playlists are within tolerance of the expected episodes on disc
    // If so, this probably isn't a valid group
    if (!m_nthLongestPlaylists.empty() &&
        CheckGroupDurations(m_nthLongestPlaylists, episodesOnDisc))
      return false;
  }
  else
  {
    // If we don't have all durations (often the case as they are loaded sequentially by the scraper)
    if (!m_nthLongestPlaylists.empty())
    {
      // Compare the longest playlist group to this group
      // Use a more relaxed tolerance (arbitrarily chosen)
      constexpr int RELAXED_DURATION_TOLERANCE_PERCENT = 40;
      if (!CheckGroupDurations(m_nthLongestPlaylists, group, RELAXED_DURATION_TOLERANCE_PERCENT))
        return false; // This probably isn't a valid group
    }
  }

  return true;
}

void CDiscDirectoryHelper::UseGroupMethod(int episodeIndex,
                                          const Episodes& episodesOnDisc,
                                          const PlaylistMap& playlists)
{
  // Method 2ai - More than one episode on disc

  // Use groups and find nth playlist (or all for all episodes) in group
  // Groups are already contain at least numEpisodes playlists of minimum duration
  // Firstly look just at groups that contain exactly numEpisodes playlists
  // Having removed duplicates
  CLog::LogF(LOGDEBUG, "Using group method - exact number of playlists");

  const std::vector groups{GetGroupsWithoutDuplicates(m_groups)};
  for (const auto& group : groups)
  {
    if (group.size() != m_numEpisodes)
      continue;

    if (!CheckGroup(group, episodesOnDisc))
      continue;

    GetPlaylistsFromGroup(episodeIndex, group);
  }

  if (m_candidatePlaylists.empty())
  {
    // Now look for groups that contain same/more than numEpisodes playlists (with duplicates)
    // Check that the first numEpisodes playlists have a duration within 20% of the desired episode
    // Exclude episodes with 0 duration as this will skew results
    CLog::LogF(LOGDEBUG, "Using group method - relaxed number of playlists");
    std::chrono::milliseconds episodeDuration;
    if (m_allEpisodes == AllEpisodes::ALL)
      episodeDuration = GetAverageEpisodeDuration(episodesOnDisc);
    else
      episodeDuration = episodesOnDisc[episodeIndex].duration * 1000ms;

    if (episodeDuration > 0ms)
    {
      for (const auto& group : m_groups)
      {
        if (group.size() < m_numEpisodes)
          continue;

        // Check duration
        if (!std::ranges::all_of(
                group | std::views::take(m_numEpisodes) |
                    std::views::transform(&CandidatePlaylistInformation::duration),
                [episodeDuration](const std::chrono::milliseconds playlistDuration)
                { return CheckDurationsWithinTolerance(episodeDuration, playlistDuration); }))
          continue;

        GetPlaylistsFromGroup(episodeIndex, group);
      }
    }
  }

  if (m_candidatePlaylists.empty() && !m_nthLongestPlaylists.empty())
  {
    // Now use the longest playlists
    // These should not be sequential (as they would have been handled above)
    if (std::ranges::adjacent_find(
            m_nthLongestPlaylists,
            [](const CandidatePlaylistInformation& a, const CandidatePlaylistInformation& b)
            { return b.playlist != a.playlist + 1; }) != m_nthLongestPlaylists.end())
    {
      // Now ensure there are no other playlists of similar length to the longest
      // Get the next longest playlist
      //
      // A playlist offering the same clips for the same length as one already chosen is another
      // copy of that episode rather than a rival for it, so it is left out - a disc commonly offers
      // each episode several times over, once per set of audio and subtitle streams, and those
      // copies must not be taken for other playlists of a similar length
      const auto isCopyOfChosenPlaylist{
          [this](const PlaylistInformation& p)
          {
            return std::ranges::any_of(m_nthLongestPlaylists,
                                       [&p](const CandidatePlaylistInformation& chosen)
                                       {
                                         return chosen.playlist != p.playlist &&
                                                chosen.duration == p.duration &&
                                                chosen.clips == p.clips;
                                       });
          }};

      std::vector<PlaylistInformation> tmp;
      tmp.reserve(playlists.size());
      std::ranges::copy(playlists | std::views::values |
                            std::views::filter(
                                [this, &isCopyOfChosenPlaylist](const PlaylistInformation& p) {
                                  return !m_playAllPlaylists.contains(p.playlist) &&
                                         !isCopyOfChosenPlaylist(p);
                                }),
                        std::back_inserter(tmp));

      // If there is no playlist beyond the m_nthLongestPlaylists ones, there is nothing
      // to compare against, so there cannot be a next playlist of similar length
      bool nextPlaylistIsClose{false};
      if (tmp.size() > m_nthLongestPlaylists.size())
      {
        std::ranges::nth_element(tmp,
                                 tmp.begin() + static_cast<unsigned>(m_nthLongestPlaylists.size()),
                                 [](const PlaylistInformation& a, const PlaylistInformation& b)
                                 { return a.duration > b.duration; });

        const auto lengthNext{tmp[m_nthLongestPlaylists.size()].duration};
        const auto length{std::ranges::min_element(m_nthLongestPlaylists, {},
                                                   &CandidatePlaylistInformation::duration)
                              ->duration};
        nextPlaylistIsClose = CheckDurationsWithinTolerance(length, lengthNext);
      }

      // If the next longest playlist is close to those in nthLongestPlaylists then
      // we cannot be certain the nthLongestPlaylists are the episodes
      if (!nextPlaylistIsClose)
        if (CheckGroup(m_nthLongestPlaylists, episodesOnDisc))
          GetPlaylistsFromGroup(episodeIndex, m_nthLongestPlaylists);
    }
  }

  if (m_candidatePlaylists.empty())
    CLog::LogF(LOGDEBUG, "No candidate playlists found");
}

namespace
{
// A playlist containing more than one episode is normally shorter than the individual episode
// playlists for those episodes added together, because the intro/recap/end credits that each
// single episode playlist carries appear only once. Every additional episode in the playlist
// omits another set, so the shortfall that can be tolerated grows with the number of episodes.
//
// In the other direction there is nothing to omit - a multiple episode playlist should not be
// materially longer than the individual episode playlists added together. So the upper bound is
// a fixed percentage that does not grow with the number of episodes.

// Both bounds are a percentage of the whole multiple
constexpr double MULTIPLE_UPPER_TOLERANCE_PERCENT{5.0};
constexpr double MULTIPLE_LOWER_TOLERANCE_PERCENT{15.0};

// The lower tolerance is increased by this factor for each episode in the playlist beyond the
// first (ie. 15%, 16.5%, 18.15%, ...), up to a maximum
constexpr double MULTIPLE_LOWER_TOLERANCE_GROWTH_FACTOR{1.1};
constexpr double MULTIPLE_LOWER_TOLERANCE_MAX_PERCENT{50.0};

// The range of multiples of a single episode's duration that a playlist containing the given
// number of episodes may occupy
constexpr std::pair<double, double> MultipleBounds(int episodes)
{
  double lowerTolerancePercent{MULTIPLE_LOWER_TOLERANCE_PERCENT};
  for (int episode = 1; episode < episodes; ++episode)
    lowerTolerancePercent *= MULTIPLE_LOWER_TOLERANCE_GROWTH_FACTOR;
  lowerTolerancePercent = std::min(lowerTolerancePercent, MULTIPLE_LOWER_TOLERANCE_MAX_PERCENT);

  const auto count{static_cast<double>(episodes)};
  return {count * (1.0 - lowerTolerancePercent / 100.0),
          count * (1.0 + MULTIPLE_UPPER_TOLERANCE_PERCENT / 100.0)};
}

// Returns the number of episodes that duration represents as a multiple of averageShortest, or 0
// if it does not fall within the tolerated range for any number of episodes.
// A playlist cannot hold more episodes than there are on the disc. The ranges are disjoint for
// the first few multiples but begin to overlap as the tolerated shortfall accumulates, so search
// ascending and take the fewest episodes that fit.
int CalculateMultiple(std::chrono::milliseconds duration,
                      std::chrono::milliseconds averageShortest,
                      unsigned int numEpisodes)
{
  if (averageShortest <= 0ms || duration <= 0ms)
    return 0;

  const double multiple{static_cast<double>(duration.count()) /
                        static_cast<double>(averageShortest.count())};

  for (int episodes = 1; std::cmp_less_equal(episodes, numEpisodes); ++episodes)
  {
    const auto [lower, upper]{MultipleBounds(episodes)};
    if (multiple >= lower && multiple <= upper)
      return episodes;
  }

  return 0;
}
} // namespace

std::chrono::milliseconds CDiscDirectoryHelper::CalculateAverageOfShortEpisodes(
    const std::vector<CandidatePlaylistInformation>& group)
{
  constexpr double SHORTEST_PERCENT{20.0};

  auto nonZero{group | std::views::filter([](const CandidatePlaylistInformation& p)
                                          { return p.duration > 0ms; })};
  if (nonZero.empty())
    return 0ms;

  // Find shortest playlist in group
  const std::chrono::milliseconds shortest = std::ranges::min(
      nonZero | std::views::transform(&CandidatePlaylistInformation::duration), {});

  // Then calculate the average of shortest (within 20% of the shortest) playlists
  const auto threshold = shortest * (1 + (SHORTEST_PERCENT / 100));
  std::vector<std::chrono::milliseconds> groupDurations;
  for (const auto& p : nonZero)
  {
    if (p.duration < threshold)
      groupDurations.push_back(p.duration);
  }

  if (groupDurations.empty())
    return 0ms;

  const std::chrono::milliseconds averageShortest{
      std::accumulate(groupDurations.begin(), groupDurations.end(), 0ms) /
      static_cast<long long>(groupDurations.size())};

  return averageShortest;
}

// Assign each playlist in the group the number of episodes it is likely to contain, being the
// whole multiple of the average shortest playlist in the group that its duration represents.
// Returns false unless every playlist is such a multiple and the multiples account for exactly
// numEpisodes episodes.
bool CDiscDirectoryHelper::CalculateGroupMultiples(std::vector<CandidatePlaylistInformation>& group,
                                                   unsigned int numEpisodes)
{
  const std::chrono::milliseconds averageShortest{CalculateAverageOfShortEpisodes(group)};

  for (auto& playlist : group)
    playlist.multiple = CalculateMultiple(playlist.duration, averageShortest, numEpisodes);

  // Check there are no playlists that are not a multiple
  if (std::ranges::any_of(group,
                          [](const CandidatePlaylistInformation& i) { return i.multiple == 0; }))
    return false;

  // Check that multiples add up to numEpisodes
  auto groupMultiples{group | std::views::transform(&CandidatePlaylistInformation::multiple)};
  return std::accumulate(groupMultiples.begin(), groupMultiples.end(), 0) ==
         static_cast<int>(numEpisodes);
}

void CDiscDirectoryHelper::UseGroupsWithMultiplesMethod(int episodeIndex,
                                                        const Episodes& episodesOnDisc,
                                                        const PlaylistMap& playlists)
{
  // No groups of numEpisodes length so see if there could be double episode playlists
  // Assume more than one playlist
  CLog::LogF(LOGDEBUG, "Using groups with multiples method");
  for (auto& group : GetGroupsWithoutDuplicates(m_allGroups))
  {
    if (!CalculateGroupMultiples(group, m_numEpisodes))
      continue;

    // The multiples adding up does not make the group the episodes - a run of extras can do that
    // too, so check the playlists are of a length to hold the episodes they would be given
    if (!CheckGroupMultipleDurations(group, episodesOnDisc))
      continue;

    // Save candidate episode(s)
    unsigned int index{
        m_numSpecials}; // Start at numSpecials as specials (S00) are before episodes in episodesOnDisc
    for (const auto& playlist : group)
    {
      auto playlistInformation{playlist};

      if (!playlists.contains(playlist.playlist))
      {
        CLog::LogF(LOGERROR, "Playlist {} missing in playlist map", playlist.playlist);
        return;
      }

      // Where a playlist covers more than one episode, see if its chapters give the boundaries
      // between them. Only for a single episode, as one covering them all is the whole playlist.
      const EpisodeExtents episodeExtents{
          m_allEpisodes == AllEpisodes::SINGLE
              ? DivideMultipleEpisodePlaylist(playlists.find(playlist.playlist)->second,
                                              playlist.multiple, m_minEpisodeDuration)
              : EpisodeExtents{}};

      for (int i = 0; i < playlist.multiple; ++i)
      {
        if (m_allEpisodes == AllEpisodes::ALL || std::cmp_equal(index, episodeIndex))
        {
          playlistInformation.index = index;
          std::tie(playlistInformation.episodeStart, playlistInformation.episodeDuration) =
              GetEpisodeExtent(episodeExtents, static_cast<size_t>(i), playlist.playlist,
                               playlist.multiple);
          m_candidatePlaylists.try_emplace(playlist.playlist, playlistInformation);
          CLog::LogF(LOGDEBUG, "Candidate playlist {} for episode {}", playlist.playlist,
                     episodesOnDisc[index].iEpisode);
        }
        ++index;
      }
    }
    break;
  }

  if (m_candidatePlaylists.empty())
    CLog::LogF(LOGDEBUG, "No candidate playlists found");
}

void CDiscDirectoryHelper::UseSingleEpisodeClipsPlaylistMethod(int episodeIndex,
                                                               const Episodes& episodesOnDisc,
                                                               const ClipMap& clips,
                                                               const PlaylistMap& playlists)
{
  // Method 3 - Some discs author the episodes as a single playlist made up of one clip per episode,
  //            with no individual episode playlists at all, so there is nothing for the play-all
  //            playlist or group methods to point at (example Tin Man S1D1 UK BD).
  //
  // Assumptions
  //   1) There is exactly one playlist at least the minimum episode duration long
  //   2) It has one clip per episode on disc, each at least the minimum episode duration long
  //   3) Where an episode's duration is known, it matches the duration of the clip in its position
  //   4) The clips are in episode order

  if (m_numSpecials > 0)
    return; // No way of telling which clip is the special

  // Find the sole playlist long enough to hold the episodes
  auto longPlaylists{playlists | std::views::values |
                     std::views::filter([this](const PlaylistInformation& p)
                                        { return p.duration >= m_minEpisodeDuration; })};
  const auto it{std::ranges::begin(longPlaylists)};
  const auto end{std::ranges::end(longPlaylists)};
  if (it == end || std::ranges::next(it) != end)
    return; // No playlist, or more than one, long enough to hold all the episodes

  const PlaylistInformation& information{*it};
  if (information.clips.size() != m_numEpisodes)
    return;

  CLog::LogF(LOGDEBUG, "Using single playlist with episode clips method");

  const EpisodeExtents episodeExtents{
      DivideClipsIntoEpisodes(information, clips, episodesOnDisc, m_minEpisodeDuration)};
  if (episodeExtents.empty())
    return;

  // Save the candidate episode. Only one entry can be stored, as m_candidatePlaylists is keyed on
  // playlist and every episode here shares the same playlist. When all episodes are wanted, the
  // playlist is returned once, covering them all.
  const unsigned int index{
      m_allEpisodes == AllEpisodes::SINGLE ? static_cast<unsigned int>(episodeIndex) : 0u};

  CLog::LogF(LOGDEBUG, "Candidate playlist {} for episode {}", information.playlist,
             episodesOnDisc[index].iEpisode);

  const auto [episodeStart,
              episodeDuration]{m_allEpisodes == AllEpisodes::SINGLE
                                   ? GetEpisodeExtent(episodeExtents, index, information.playlist,
                                                      static_cast<int>(m_numEpisodes))
                                   : EpisodeExtent{0ms, 0ms}};

  m_candidatePlaylists.try_emplace(
      information.playlist, CandidatePlaylistInformation{
                                .playlist = information.playlist,
                                .index = index,
                                .duration = information.duration,
                                .chapters = static_cast<unsigned int>(information.chapters.size()),
                                .clips = information.clips,
                                .languages = information.languages,
                                .episodeStart = episodeStart,
                                .episodeDuration = episodeDuration});
}

void CDiscDirectoryHelper::ChooseSingleBestPlaylist(const Episodes& episodesOnDisc)
{
  // Rebuild candidatePlaylists
  auto oldCandidatePlaylists{std::move(m_candidatePlaylists)};
  m_candidatePlaylists.clear();

  // Loop through each episode (in case of all episodes)
  // Generate set of indexes of episode entry in episodesOnDisc
  const auto indexView{oldCandidatePlaylists | std::views::values |
                       std::views::transform(&CandidatePlaylistInformation::index)};
  std::set<unsigned int> indexes(indexView.begin(), indexView.end());

  // Loop through each index (episode) and find the playlist with the closest duration
  for (unsigned int currentEpisodeIndex : indexes)
  {
    std::chrono::milliseconds duration{episodesOnDisc[currentEpisodeIndex].duration * 1000};

    // If episode length not known, ensure the longest playlist selected
    if (duration == 0ms)
      duration = std::chrono::milliseconds::max();

    auto filter{oldCandidatePlaylists |
                std::views::filter([currentEpisodeIndex](const auto& p)
                                   { return p.second.index == currentEpisodeIndex; }) |
                std::views::values};
    auto filteredCandidatePlaylists{std::vector(filter.begin(), filter.end())};

    for (auto& playlistInformation : filteredCandidatePlaylists)
      playlistInformation.durationDelta = abs(playlistInformation.duration - duration);

    // Sort descending based on number of chapters and ascending by relative difference in duration
    std::ranges::sort(
        filteredCandidatePlaylists,
        [](const CandidatePlaylistInformation& p, const CandidatePlaylistInformation& q)
        {
          if (p.chapters == q.chapters)
            return p.durationDelta < q.durationDelta;
          return p.chapters > q.chapters;
        });

    // Keep the playlist with most chapters and closest duration
    if (filteredCandidatePlaylists.empty())
    {
      CLog::LogF(LOGDEBUG, "No candidate playlists found for episode index {}",
                 currentEpisodeIndex);
      return;
    }
    const unsigned int playlist{filteredCandidatePlaylists[0].playlist};
    m_candidatePlaylists.try_emplace(playlist, filteredCandidatePlaylists[0]);

    CLog::LogF(LOGDEBUG,
               "Remaining candidate playlist (closest in duration) is {} for episode index {}",
               playlist, currentEpisodeIndex);
  }
}

void CDiscDirectoryHelper::AddIdenticalPlaylists(const PlaylistMap& playlists)
{
  // Collect the additions separately, so newly added playlists are not themselves
  // used as candidates to match against
  CandidatePlaylistsMap identicalPlaylists;

  for (const auto& [candidatePlaylist, candidatePlaylistInformation] : m_candidatePlaylists)
  {
    const auto candidate{playlists.find(candidatePlaylist)};

    // Find all other playlists of same duration with same clips
    for (const auto& [playlist, playlistInformation] : playlists)
    {
      if (playlist != candidatePlaylist &&
          candidatePlaylistInformation.duration == playlistInformation.duration &&
          candidatePlaylistInformation.languages != playlistInformation.languages &&
          candidatePlaylistInformation.clips == playlistInformation.clips)
      {
        // A playlist offering no streams the candidate does not already offer is a reduced
        // presentation of the same content rather than an alternative, so there is nothing to
        // choose between them
        if (candidate != playlists.end() && IsStreamSubset(playlistInformation, candidate->second))
        {
          CLog::LogF(LOGDEBUG,
                     "Ignoring playlist {} - its streams are a subset of those of playlist {}",
                     playlist, candidatePlaylist);
          continue;
        }

        CLog::LogF(LOGDEBUG, "Adding playlist {} as same duration and clips as playlist {}",
                   playlist, candidatePlaylist);

        // Copy the candidate but with this playlist's own number and languages
        CandidatePlaylistInformation identicalPlaylistInformation{candidatePlaylistInformation};
        identicalPlaylistInformation.playlist = playlist;
        identicalPlaylistInformation.chapters =
            static_cast<unsigned int>(playlistInformation.chapters.size());
        identicalPlaylistInformation.languages = playlistInformation.languages;
        identicalPlaylists.try_emplace(playlist, identicalPlaylistInformation);
      }
    }
  }

  m_candidatePlaylists.merge(identicalPlaylists);
}

void CDiscDirectoryHelper::FindCandidatePlaylists(const Episodes& episodesOnDisc,
                                                  int episodeIndex,
                                                  const ClipMap& clips,
                                                  const PlaylistMap& playlists)
{
  // At this stage we have a number of ways of trying to determine the correct playlist for an episode
  //
  // 1) Using a 'Play All' playlist, if present.
  //
  // 2ai) Using the longest playlists that are consecutive, using the playlist in the nth position.
  //      Taking into account that there may be multiple episodes in a single playlist.
  //
  // 2aii) If there are no consecutive playlist groups but the number of possible playlists equals the number of episodes
  //       then consider that a group even if not consecutive.
  //
  // 2b) For single episode discs, look for the longest playlist.
  //     There are some discs where there are extras that are longer than the episode itself, in this case
  //     we look for the playlist with a common starting number (eg. 1, 800 etc..)
  //
  // 3) Using a single playlist that holds every episode as a clip, where the disc has no individual
  //    episode playlists at all.

  if (m_allEpisodes == AllEpisodes::ALL)
    CLog::LogF(LOGDEBUG, "Looking for all episodes on disc");

  m_candidatePlaylists.clear();
  m_candidateSpecials.clear();

  if (m_playAllPlaylists.size() == 1)
    UsePlayAllPlaylistMethod(episodeIndex, playlists);
  else if (m_playAllPlaylistEpisodeMap.size() == 1)
    UseRelaxedPlayAllPlaylistMethod(episodeIndex, playlists);
  else if (m_numEpisodes == 1)
    UseLongOrCommonMethodForSingleEpisode(episodeIndex, playlists);
  else if (!m_groups.empty())
    UseGroupMethod(episodeIndex, episodesOnDisc, playlists);

  if (m_candidatePlaylists.empty() && !m_allGroups.empty() && m_numEpisodes > 1)
    UseGroupsWithMultiplesMethod(episodeIndex, episodesOnDisc, playlists);

  if (m_candidatePlaylists.empty() && m_numEpisodes > 1 && m_isSpecial == IsSpecial::EPISODE)
    UseSingleEpisodeClipsPlaylistMethod(episodeIndex, episodesOnDisc, clips, playlists);

  // Now deal with the possibility there may be more than one playlist (per episode)
  // For this, see which is closest in duration to the desired episode duration (from the scraper)
  // If we are looking for a special then leave all potential episode playlists in array
  if (m_candidatePlaylists.size() > 1 && m_isSpecial == IsSpecial::EPISODE)
    ChooseSingleBestPlaylist(episodesOnDisc);

  // candidatePlaylists should now (ideally) contain one candidate title for the episode or all episodes
  // Now look at durations of found playlist and add identical (in case language options)
  if (!m_candidatePlaylists.empty())
    AddIdenticalPlaylists(playlists);
}

void CDiscDirectoryHelper::FindSpecials(const PlaylistMap& playlists)
{
  if (m_numSpecials == 0)
    return;

  // Specials
  //
  // These are more difficult - as there may only be one per disc and we can't make assumptions about playlists.
  // So have to look on basis of duration alone.

  // Remove episodes and short playlists
  PlaylistVector playlistsLength;
  playlistsLength.reserve(playlists.size());
  playlistsLength.assign(playlists.begin(), playlists.end());
  if (m_numEpisodes > 0)
  {
    std::erase_if(
        playlistsLength,
        [this](const PlaylistVectorEntry& playlist)
        {
          const auto& [playlistNumber, playlistInformation] = playlist;

          const bool isShort{playlistInformation.duration < MIN_SPECIAL_DURATION};

          const auto candidatePlaylistNumbers{m_candidatePlaylists | std::views::keys};
          const bool isEpisode{std::ranges::find(candidatePlaylistNumbers, playlistNumber) !=
                               candidatePlaylistNumbers.end()};

          const auto playAllPlaylistNumbers{
              m_playAllPlaylists | std::views::transform(&CandidatePlaylistInformation::playlist)};
          const bool isPlayAll{std::ranges::find(playAllPlaylistNumbers, playlistNumber) !=
                               playAllPlaylistNumbers.end()};

          return isShort || isEpisode || isPlayAll;
        });
  }

  // Sort playlists by length
  std::ranges::sort(playlistsLength,
                    [](const PlaylistVectorEntry& i, const PlaylistVectorEntry& j)
                    {
                      const auto& [i_playlist, i_playlistInformation] = i;
                      const auto& [j_playlist, j_playlistInformation] = j;
                      if (i_playlistInformation.duration == j_playlistInformation.duration)
                        return i_playlist < j_playlist;
                      return i_playlistInformation.duration > j_playlistInformation.duration;
                    });

  // Take the remaining playlists as specials
  // If more than one candidate, we don't know which the special is, so include them all
  if (playlistsLength.size() >= m_numSpecials)
  {
    for (unsigned int playlist : playlistsLength | std::views::keys)
      m_candidateSpecials.emplace(playlist);
  }
}

namespace
{
std::shared_ptr<CFileItem> GenerateEpisodeItem(const CURL& url,
                                               unsigned int playlist,
                                               const PlaylistInformation& information,
                                               const Episode& episode,
                                               bool isSpecial,
                                               std::chrono::milliseconds episodeStart = 0ms,
                                               std::chrono::milliseconds episodeDuration = 0ms)
{
  CURL path{url};
  std::string buf{StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", playlist)};
  path.SetFileName(buf);
  const auto item{std::make_shared<CFileItem>(path.Get(), false)};

  // Get clips
  const std::chrono::milliseconds duration{episodeDuration > 0ms ? episodeDuration
                                                                 : information.duration};

  // Get languages
  const std::string langs{information.languages};

  CVideoInfoTag* itemTag{item->GetVideoInfoTag()};
  itemTag->SetDuration(static_cast<int>(duration.count() / 1000));

  // Write an episode bookmark
  if (episodeStart > 0ms)
  {
    itemTag->m_EpBookmark.timeInSeconds = static_cast<double>(episodeStart.count()) / 1000.0;
    itemTag->m_EpBookmark.totalTimeInSeconds =
        static_cast<double>(information.duration.count()) / 1000.0;
  }

  item->SetProperty("bluray_playlist", playlist);

  // Get episode title
  const std::string& title{episode.strTitle};
  if (isSpecial)
  {
    if (title.empty())
      buf = CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21350); /* Special */
    else
      /* Special xx - title */
      buf = StringUtils::Format(
          CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21348), episode.iEpisode,
          episode.strTitle);
  }
  else
    /* Episode xx - title */
    buf =
        StringUtils::Format(CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(21349),
                            episode.iEpisode, episode.strTitle);
  item->SetTitle(buf);
  item->SetLabel(buf);

  item->SetLabel2(StringUtils::Format(
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25005) /* Title: {0:d} */ +
          " - {1:s}: {2:s}\r\n{3:s}: {4:s}",
      playlist,
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(180) /* Duration */,
      StringUtils::SecondsToTimeString(static_cast<int>(duration.count() / 1000)),
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(24026) /* Languages */,
      langs));
  item->SetSize(0);
  item->SetArt("icon", "DefaultVideo.png");

  return item;
}

// Stream details are deferred during title determination, as deriving them for every title on a
// disc is expensive.
void AddStreamDetails(const StreamDetailsProvider& getStreamDetails,
                      const CFileItemList& allTitles,
                      unsigned int playlist,
                      CFileItem& item,
                      std::chrono::milliseconds episodeDuration = 0ms)
{
  if (!getStreamDetails)
    return;

  if (!allTitles.Contains(item.GetPath()))
  {
    CLog::LogF(LOGDEBUG, "Playlist {} not found in disc titles", playlist);
    return;
  }

  getStreamDetails(playlist, item);

  if (episodeDuration > 0ms)
  {
    item.GetVideoInfoTag()->m_streamDetails.SetVideoDuration(
        0, static_cast<int>(episodeDuration.count() / 1000));
  }
}
} // namespace

void CDiscDirectoryHelper::EndEpisodePlaylistSearch()
{
  CLog::LogF(LOGDEBUG, "*** Episode Search End ***");
}

void CDiscDirectoryHelper::PopulateEpisodeFileItems(const CURL& url,
                                                    CFileItemList& items,
                                                    const CFileItemList& allTitles,
                                                    int episodeIndex,
                                                    const Episodes& episodesOnDisc,
                                                    const PlaylistMap& playlists) const
{
  // Now populate CFileItemList to return
  //
  // For singe episodes - relevant playlists are in candidatePlaylists. Show then all
  // For all episodes   - relevant playlists are in candidatePlaylists and specials in candidateSpecials
  //                    - show all episodes and specials (with latter entitled 'Special')
  // For single special - relevant playlists are in candidateSpecials.
  //                    - if there is a single playlist then label that with special's title
  //                      otherwise label all playlists as 'Special' as cannot determine which it is

  if (m_isSpecial == IsSpecial::EPISODE)
  {
    // Sort by index, number of languages and playlist
    auto filter = m_candidatePlaylists | std::views::values;
    auto sortedPlaylists = std::vector(filter.begin(), filter.end());

    std::ranges::sort(
        sortedPlaylists,
        [](const CandidatePlaylistInformation& i, const CandidatePlaylistInformation& j)
        {
          if (i.index == j.index)
          {
            if (i.languages.size() == j.languages.size())
              return i.playlist < j.playlist;
            return i.languages.size() > j.languages.size();
          }
          return i.index < j.index;
        });

    for (const auto& playlist : sortedPlaylists)
    {
      if (!playlists.contains(playlist.playlist))
      {
        CLog::LogF(LOGERROR, "Playlist {} missing in playlist map", playlist.playlist);
        continue;
      }
      if (playlist.index >= episodesOnDisc.size())
      {
        CLog::LogF(LOGERROR, "Playlist {} index out of range ({}) in episodesOnDisc ({})",
                   playlist.playlist, playlist.index, episodesOnDisc.size());
        continue;
      }
      const auto& information{playlists.find(playlist.playlist)->second};
      const auto newItem{GenerateEpisodeItem(url, playlist.playlist, information,
                                             episodesOnDisc[playlist.index], false, // Episode
                                             playlist.episodeStart, playlist.episodeDuration)};
      if (!newItem)
      {
        CLog::LogF(LOGDEBUG, "Failed to generate FileItem for playlist {}", playlist.playlist);
        continue;
      }

      AddStreamDetails(m_getStreamDetails, allTitles, playlist.playlist, *newItem,
                       playlist.episodeDuration);

      items.Add(newItem);
    }
  }

  if (m_isSpecial == IsSpecial::SPECIAL)
  {
    for (const auto& playlist : m_candidateSpecials)
    {
      if (!playlists.contains(playlist))
      {
        CLog::LogF(LOGERROR, "Playlist {} missing in playlist map", playlist);
        continue;
      }
      const auto& information{playlists.find(playlist)->second};

      Episode episode;
      if (m_isSpecial == IsSpecial::SPECIAL && m_candidateSpecials.size() == 1)
        episode = episodesOnDisc[episodeIndex];

      const auto newItem{GenerateEpisodeItem(url, playlist, information, episode, true)}; // Special
      if (!newItem)
      {
        CLog::LogF(LOGDEBUG, "Failed to generate FileItem for playlist {}", playlist);
        continue;
      }

      AddStreamDetails(m_getStreamDetails, allTitles, playlist, *newItem);

      items.Add(newItem);
    }
  }

  LogEpisodePlaylistSearchResult(items, episodeIndex, episodesOnDisc);
}

void CDiscDirectoryHelper::LogEpisodePlaylistSearchResult(const CFileItemList& items,
                                                          int episodeIndex,
                                                          const Episodes& episodesOnDisc) const
{
  std::vector<std::string> foundPlaylists;
  foundPlaylists.reserve(items.Size());
  for (const auto& item : items)
    foundPlaylists.emplace_back(item->GetProperty("bluray_playlist").asString());
  const std::string playlistList{foundPlaylists.empty() ? "none"
                                                        : StringUtils::Join(foundPlaylists, ",")};

  if (m_allEpisodes == AllEpisodes::ALL)
  {
    CLog::LogF(LOGDEBUG, "Episode search for all episodes on disc found playlist(s) {}",
               playlistList);
    return;
  }

  const Episode& episode{episodesOnDisc[episodeIndex]};
  CLog::LogF(LOGDEBUG, "Episode search for season {} episode {} found playlist(s) {}",
             episode.iSeason, episode.iEpisode, playlistList);
}

bool CDiscDirectoryHelper::GetEpisodePlaylists(
    const CURL& url,
    CFileItemList& items,
    const CFileItemList& allTitles, // FileItem for each playlist on the disc (no stream details)
    int episodeIndex,
    const Episodes& episodesOnDiscUnsorted,
    const ClipMap& clips,
    const PlaylistMap& playlists)
{
  items.Clear();
  Reset();

  // Checks
  if (playlists.empty() || clips.empty() || episodeIndex < -1 ||
      std::cmp_greater_equal(episodeIndex, episodesOnDiscUnsorted.size()))
    return false;

  // Sort (subsequent routines assume that specials (season 0) are before episodes)
  auto episodesOnDisc{episodesOnDiscUnsorted};
  SortEpisodes(episodesOnDisc);
  if (episodeIndex >= 0)
  {
    // Adjust index
    const auto& wantedEpisode{episodesOnDiscUnsorted[episodeIndex]};
    const auto it{std::ranges::find(episodesOnDisc, wantedEpisode)};
    episodeIndex = static_cast<int>(std::ranges::distance(episodesOnDisc.begin(), it));
  }

  InitialiseEpisodePlaylistSearch(episodeIndex, episodesOnDisc);
  FindPlayAllPlaylists(clips, playlists, episodesOnDisc);
  FindGroups(playlists, episodesOnDisc);
  FindRelaxedPlayAllPlaylists(playlists); // Uses m_allGroups
  FindCandidatePlaylists(episodesOnDisc, episodeIndex, clips, playlists);
  FindSpecials(playlists);
  EndEpisodePlaylistSearch();
  PopulateEpisodeFileItems(url, items, allTitles, episodeIndex, episodesOnDisc, playlists);

  return !items.IsEmpty();
}

namespace
{
void InitialiseAllEpisodesPlaylistSearch(std::vector<PlaylistInformation>& playlists,
                                         const PlaylistMap& playlistMap)
{
  playlists.reserve(playlistMap.size());
  std::ranges::transform(playlistMap, std::back_inserter(playlists),
                         [](const PlaylistMapEntry& pair) { return pair.second; });
}

std::shared_ptr<CFileItem> GenerateAllEpisodesItem(const CURL& url,
                                                   unsigned int playlist,
                                                   const PlaylistInformation& information)
{
  CURL path{url};
  path.SetFileName(StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", playlist));
  const auto item{std::make_shared<CFileItem>(path.Get(), false)};

  // Get clips
  const std::chrono::milliseconds duration{information.duration};

  // Get languages
  const std::string langs{information.languages};

  CVideoInfoTag* itemTag{item->GetVideoInfoTag()};
  itemTag->SetDuration(static_cast<int>(duration.count() / 1000));
  item->SetProperty("bluray_playlist", playlist);

  // Get title
  const std::string title{StringUtils::Format(
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25005), playlist)};
  item->SetTitle(title);
  item->SetLabel(title);

  item->SetLabel2(StringUtils::Format(
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25005) /* Title: {0:d} */ +
          " - {1:s}: {2:s}\r\n{3:s}: {4:s}",
      playlist,
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(180) /* Duration */,
      StringUtils::SecondsToTimeString(static_cast<int>(duration.count() / 1000)),
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(24026) /* Languages */,
      langs));
  item->SetSize(0);
  item->SetArt("icon", "DefaultVideo.png");

  return item;
}

void PopulateAllEpisodesFileItems(const CURL& url,
                                  CFileItemList& items,
                                  const CFileItemList& allTitles,
                                  const std::vector<PlaylistInformation>& playlists,
                                  const PlaylistMap& playlistMap,
                                  const StreamDetailsProvider& getStreamDetails)
{
  // Sort by playlist
  auto sortedPlaylists = playlists;
  std::ranges::sort(sortedPlaylists, {}, &PlaylistInformation::playlist);

  for (const auto& playlist : sortedPlaylists)
  {
    if (!playlistMap.contains(playlist.playlist))
    {
      CLog::LogF(LOGERROR, "Playlist {} missing in playlist map", playlist.playlist);
      continue;
    }
    const auto& information{playlistMap.find(playlist.playlist)->second};
    const auto newItem{GenerateAllEpisodesItem(url, playlist.playlist, information)};
    if (!newItem)
    {
      CLog::LogF(LOGDEBUG, "Failed to generate FileItem for playlist {}", playlist.playlist);
      continue;
    }

    AddStreamDetails(getStreamDetails, allTitles, playlist.playlist, *newItem);

    items.Add(newItem);
  }
}
} // namespace

bool CDiscDirectoryHelper::FilterAllEpisodesPlaylists(std::vector<PlaylistInformation>& playlists,
                                                      GetTitle job)
{
  if (job != GetTitle::ALL)
  {
    // Remove all clips less than minEpisodeDuration or play all playlist
    std::erase_if(playlists,
                  [this](const PlaylistInformation& playlist)
                  {
                    return playlist.duration < m_minEpisodeDuration ||
                           m_playAllPlaylists.contains(playlist.playlist);
                  });
  }
  return !playlists.empty();
}

bool CDiscDirectoryHelper::GetAllEpisodePlaylists(
    const CURL& url,
    CFileItemList& items,
    const CFileItemList& allTitles, // FileItem for each playlist on the disc (no stream details)
    GetTitle job,
    const Episodes& episodesOnDiscUnsorted,
    const ClipMap& clips,
    const PlaylistMap& playlistMap)
{
  items.Clear();
  Reset();

  // Checks
  if (playlistMap.empty() || clips.empty())
    return false;

  // Sort (subsequent routines assume that specials (season 0) are before episodes)
  if (!episodesOnDiscUnsorted.empty())
  {
    auto episodesOnDisc{episodesOnDiscUnsorted};
    SortEpisodes(episodesOnDisc);

    InitialiseEpisodePlaylistSearch(ALL_PLAYLISTS, episodesOnDisc);
    FindPlayAllPlaylists(clips, playlistMap, episodesOnDisc);
  }

  std::vector<PlaylistInformation> playlists;
  InitialiseAllEpisodesPlaylistSearch(playlists, playlistMap);
  if (!FilterAllEpisodesPlaylists(playlists, job))
    return false;
  EndEpisodePlaylistSearch();
  PopulateAllEpisodesFileItems(url, items, allTitles, playlists, playlistMap, m_getStreamDetails);

  return !items.IsEmpty();
}

namespace
{
std::string_view GetTitlesJobDescription(GetTitle job)
{
  switch (job)
  {
    using enum GetTitle;
    case SINGLE:
      return "single playlist";
    case MAIN:
      return "main playlist(s)";
    case EPISODES:
      return "episode playlists";
    case ALL:
      return "all playlists";
  }
  return "unknown";
}

void LogMoviePlaylist(std::string_view prefix, const PlaylistInformation& playlist)
{
  CLog::LogF(
      LOGDEBUG, "{} playlist {}, duration {}, chapters {}, clips {}, langs {}, subs {}", prefix,
      playlist.playlist,
      StringUtils::SecondsToTimeString(static_cast<int>(
          std::chrono::duration_cast<std::chrono::seconds>(playlist.duration).count())),
      playlist.chapters.size(), playlist.clips.size(), playlist.languages,
      fmt::join(playlist.pgStreams | std::views::transform([](const auto& stream)
                                                           { return stream.language.AsBcp47(); }),
                ","));
}

void LogMoviePlaylists(std::string_view prefix, const std::vector<PlaylistInformation>& playlists)
{
  if (playlists.empty())
  {
    CLog::LogF(LOGDEBUG, "{} none", prefix);
    return;
  }
  for (const auto& playlist : playlists)
    LogMoviePlaylist(prefix, playlist);
}

/*!
 * \brief Removes the playlists matching shouldRemove, logging each removal and the reason for it.
 */
template<typename Predicate>
void RemovePlaylists(std::vector<PlaylistInformation>& playlists,
                     std::string_view reason,
                     Predicate shouldRemove)
{
  for (const auto& playlist : playlists | std::views::filter(shouldRemove))
    LogMoviePlaylist(StringUtils::Format("Rejected ({}) -", reason), playlist);

  std::erase_if(playlists, shouldRemove);
}

void InitialiseMoviePlaylistSearch(std::vector<PlaylistInformation>& playlists,
                                   const PlaylistMap& playlistMap,
                                   GetTitle job,
                                   int mainPlaylist)
{
  playlists.reserve(playlistMap.size());
  std::ranges::transform(playlistMap | std::views::values, std::back_inserter(playlists),
                         [](const PlaylistInformation& information) { return information; });

  CLog::LogF(LOGDEBUG, "*** Movie Search Start ***");
  CLog::LogF(LOGDEBUG, "Looking for {} - main playlist {}", GetTitlesJobDescription(job),
             mainPlaylist >= 0 ? std::to_string(mainPlaylist) : "unknown");
  LogMoviePlaylists("Candidate -", playlists);
}

//! \brief The sorted durations of a playlist's clips, or none at all if any of them is unknown.
std::vector<std::chrono::milliseconds> GetSortedClipDurations(const PlaylistInformation& playlist,
                                                              const ClipMap& clips)
{
  std::vector<std::chrono::milliseconds> durations;
  durations.reserve(playlist.clips.size());
  for (const unsigned int clip : playlist.clips)
  {
    const auto it{clips.find(clip)};
    if (it == clips.end() || it->second.duration <= 0ms)
      return {};
    durations.emplace_back(it->second.duration);
  }
  std::ranges::sort(durations);
  return durations;
}

/*!
 * \brief Whether two playlists present the same content, whatever streams they expose.
 *
 * Playlists assembling the same content do not have to reference the same clips in the same order.
 * Discs hide the movie among copies of it, either by giving each copy its own copies of the short
 * clips joining the movie's longer ones (eg. Avatar (2009), whose playlists 800 and 801 share the
 * movie's clips but each use their own copies of the clips joining them), or by playing the same
 * clips in a different order (eg. John Wick: Chapter 3 - Parabellum (2019), whose 385 copies of the
 * movie are 385 orderings of the same 19 clips). Playlists of the same overall length, cut into the
 * same number of chapters and into clips of the same durations, therefore present the same content
 * however they reference and order those clips - provided they have a clip in common, without which
 * unrelated titles that happen to run the same length would be taken for copies of one another.
 */
bool IsSamePresentation(const PlaylistInformation& a,
                        const std::vector<std::chrono::milliseconds>& aClipDurations,
                        const PlaylistInformation& b,
                        const std::vector<std::chrono::milliseconds>& bClipDurations)
{
  if (a.duration != b.duration)
    return false;
  if (a.clips == b.clips)
    return true;
  if (aClipDurations.size() < 2 || a.chapters.size() != b.chapters.size() ||
      aClipDurations != bClipDurations)
    return false;

  const std::set<unsigned int> aClips{a.clips.begin(), a.clips.end()};
  return std::ranges::any_of(b.clips,
                             [&aClips](unsigned int clip) { return aClips.contains(clip); });
}

/*!
 * \brief The playlist presenting the movie most fully of those of (near) the longest length.
 *
 * Playlists within MOVIE_EQUAL_LENGTH_TOLERANCE of one another are the same movie presented
 * differently rather than separate editions of it, so the longest is not necessarily the best -
 * a sing along wrapping the movie in a bumper runs a second or two longer while offering fewer
 * streams. Only the streams are compared, as holding the same content in more chapters makes a
 * playlist no fuller a presentation of it.
 */
const PlaylistInformation& GetBestMoviePlaylist(const std::vector<PlaylistInformation>& playlists)
{
  const auto offersMoreStreams{[](const PlaylistInformation& a, const PlaylistInformation& b)
                               {
                                 if (a.audioStreams.size() != b.audioStreams.size())
                                   return a.audioStreams.size() > b.audioStreams.size();
                                 return a.pgStreams.size() > b.pgStreams.size();
                               }};

  const auto longest{std::ranges::max_element(playlists, {}, &PlaylistInformation::duration)};
  const PlaylistInformation* best{&*longest};
  for (const auto& playlist : playlists)
  {
    if (std::chrono::abs(playlist.duration - longest->duration) <= MOVIE_EQUAL_LENGTH_TOLERANCE &&
        offersMoreStreams(playlist, *best))
      best = &playlist;
  }
  return *best;
}

bool IsRicherPresentation(const PlaylistInformation& a, const PlaylistInformation& b)
{
  if (a.audioStreams.size() != b.audioStreams.size())
    return a.audioStreams.size() > b.audioStreams.size();
  if (a.pgStreams.size() != b.pgStreams.size())
    return a.pgStreams.size() > b.pgStreams.size();
  if (a.chapters.size() != b.chapters.size())
    return a.chapters.size() > b.chapters.size();
  return a.playlist < b.playlist;
}

/*!
 * \brief Discards the copies a disc holds of the same presentation, keeping the fullest of each.
 *
 * Discs present the same content through several playlists both to expose different sets of streams
 * (eg. 28 Days Later) and to hide the movie among copies of itself (eg. John Wick: Chapter 3 -
 * Parabellum (2019), which holds 385 copies). Only one of each set of copies is a candidate.
 *
 * The playlist the disc names as the main one (from disc.inf) is always the copy kept, as the rest
 * of the search identifies the movie by it.
 */
void RemoveDuplicateMoviePlaylists(std::vector<PlaylistInformation>& playlists,
                                   const ClipMap& clips,
                                   GetTitle job,
                                   int mainPlaylist)
{
  if (job == GetTitle::ALL || playlists.size() < 2)
    return;

  // The clip durations are gathered up front, as a disc can hold hundreds of copies of the movie
  std::vector<std::vector<std::chrono::milliseconds>> clipDurations;
  clipDurations.reserve(playlists.size());
  std::ranges::transform(playlists, std::back_inserter(clipDurations),
                         [&clips](const PlaylistInformation& playlist)
                         { return GetSortedClipDurations(playlist, clips); });

  std::unordered_set<unsigned int> duplicatePlaylists;
  for (size_t i = 0; i + 1 < playlists.size(); ++i)
  {
    if (duplicatePlaylists.contains(playlists[i].playlist))
      continue;

    for (size_t j = i + 1; j < playlists.size(); ++j)
    {
      if (duplicatePlaylists.contains(playlists[j].playlist) ||
          !IsSamePresentation(playlists[i], clipDurations[i], playlists[j], clipDurations[j]))
        continue;

      // The main playlist (from disc.inf) is the copy kept, however fully the other presents the
      // content
      const bool firstIsMain{std::cmp_equal(playlists[i].playlist, mainPlaylist)};
      const bool secondIsMain{std::cmp_equal(playlists[j].playlist, mainPlaylist)};
      const bool keepFirst{firstIsMain ||
                           (!secondIsMain && IsRicherPresentation(playlists[i], playlists[j]))};
      const PlaylistInformation& duplicate{keepFirst ? playlists[j] : playlists[i]};
      const PlaylistInformation& kept{keepFirst ? playlists[i] : playlists[j]};
      const bool keptIsMain{keepFirst ? firstIsMain : secondIsMain};
      LogMoviePlaylist(
          StringUtils::Format(keptIsMain
                                  ? "Rejected (main playlist {} presents the same content) -"
                                  : "Rejected (playlist {} presents the same content as fully) -",
                              kept.playlist),
          duplicate);
      duplicatePlaylists.emplace(duplicate.playlist);

      if (!keepFirst)
        break; // playlists[i] is itself a duplicate, so cannot stand in for the ones after it
    }
  }

  std::erase_if(playlists, [&duplicatePlaylists](const PlaylistInformation& playlist)
                { return duplicatePlaylists.contains(playlist.playlist); });
}

bool FilterMoviePlaylists(std::vector<PlaylistInformation>& playlists, GetTitle job)
{
  if (job != GetTitle::ALL)
  {
    // Remove all playlists less than MIN_MOVIE_DURATION
    RemovePlaylists(playlists, "shorter than minimum movie duration",
                    [](const PlaylistInformation& playlist)
                    { return playlist.duration < MIN_MOVIE_DURATION; });
  }
  if (playlists.empty())
    CLog::LogF(LOGDEBUG, "No playlists of at least minimum movie duration");

  return !playlists.empty();
}

//! \brief The playlist's video resolution, or 0 if not known.
int GetPlaylistHeight(const PlaylistInformation& playlist)
{
  if (playlist.videoStreams.empty())
    return 0;
  return std::ranges::max(playlist.videoStreams | std::views::transform(&VideoStreamInfo::height));
}

//!  \brief The audio languages the playlist offers, empty if not known.
std::set<std::string> GetPlaylistLanguages(const PlaylistInformation& playlist)
{
  if (playlist.languages.empty())
    return {};
  const std::vector<std::string> languages{StringUtils::Split(playlist.languages, ",")};
  return {languages.begin(), languages.end()};
}

/*!
 * \brief Discards playlists of a lower resolution than another candidate.
 *
 * The movie is always presented at the disc's highest resolution, as is every version of it,
 * whereas extras are often standard definition.
 *
 * Playlists without video stream information are neither discarded nor
 * used for comparison.
 */
void FilterMoviePlaylistsByResolution(std::vector<PlaylistInformation>& playlists,
                                      GetTitle job,
                                      int mainPlaylist)
{
  if (job == GetTitle::ALL || playlists.size() < 2)
    return;

  std::map<unsigned int, int> heights;
  for (const auto& playlist : playlists)
  {
    const int height{GetPlaylistHeight(playlist)};
    if (height > 0)
      heights[playlist.playlist] = height;
  }

  if (heights.empty())
    return;

  const int highest{std::ranges::max(heights | std::views::values)};
  CLog::LogF(LOGDEBUG, "Highest resolution of any playlist is {}", highest);

  RemovePlaylists(playlists, "lower resolution than another playlist",
                  [&heights, highest, mainPlaylist](const PlaylistInformation& playlist)
                  {
                    // Remove if not main playlist (from disc.inf) and lower resolution than another playlist
                    const auto it{heights.find(playlist.playlist)};
                    return it != heights.end() && it->second < highest &&
                           std::cmp_not_equal(playlist.playlist, mainPlaylist);
                  });
}

void GetMainMoviePlaylists(std::vector<PlaylistInformation>& playlists,
                           GetTitle job,
                           int mainPlaylist)
{
  // If any playlists have more than one chapter, discard those without
  if (job != GetTitle::ALL && std::ranges::any_of(playlists, [](const PlaylistInformation& p)
                                                  { return p.chapters.size() > 1; }))
  {
    RemovePlaylists(playlists, "single chapter",
                    [mainPlaylist](const PlaylistInformation& p) {
                      return p.chapters.size() <= 1 && std::cmp_not_equal(p.playlist, mainPlaylist);
                    });
  }

  if (playlists.empty())
    return;

  if (job == GetTitle::SINGLE && mainPlaylist >= 0)
  {
    const auto it{std::ranges::find(playlists, mainPlaylist, &PlaylistInformation::playlist)};
    if (it != playlists.end())
    {
      CLog::LogF(LOGDEBUG, "Using playlist {} from disc information (disc.inf)", mainPlaylist);
      playlists = {std::move(*it)};
      return;
    }
  }

  const auto it{std::ranges::max_element(playlists, {}, &PlaylistInformation::duration)};
  if (job == GetTitle::SINGLE)
  {
    // Single longest playlist. Where playlists are of (near) identical length they are the same
    // movie presented differently rather than another edition of it (eg. a sing along, which wraps
    // the movie in a bumper and drops the other audio tracks), so the fullest presentation is used
    const PlaylistInformation& best{GetBestMoviePlaylist(playlists)};
    LogMoviePlaylist(&best == &*it ? "Using longest -" : "Using fullest of the longest -", best);
    playlists = {best};
    return;
  }

  if (job == GetTitle::MAIN)
  {
    // All playlists with duration of at least 70% of the longest title (to allow multiple editions
    // on same disc), plus any playlist presenting the movie as fully (resolution, languages) as
    // the longest one does
    const auto minimumDuration{it->duration * MAIN_TITLE_LENGTH_PERCENT / 100};
    const auto minimumEditionDuration{it->duration * MIN_EDITION_LENGTH_PERCENT / 100};
    const std::set<std::string> longestLanguages{GetPlaylistLanguages(*it)};
    const int longestHeight{GetPlaylistHeight(*it)};
    LogMoviePlaylist("Longest -", *it);
    CLog::LogF(LOGDEBUG, "Accepting playlists of at least {}% of the longest ({})",
               MAIN_TITLE_LENGTH_PERCENT,
               StringUtils::SecondsToTimeString(static_cast<int>(
                   std::chrono::duration_cast<std::chrono::seconds>(minimumDuration).count())));
    RemovePlaylists(
        playlists, "shorter than minimum percentage of longest playlist",
        [minimumDuration, minimumEditionDuration, &longestLanguages, longestHeight,
         mainPlaylist](const PlaylistInformation& playlist)
        {
          if (playlist.duration >= minimumDuration ||
              std::cmp_equal(playlist.playlist, mainPlaylist))
            return false;

          // Another edition of the movie can be considerably shorter than the longest one (eg. a
          // theatrical cut against an extended one), but offers at least the same languages and
          // resolution. An edition may have more languages than a longer one (more dubs having
          // been made of the theatrical release), so an exact match isn't required
          if (longestLanguages.empty() || playlist.duration < minimumEditionDuration)
            return true;

          const std::set<std::string> languages{GetPlaylistLanguages(playlist)};
          const int height{GetPlaylistHeight(playlist)};
          return !std::ranges::includes(languages, longestLanguages) ||
                 (height > 0 && longestHeight > 0 && height < longestHeight);
        });
  }
}

void EndMoviePlaylistSearch(const std::vector<PlaylistInformation>& playlists)
{
  LogMoviePlaylists("Selected -", playlists);
  CLog::LogF(LOGDEBUG, "*** Movie Search End ***");
}

std::shared_ptr<CFileItem> GenerateMovieItem(const CURL& url,
                                             unsigned int playlist,
                                             unsigned int mainPlaylist,
                                             const PlaylistInformation& information)
{
  CURL path{url};
  std::string buf{StringUtils::Format("BDMV/PLAYLIST/{:05}.mpls", playlist)};
  path.SetFileName(buf);
  const auto item{std::make_shared<CFileItem>(path.Get(), false)};

  // Get clips
  const std::chrono::milliseconds duration{information.duration};

  // Get languages
  const std::string langs{information.languages};

  CVideoInfoTag* itemTag{item->GetVideoInfoTag()};
  itemTag->SetDuration(static_cast<int>(duration.count() / 1000));
  item->SetProperty("bluray_playlist", playlist);

  buf = StringUtils::Format(
      playlist == mainPlaylist
          ? CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25004) /* Main Title */
          : CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25005) /* Title */,
      playlist);
  item->SetTitle(buf);
  item->SetLabel(buf);

  const std::string chap{StringUtils::Format(
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25007),
      information.chapters.size(),
      StringUtils::SecondsToTimeString(static_cast<int>(duration.count() / 1000)))};
  item->SetLabel2(chap);

  item->SetSize(0);
  item->SetArt("icon", "DefaultVideo.png");

  return item;
}

void PopulateMovieFileItems(
    const CURL& url,
    CFileItemList& items,
    int mainPlaylist,
    const CFileItemList& allTitles, // FileItem for each playlist on the disc (no stream details)
    const std::vector<PlaylistInformation>& playlists,
    const StreamDetailsProvider& getStreamDetails)
{
  // Sort by duration (putting mainPlaylist first if present)
  auto sortedPlaylists = playlists;
  std::ranges::sort(sortedPlaylists,
                    [mainPlaylist](const PlaylistInformation& a, const PlaylistInformation& b)
                    {
                      const bool aIsMain{std::cmp_equal(a.playlist, mainPlaylist)};
                      const bool bIsMain{std::cmp_equal(b.playlist, mainPlaylist)};

                      if (aIsMain || bIsMain)
                        return aIsMain && !bIsMain;

                      if (a.duration != b.duration)
                        return a.duration > b.duration;

                      return a.playlist < b.playlist;
                    });

  // The first item becomes the default version, so the best playlist leads. Of playlists of (near)
  // identical length the longest is not necessarily the fullest presentation of the movie
  if (std::ranges::none_of(sortedPlaylists, [mainPlaylist](const PlaylistInformation& playlist)
                           { return std::cmp_equal(playlist.playlist, mainPlaylist); }))
  {
    const auto best{std::ranges::find(sortedPlaylists,
                                      GetBestMoviePlaylist(sortedPlaylists).playlist,
                                      &PlaylistInformation::playlist)};
    std::rotate(sortedPlaylists.begin(), best, std::next(best));
  }

  for (const auto& playlist : sortedPlaylists)
  {
    const auto newItem{GenerateMovieItem(url, playlist.playlist, mainPlaylist, playlist)};
    if (!newItem)
    {
      CLog::LogF(LOGDEBUG, "Failed to generate FileItem for playlist {}", playlist.playlist);
      continue;
    }

    AddStreamDetails(getStreamDetails, allTitles, playlist.playlist, *newItem);

    items.Add(newItem);
  }
}
} // namespace

bool CDiscDirectoryHelper::GetMoviePlaylists(const CURL& url,
                                             CFileItemList& items,
                                             const CFileItemList& allTitles,
                                             int mainPlaylist,
                                             GetTitle job,
                                             const ClipMap& clips,
                                             const PlaylistMap& playlistMap)
{
  items.Clear();
  Reset();

  // Checks
  if (playlistMap.empty() || clips.empty())
    return false;

  std::vector<PlaylistInformation> playlists;
  InitialiseMoviePlaylistSearch(playlists, playlistMap, job, mainPlaylist);
  RemoveDuplicateMoviePlaylists(playlists, clips, job, mainPlaylist);
  if (!FilterMoviePlaylists(playlists, job))
  {
    EndMoviePlaylistSearch(playlists);
    return false;
  }
  FilterMoviePlaylistsByResolution(playlists, job, mainPlaylist);
  GetMainMoviePlaylists(playlists, job, mainPlaylist);
  PopulateMovieFileItems(url, items, mainPlaylist, allTitles, playlists, m_getStreamDetails);
  EndMoviePlaylistSearch(playlists);

  return !items.IsEmpty();
}

void CDiscDirectoryHelper::AddRootOptions(const CURL& url,
                                          CFileItemList& items,
                                          AllTitles allTitlesType,
                                          AddMenuOption addMenuOption)
{
  CURL path{url};
  if (allTitlesType == AllTitles::MOVIES)
    path.SetFileName(URIUtils::AddFileToFolder("root", "titles", "all"));
  else if (allTitlesType == AllTitles::EPISODES)
    path.SetFileName(URIUtils::AddFileToFolder("root", "titles", "episodes", "all"));

  auto item{std::make_shared<CFileItem>(path.Get(), true)};
  item->SetLabel(
      CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25002) /* All titles */);
  item->SetArt("icon", "DefaultVideoPlaylists.png");
  items.Add(item);

  if (addMenuOption == AddMenuOption::ADD_MENU)
  {
    path.SetFileName("menu");
    item = {std::make_shared<CFileItem>(path.Get(), false)};
    item->SetLabel(
        CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(25003) /* Menu */);
    item->SetArt("icon", "DefaultProgram.png");
    items.Add(item);
  }
}

std::vector<CVideoInfoTag> CDiscDirectoryHelper::GetEpisodesOnDisc(const CURL& url)
{
  CVideoDatabase database;
  if (!database.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database");
    return {};
  }
  std::vector<CVideoInfoTag> episodesOnDisc;
  database.GetEpisodesByBlurayPath(url.Get(), episodesOnDisc);
  std::ranges::sort(episodesOnDisc,
                    [](const CVideoInfoTag& i, const CVideoInfoTag& j)
                    {
                      if (i.m_iSeason == j.m_iSeason)
                        return i.m_iEpisode < j.m_iEpisode;
                      return i.m_iSeason < j.m_iSeason;
                    });
  return episodesOnDisc;
}

bool CDiscDirectoryHelper::GetOrShowPlaylistSelection(const CFileItem& item,
                                                      CFileItemList& items,
                                                      MenuDecision playback)
{
  const bool silent{playback == MenuDecision::SILENT};
  const auto settings = CServiceBroker::GetSettingsComponent()->GetSettings();
  const auto action = static_cast<SimilarVideoScanAction>(
      settings->GetInt(CSettings::SETTING_VIDEOLIBRARY_SIMILARVIDEOACTION));
  const bool returnMultipleItems{(silent && action != SimilarVideoScanAction::NONE &&
                                  item.GetVideoContentType() == VideoDbContentType::MOVIES)};

  const std::string directory{
      [&item, &playback, &returnMultipleItems]
      {
        const bool forceSelection{item.GetProperty("force_playlist_selection").asBoolean(false)};

        // All episodes
        if (item.HasProperty("episodes_start"))
          return URIUtils::GetBlurayAllEpisodesPath(item.GetDynPath());

        // Single episode
        if (item.GetVideoContentType() == VideoDbContentType::EPISODES && !forceSelection)
        {
          const CVideoInfoTag* tag{item.GetVideoInfoTag()};
          return URIUtils::GetBlurayEpisodePath(item.GetDynPath(), tag->m_iSeason, tag->m_iEpisode);
        }

        // Playlists > 70% longest
        using enum MenuDecision;
        using enum VideoDbContentType;
        if (playback == SHOW_SIMPLE_MENU)
        {
          if (item.GetVideoContentType() == EPISODES || item.GetVideoContentType() == TVSHOWS)
            return URIUtils::GetBlurayTitlesPath(item.GetDynPath(), URIUtils::GetAllTitles::LONG,
                                                 URIUtils::AllTitlesOptions::EPISODES);
          else
            return URIUtils::GetBlurayTitlesPath(item.GetDynPath(), URIUtils::GetAllTitles::LONG,
                                                 URIUtils::AllTitlesOptions::MOVIES);
        }

        if (item.GetVideoContentType() == EPISODES || item.GetVideoContentType() == TVSHOWS ||
            forceSelection)
          // Single main title
          return URIUtils::GetBlurayMainTitlePath(item.GetDynPath());
        else if (returnMultipleItems)
          // Versions
          return URIUtils::GetBlurayMainTitlePath(item.GetDynPath(), URIUtils::GetAllTitles::ALL);
        else
          // Single main title
          return URIUtils::GetBlurayMainTitlePath(item.GetDynPath());
      }()};
  if (directory.empty())
  {
    CLog::LogF(LOGERROR, "Unable to derive a bluray path from {}",
               CURL::GetRedacted(item.GetDynPath()));
    return false;
  }

  // Get playlists that are already used (to avoid duplicates in file table)
  std::vector<CVideoDatabase::PlaylistInfo> usedPlaylists{};
  CVideoDatabase database;
  if (!database.Open())
  {
    CLog::LogF(LOGERROR, "Failed to open video database");
    return false;
  }

  // Add duration to bluray:// url as needed for episode determination in CBlurayDirectory
  std::string directoryDuration{directory};
  if (item.HasVideoInfoTag() && item.GetVideoInfoTag()->GetDuration() > 0 &&
      item.GetVideoContentType() == VideoDbContentType::EPISODES)
  {
    CURL dirUrl(directory);
    dirUrl.SetOption("duration", std::to_string(item.GetVideoInfoTag()->GetDuration()));
    directoryDuration = dirUrl.Get();
  }

  // Get items
  CFileItemList sourceItems;
  if (!GetItems(sourceItems, directoryDuration, silent))
  {
    // No main movie or episode playlist found
    if (silent)
      return false;

    // Not silent so fallback to all titles
    const std::string fallbackDirectory{
        item.GetVideoContentType() == VideoDbContentType::EPISODES ||
                item.GetVideoContentType() == VideoDbContentType::TVSHOWS
            ? URIUtils::GetBlurayTitlesPath(item.GetDynPath(), URIUtils::GetAllTitles::ALL,
                                            URIUtils::AllTitlesOptions::EPISODES)
            : URIUtils::GetBlurayTitlesPath(item.GetDynPath(), URIUtils::GetAllTitles::ALL,
                                            URIUtils::AllTitlesOptions::MOVIES)};
    if (!GetItems(sourceItems, fallbackDirectory, silent))
    {
      CGUIDialogOK::ShowAndGetInput(
          CVariant{257},
          CVariant{item.GetVideoContentType() == VideoDbContentType::EPISODES ? 25017 : 25016});
      return false;
    }
  }

  // Select item
  CFileItem selectedItem;
  if (!silent)
  {
    if (playback == MenuDecision::SHOW_SIMPLE_MENU)
    {
      usedPlaylists =
          database.GetPlaylistsByPath(URIUtils::GetBlurayPlaylistPath(item.GetDynPath()));

      // If replacing existing playlist (FORCE_PLAYLIST_SELECTION), remove it from exclude list
      // as user could choose the same playlist again
      if (item.GetProperty("force_playlist_selection").asBoolean(false))
      {
        CRegExp regex{true, CRegExp::autoUtf8, R"(\/(\d{5}).mpls$)"};
        if (regex.RegFind(item.GetDynPath()) != -1)
        {
          const int playlist{std::stoi(regex.GetMatch(1))};
          std::erase_if(usedPlaylists, [&playlist](const CVideoDatabase::PlaylistInfo& p)
                        { return p.playlist == playlist; });
        }
      }

      // Use simple menu dialog to select playlist
      while (true)
      {
        if (!CGUIDialogSimpleMenu::ShowPlaylistSelection(item, selectedItem, sourceItems,
                                                         usedPlaylists))
          return false;

        // If a non-folder item is selected, we're done
        if (!selectedItem.IsFolder())
          break;

        // Folder selected - retrieve all titles within it
        if (!GetItems(sourceItems, selectedItem.GetDynPath(), silent))
          return false;
      }
    }
  }
  else
  {
    // Silent
    if (sourceItems.Size() > 1 && !returnMultipleItems)
    {
      CLog::LogF(LOGDEBUG, "Automatically selected playlist {} of the {} offered for {}",
                 sourceItems[0]->GetProperty("bluray_playlist").asInteger32(0), sourceItems.Size(),
                 CURL::GetRedacted(directory));
    }
  }

  auto GenerateItem{
      [](const CFileItem& originalItem, const CFileItem& selectedItem, const CFileItem& item)
      {
        auto newItem{std::make_shared<CFileItem>(originalItem)};
        newItem->SetDynPath(selectedItem.GetDynPath());
        const auto tag{newItem->GetVideoInfoTag()};
        tag->SetFileNameAndPath(selectedItem.GetDynPath());
        if (selectedItem.HasVideoInfoTag())
        {
          // Don't overwrite streamdetails that came from an nfo
          if (selectedItem.GetVideoInfoTag()->HasStreamDetails() && !tag->HasNFOStreamDetails())
            tag->m_streamDetails = selectedItem.GetVideoInfoTag()->m_streamDetails;

          // Episode bookmarks
          if (const CBookmark& bookmark{selectedItem.GetVideoInfoTag()->m_EpBookmark};
              bookmark.IsSet())
            tag->m_EpBookmark = bookmark;

          // The duration of the playlist, or of the episode's part of it where several share one, is
          // measured from the disc and so is preferred over the scraper's.
          // Loose sanity check that the scraper and found durations are similar, to avoid a
          // mis-identified playlist from overwriting the episode's duration and affecting future
          // playlist identification.
          static constexpr int SCRAPED_DURATION_TOLERANCE_PERCENT{50};
          const unsigned int scrapedDuration{tag->GetStaticDuration()};
          if (const unsigned int discDuration{selectedItem.GetVideoInfoTag()->GetDuration()};
              discDuration > 0 &&
              (scrapedDuration == 0 ||
               CheckDurationsWithinTolerance(scrapedDuration * 1000ms, discDuration * 1000ms,
                                             SCRAPED_DURATION_TOLERANCE_PERCENT)))
            tag->SetDuration(static_cast<int>(discDuration));
        }

        if (tag->GetAssetInfo().GetTitle().empty())
          tag->GetAssetInfo().SetTitle(
              CServiceBroker::GetResourcesComponent().GetLocalizeStrings().Get(
                  VIDEO_VERSION_ID_DEFAULT));
        if (selectedItem.HasProperty("bluray_playlist"))
          newItem->SetProperty("bluray_playlist", selectedItem.GetProperty("bluray_playlist"));
        newItem->SetProperty("original_listitem_url", item.GetDynPath());
        return newItem;
      }};

  items.Clear();
  if (!selectedItem.GetPath().empty())
  {
    // If SelectedItem is not empty then we have a user selected playlist, so return it
    const auto newItem{GenerateItem(item, selectedItem, item)};

    // GenerateItem points the paths in the tag at the newly selected playlist
    // Flag so CSaveFileStateJob can tell playlist has changed
    if (selectedItem.GetDynPath() != item.GetDynPath())
      newItem->SetProperty("new_playlist_path", true);

    items.Add(newItem);
  }
  else if (!returnMultipleItems)
    // Return single item
    items.Add(GenerateItem(item, *sourceItems[0], item));
  else
  {
    // Return all items
    for (const auto& sourceItem : sourceItems)
      items.Add(GenerateItem(item, *sourceItem, item));
  }

  return true;
}

bool CDiscDirectoryHelper::GetItems(CFileItemList& items, const std::string& directory, bool silent)
{
  items.Clear();
  if (!GetDirectoryItems(directory, items, CDirectory::CHints(), silent))
  {
    CLog::LogF(LOGERROR, "Failed to get play directory for {}", directory);
    return false;
  }

  if (items.IsEmpty())
  {
    CLog::LogF(LOGERROR, "Failed to get any items in {}", directory);
    return false;
  }

  return true;
}

bool CDiscDirectoryHelper::GetDirectoryItems(const std::string& path,
                                             CFileItemList& items,
                                             const CDirectory::CHints& hints,
                                             bool silent)
{
  if (silent)
  {
    // Non-interactive path so skip the busy dialog
    return CDirectory::GetDirectory(path, items, hints);
  }

  CGetDirectoryItems getItems(path, items, hints);
  if (!CGUIDialogBusy::Wait(&getItems, 100, true))
  {
    return false;
  }
  return getItems.m_result;
}
