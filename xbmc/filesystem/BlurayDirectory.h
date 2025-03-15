/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DiscDirectoryHelper.h"
#include "IDirectory.h"
#include "URL.h"

#include <memory>

#include <libbluray/bluray.h>

class CFileItem;
class CFileItemList;

enum class BLURAY_PLAYBACK_TYPE : unsigned int
{
  SEQUENTIAL = 1,
  RANDOM,
  SHUFFLE
};

enum class BLURAY_CONNECTION : unsigned int
{
  SEAMLESS = 1,
  NONSEAMLESS = 5,
  BRANCHING = 6 // Out-of-mux connection
};

enum class BLURAY_SUBPATH_TYPE : unsigned int
{
  PRIMARY_AUDIO_SLIDESHOW = 2,
  INTERACTIVE_GRAPHICS_PRESENTATION_MENU,
  TEXT_SUBTITLE_PRESENTATION,
  OUTOFMUX_SYNCHRONOUS,
  OUTOFMUX_ASYNCHRONOUS_PIP,
  INMUX_SYNCHRONOUS_PIP,
  STEREOSCOPIC_VIDEO,
  STEREOSCOPIC_INTERACTIVE_GRAPHICS_MENU,
  DOLBY_VISION_LAYER
};

enum class BLURAY_STREAM_TYPE : unsigned int
{
  PLAYITEM = 1,
  SUBPATH,
  SUBPATH_INMUX_SYNCHRONOUS_PIP,
  SUBPATH_DOLBY_VISION_LAYER
};

enum class BLURAY_MARK_TYPE : unsigned int
{
  ENTRY = 1,
  LINK
};

enum class STREAM_TYPE : unsigned int
{
  VIDEO_STREAM = 0,
  AUDIO_STREAM,
  PRESENTATION_GRAPHIC_STREAM,
  INTERACTIVE_GRAPHIC_STREAM,
  SECONDARY_VIDEO_STREAM,
  SECONDARY_AUDIO_STREAM,
  PICTURE_IN_PICTURE_SUBTITLE_STREAM,
  DOLBY_VISION_STREAM
};

struct StreamInformation
{
  BLURAY_STREAM_TYPE type;
  unsigned int coding{0};
  unsigned int packetIdentifier{0};
  unsigned int subpathId{0};
  unsigned int subclipId{0};
  unsigned int format{0};
  unsigned int rate{0};
  unsigned int aspect{0}; // CLPI only
  unsigned int dynamicRangeType{0};
  unsigned int colorSpace{0};
  bool copyRestricted{false};
  bool outOfMux{false}; // CLPI only
  bool HDRPlus{false};
  unsigned int characterEncoding{0};
  std::string language;
  std::vector<unsigned int> secondaryAudio_audioReferences;
  std::vector<unsigned int> secondaryVideo_audioReferences;
  std::vector<unsigned int> secondaryVideo_presentationGraphicReferences;
};

struct ProgramInformation
{
  unsigned int spnProgramSequenceStart{0};
  unsigned int programId{0};
  unsigned int numGroups{0};
  std::vector<StreamInformation> streams;
};

struct ClipInformation
{
  unsigned int clip{0};
  std::string version;
  std::string codec;
  unsigned int time{0}; // calculated from playItem
  unsigned int duration{0}; // calculated from playItem
  std::vector<ProgramInformation> programs;
};

struct PlaylistMarkInformation
{
  BLURAY_MARK_TYPE markType;
  unsigned int playItemReference{0};
  unsigned int time{0};
  unsigned int elementaryStreamPacketIdentifier{0};
  unsigned int duration{0};
};

struct ChapterInformation
{
  unsigned int chapter{0};
  unsigned int start{0};
  unsigned int duration{0};
};

