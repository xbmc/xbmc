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
//! Title numbers that are not disc titles
constexpr unsigned int TITLE_TOP_MENU = 0;
constexpr unsigned int TITLE_FIRST_PLAYBACK = 0xFFFF;

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
 \brief Where a given MovieObject.bdmv object is referenced from.
 A single object can back several titles, so titles is a list.
 */
struct MovieObjectUsage
{
  bool firstPlayback{false};
  bool topMenu{false};
  std::vector<unsigned int> titles; // 1-based title numbers
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

  /*! \brief Whether the top menu is an HDMV object rather than a BD-J application.
   Discs with an HDMV top menu tend to express each menu entry as its own title, so their
   MovieObject.bdmv describes the disc's structure. Discs with a BD-J top menu do not - their
   navigation lives in the BD-J application and MovieObject.bdmv is at best a stub. */
  bool HasHdmvTopMenu() const;

  /*! \brief Find where a MovieObject.bdmv object is referenced from. */
  MovieObjectUsage GetMovieObjectUsage(unsigned int movieObject) const;
};

class CIndexParser
{
public:
  /*! \brief Parse the disc's BDMV/index.bdmv into indexInformation. */
  static bool ReadIndex(const CURL& url, IndexInformation& indexInformation);
};
} // namespace XFILE
