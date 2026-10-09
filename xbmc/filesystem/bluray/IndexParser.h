/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "URL.h"

#include <cstdint>
#include <string>
#include <vector>

namespace XFILE
{
/*! \brief Value of an HDMV object reference meaning 'no object'. */
constexpr unsigned int MOVIE_OBJECT_NONE = 0xFFFF;

/*! \brief The kind of navigation object a title in index.bdmv refers to. */
enum class BLURAY_OBJECT_TYPE : uint8_t
{
  NONE = 0,
  HDMV = 1,
  BDJ = 2
};

/*! \brief How a title behaves when it reaches its end. */
enum class BLURAY_TITLE_PLAYBACK_TYPE : uint8_t
{
  MOVIE = 0, // plays through and stops
  INTERACTIVE = 1 // presents a menu or runs an application
};

/*!
 \brief One entry of index.bdmv - the first playback object, the top menu object or a title.
 */
struct IndexObjectInformation
{
  BLURAY_OBJECT_TYPE objectType{BLURAY_OBJECT_TYPE::NONE};
  BLURAY_TITLE_PLAYBACK_TYPE playbackType{BLURAY_TITLE_PLAYBACK_TYPE::MOVIE};
  unsigned int movieObject{MOVIE_OBJECT_NONE}; // HDMV only - index into MovieObject.bdmv
  std::string bdjObject; // BD-J only - name of the BDMV/BDJO/<name>.bdjo file
  unsigned int accessType{0}; // titles only
};

/*!
 \brief Parsed contents of index.bdmv.
 This is the disc's table of contents - it names the first playback and top menu objects and maps
 each title to the HDMV movie object or BD-J object that implements it.
 */
struct IndexInformation
{
  std::string version;
  IndexObjectInformation firstPlayback;
  IndexObjectInformation topMenu;
  std::vector<IndexObjectInformation> titles; // titles[0] is title 1

  /*! \brief Whether any object on the disc is HDMV, ie. whether MovieObject.bdmv is used at all. */
  bool HasHdmvObjects() const;
};

class CIndexParser
{
public:
  /*! \brief Parse the disc's BDMV/index.bdmv into indexInformation. */
  static bool ReadIndex(const CURL& url, IndexInformation& indexInformation);
};
} // namespace XFILE