struct SubPlayItemInformation
{
  BLURAY_CONNECTION connectionCondition;
  bool isMultiClip{false};
  unsigned int inTime{0};
  unsigned int outTime{0};
  unsigned int syncPlayItemId{0};
  std::vector<ClipInformation> clips;
};

struct SubPathInformation
{
  BLURAY_SUBPATH_TYPE type;
  bool repeat{false};
  std::vector<SubPlayItemInformation> subPlayItems;
};

struct PlayItemInformation
{
  bool isMultiAngle{false};
  std::vector<ClipInformation> angleClips;
  BLURAY_CONNECTION connectionCondition{0};
  unsigned int inTime{0};
  unsigned int outTime{0};
  bool randomAccessFlag{false};
  unsigned int stillMode{0};
  unsigned int stillTime{0};
  std::vector<StreamInformation> videoStreams;
  std::vector<StreamInformation> audioStreams;
  std::vector<StreamInformation> presentationGraphicStreams;
  std::vector<StreamInformation> interactiveGraphicStreams;
  std::vector<StreamInformation> secondaryAudioStreams;
  std::vector<StreamInformation> secondaryVideoStreams;
  std::vector<StreamInformation> dolbyVisionStreams;
};

struct PlaylistInformation
{
  unsigned int playlist{0};
  std::string version;
  unsigned int duration{0};
  BLURAY_PLAYBACK_TYPE playbackType;
  unsigned int playbackCount{0};
  std::vector<PlayItemInformation> playItems;
  std::vector<ClipInformation> clips;
  std::vector<SubPlayItemInformation> subPlayItems;
  std::vector<PlaylistMarkInformation> playlistMarks;
  std::vector<ChapterInformation> chapters;
};

namespace XFILE
{

class CBlurayDirectory : public IDirectory
{
public:
  CBlurayDirectory();
  ~CBlurayDirectory() override;
  bool GetDirectory(const CURL& url, CFileItemList& items) override;

  bool InitializeBluray(const std::string &root);
  static std::string GetBasePath(const CURL& url);
  std::string GetBlurayTitle();
  std::string GetBlurayID();

private:
  enum class DiscInfo
  {
    TITLE,
    ID
  };

  void Dispose();
  std::string GetDiscInfoString(DiscInfo info);
  void GetPlaylistsInformation(ClipMap& clips, PlaylistMap& playlists);
  bool GetPlaylists(GetTitlesJob job, CFileItemList& items, SortTitlesJob sort);
  int GetMainPlaylistFromDisc() const;
  std::shared_ptr<CFileItem> GetFileItem(const PlaylistInfo& title, const std::string& label) const;

  std::string GetCachePath() const;
  const BLURAY_DISC_INFO* GetDiscInfo() const;
  bool GetPlaylistInfoFromDisc(unsigned int playlist, PlaylistInfo& p);
  bool ReadMPLS(unsigned int playlist, PlaylistInformation& playlistInformation);
  bool ParseMPLS(const std::vector<char>& buffer,
                 PlaylistInformation& playlistInformation,
                 unsigned int playlist);
  static bool ParsePlayItem(const std::vector<char>& buffer,
                            unsigned int& offset,
                            PlayItemInformation& playItem);
  static bool ParseSubPlayItem(const std::vector<char>& buffer,
                               unsigned int& offset,
                               SubPlayItemInformation& subPlayItemInformation);
  static StreamInformation ParseStream(const std::vector<char>& buffer,
                                       unsigned int& offset,
                                       STREAM_TYPE streamType);

  bool ReadCLPI(unsigned int clip, ClipInformation& clipInformation) const;
  static bool ParseCLPI(const std::vector<char>& buffer,
                        ClipInformation& clipInformation,
                        unsigned int clip);

  CURL m_url;
  std::string m_realPath;
  BLURAY* m_bd{nullptr};
  bool m_blurayInitialized{false};
  bool m_blurayMenuSupport{false};

  std::map<unsigned int, ClipInformation> m_clips;
};
} // namespace XFILE