/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "URL.h"
#include "filesystem/DiscDirectoryHelper.h"

#include <cstddef>
#include <map>
#include <memory>
#include <optional>
#include <unordered_map>
#include <vector>

namespace XFILE
{
enum class ENCODING_TYPE : uint8_t;
struct BlurayPlaylistInformation;

struct Descriptor
{
  // user-defined ctor required for XCode 15.2 and emplace_back
  Descriptor(unsigned int newTag, int newLength, std::vector<std::byte>&& newData);

  unsigned int tag;
  int length;
  std::vector<std::byte> data;
};

struct TSStreamInfo
{
  unsigned int pid{};
  ENCODING_TYPE streamType{};
  std::vector<Descriptor> descriptors{};

  // Determine if details complete
  unsigned int seen{0};
  bool completed{false};

  // Methods
  TSStreamInfo() = default;
  virtual ~TSStreamInfo() = default;
  TSStreamInfo(const TSStreamInfo&) = default;
  TSStreamInfo& operator=(const TSStreamInfo&) = default;
  TSStreamInfo(TSStreamInfo&&) noexcept = default;
  TSStreamInfo& operator=(TSStreamInfo&&) noexcept = default;
};

struct TSAudioStreamInfo : TSStreamInfo
{
  unsigned int channels{0};
  unsigned int sampleRate{0};

  // DTS
  bool isXLL{false}; // DTS-HD MA - needs to be true for DTS:X
  bool hasSubstream{false}; // Needs to be true for DTS:X
  bool isXLLX{false};
  bool isXLLXIMAX{false};

  // AC3 / Dolby
  bool hasDependantStream{false};
  bool isAtmos{false};
};

struct TSVideoStreamInfo : TSStreamInfo
{
  unsigned int height{0};
  unsigned int width{0};
  unsigned int bitDepth{0};
  float aspectRatio{0.0};
  bool is3d{false};

  bool hdr10{false};
  bool hdr10Plus{false};
  bool dolbyVision{false};
  bool isEnhancementLayer{false};
};

using StreamMap = std::unordered_map<unsigned int, std::shared_ptr<TSStreamInfo>>;

//
// Interactive graphics menu
//
// A clip may carry an interactive graphics stream alongside its video. That stream holds the
// disc's menu - its pages, its buttons and the navigation commands behind them - which is where a
// disc records the playlist each menu entry plays. MovieObject.bdmv says so only on discs whose
// menu is itself HDMV, and never for the per-episode or per-chapter entries a menu dispatches
// through a register.
//

/*!
 \brief What one menu button does when activated.

 A button carries the same navigation commands a movie object does, so the playlist or title it
 leads to is resolved the same way. Buttons that only move the selection around carry none, and
 leave every field empty.
 */
struct IGButtonInformation
{
  unsigned int button{0};
  bool autoAction{false}; //!< activates as soon as it is selected

  std::optional<unsigned int> playlist; //!< playlist the button plays
  std::optional<unsigned int> playItem; //!< play item within that playlist
  std::optional<unsigned int> playMark; //!< play mark (chapter) within that playlist
  std::optional<unsigned int> title; //!< title the button jumps to

  //! A chapter menu shown over the feature moves within the playlist that is already playing
  //! rather than starting one, so these say nothing about which playlist is meant.
  std::optional<unsigned int> linkPlayItem;
  std::optional<unsigned int> linkPlayMark;

  //! General purpose registers the button sets before branching. A menu that dispatches through a
  //! title commonly puts the chapter or episode index in a register first, so this is what
  //! distinguishes the buttons of a chapter or episode selection page from each other.
  std::map<unsigned int, uint32_t> registers;

  /*! \brief Whether the button leads somewhere, as opposed to only moving the selection. */
  bool IsNavigation() const
  {
    return playlist.has_value() || title.has_value() || linkPlayItem.has_value() ||
           linkPlayMark.has_value();
  }
};

struct IGPageInformation
{
  unsigned int page{0};
  unsigned int defaultSelectedButton{0};
  unsigned int defaultActivatedButton{0};
  std::vector<IGButtonInformation> buttons;
};

/*! \brief The interactive graphics menu carried by a clip. */
struct IGMenuInformation
{
  unsigned int width{0};
  unsigned int height{0};
  bool popup{false}; //!< pop-up menu rather than one shown for the whole title
  std::vector<IGPageInformation> pages;
};

class CM2TSParser
{
public:
  static bool GetStreams(const CURL& url,
                         BlurayPlaylistInformation& playlistInformation,
                         StreamMap& streams);

  static bool GetStreamsFromFile(const std::string& path,
                                 unsigned int clip,
                                 const std::string& clipExtension,
                                 StreamMap& streams);

  /*!
   \brief Parse the interactive graphics menu out of a clip's .m2ts.

   Reads the interactive composition segment of the first interactive graphics stream found. That
   segment lists every page and button of the menu together with the navigation commands behind
   them.
   
   \return true when a menu was found and decoded
   */
  static bool GetMenu(const CURL& url, unsigned int clip, IGMenuInformation& menu);

  static std::vector<std::reference_wrapper<TSVideoStreamInfo>> GetVideoStreams(
      const StreamMap& streams);
  static std::vector<std::reference_wrapper<TSAudioStreamInfo>> GetAudioStreams(
      const StreamMap& streams);
  static std::vector<std::reference_wrapper<TSStreamInfo>> GetSubtitleStreams(
      const StreamMap& streams);
};
} // namespace XFILE
