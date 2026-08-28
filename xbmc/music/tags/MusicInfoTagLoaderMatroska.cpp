/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "MusicInfoTagLoaderMatroska.h"

#include "MatroskaTagMapping.h"
#include "MatroskaTagReader.h"
#include "MusicCodecInfoFFmpeg.h"
#include "MusicInfoTag.h"
#include "ServiceBroker.h"
#include "URL.h"
#include "filesystem/FFmpegVfsContext.h"
#include "music/MusicEmbeddedCoverLoaderFFmpeg.h"
#include "settings/AdvancedSettings.h"
#include "settings/SettingsComponent.h"
#include "utils/EmbeddedArt.h"

#include <algorithm>
#include <string>
#include <vector>

using namespace MUSIC_INFO;
using namespace XFILE;

/*!
* Used by Matroska files with no chapters (most common) or with a single (one song)
* Typically these are Matroska files split by Chapter start times with each chapter having
* song tags but no chapter name or chapter tags.
*/
bool CMusicInfoTagLoaderMatroska::Load(const std::string& strFileName,
                                       CMusicInfoTag& tag,
                                       EmbeddedArt* art)
{
  tag.SetLoaded(false);

  std::vector<std::string> separators{";", " feat. ", " ft. ", " Feat. ", " Ft. ", ":",
                                      "|", "#",       "/",     " with ",  "&"};
  const std::string musicsep =
      CServiceBroker::GetSettingsComponent()->GetAdvancedSettings()->m_musicItemSeparator;
  if (musicsep.find_first_of(";/,&|#") == std::string::npos)
    separators.push_back(musicsep);

  /*!
  * One context for the three things that need one: the tag reader where FFmpeg is the reader this
  * build has, the cover art, and the codec details. Opening the file three times over SMB or NFS
  * is the cost this whole path exists to keep down.
  *
  * A failure to open is not this function's failure. Where TagLib is the reader it opens the file
  * itself and reads the tags of a file FFmpeg would not probe at all - a truncated or damaged one
  * still says what song it is. Only the cover art and the codec details are lost, and both already
  * take a null context for the answer that there are none.
  */
  CFFmpegVfsContext demux;
  const bool opened = demux.Open(strFileName);

  AVFormatContext* const fctx = demux.FormatContext();

  MatroskaAlbum album = ReadMatroskaTags(CURL(strFileName), fctx);
  if (!album.hasAlbumTags())
    return opened;

  // Before anything below asks what is a track: an open chapter has no length until the file is
  // measured.
  CloseOpenEndedChapters(album, demux.Duration());

  for (const auto& t : album.albumTags)
    MatroskaTagMapping::MapTag(t.first, t.second, MatroskaTagMapping::TagLevel::Album, separators,
                               musicsep, tag);
  for (const auto& t : album.fileTags)
    MatroskaTagMapping::MapTag(t.first, t.second, MatroskaTagMapping::TagLevel::File, separators,
                               musicsep, tag);

  /*!
  * A file with more than one track is an album, and CAudioBookFileDirectory expands it into one
  * song per chapter before this loader is ever reached. What is left here carries at most one
  * chapter long enough to be a track, and its tags describe this very song - the chapters too
  * short to be one are the artefacts CAudioBookFileDirectory did not count either.
  */
  const auto song = std::find_if(album.chapters.begin(), album.chapters.end(),
                                 [](const ChapterTags& c) { return IsTrack(c.start, c.end); });
  if (song != album.chapters.end())
    for (const auto& t : song->tags)
      MatroskaTagMapping::MapTag(t.first, t.second, MatroskaTagMapping::TagLevel::Track, separators,
                                 musicsep, tag);

  /*!
  * A song whose file says no more than what album it belongs to is still a song. The Segment
  * title is what names a track, and a tagger that wrote only album level tags left none, so the
  * album title stands in - it is the closest the file comes to saying what this is.
  */
  if (tag.GetTitle().empty())
    tag.SetTitle(tag.GetAlbum());

  // Look for any embedded cover art
  CMusicEmbeddedCoverLoaderFFmpeg::GetEmbeddedCover(fctx, tag, art);

  // Get Codec data using FFmpeg (taglib not accurate for all codecs yet - v2.3)
  // Only where the streams were read: without that the fields below are zeroes rather than
  // measurements, and TagLib has already given the tag what the header says.
  musicCodecInfo codec_info;
  if (demux.HasStreamInfo() && CMusicCodecInfoFFmpeg::GetMusicCodecInfo(fctx, codec_info))
  {
    tag.SetBitRate(codec_info.bitRate);
    tag.SetSampleRate(codec_info.sampleRate);
    tag.SetNoOfChannels(codec_info.channels);
    tag.SetDuration(codec_info.duration);
  }

  if (!tag.GetAlbum().empty() || !tag.GetTitle().empty())
    tag.SetLoaded(true);

  return true;
}
