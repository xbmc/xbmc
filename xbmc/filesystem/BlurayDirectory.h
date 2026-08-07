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
#include "bluray/MPLSParser.h"
#if defined(HAS_UDFREAD)
#include "filesystem/UDFContext.h"
#endif

#include <map>
#include <optional>
#include <string>
#include <vector>

#include <libbluray/bluray.h>

class CFileItem;
class CFileItemList;

class TestBlurayDirectory;

namespace XFILE
{
using namespace std::chrono_literals;

class CBlurayDirectory : public IDirectory
{
public:
  CBlurayDirectory();
  ~CBlurayDirectory() override;
  CBlurayDirectory(const CBlurayDirectory&) = delete;
  CBlurayDirectory& operator=(const CBlurayDirectory&) = delete;
  CBlurayDirectory(CBlurayDirectory&&) noexcept = default;
  CBlurayDirectory& operator=(CBlurayDirectory&&) noexcept = default;

  bool GetDirectory(const CURL& url, CFileItemList& items) override;
  bool Resolve(CFileItem& item) const override;

  /*!
   \brief Resolve the underlying path and open the disc with libbluray.
   Only needed by callers that want the disc's own metadata (see GetBlurayTitle/GetBlurayID).
   GetDirectory resolves the path but leaves libbluray closed until something needs it.
   \return true if libbluray could open the disc, ie. this is a bluray
   */
  bool InitializeBluray(const std::string& root);
  static std::string GetBasePath(const CURL& url);
  std::string GetBlurayTitle();
  std::string GetBlurayID();

private:
  friend class ::TestBlurayDirectory;

  /*!
   \brief Populate the stream details of a playlist on this disc.
   Deriving these means parsing the playlist's m2ts, so it is only done once a playlist is known
   to be wanted rather than for every playlist on the disc during playlist determination. Handed to
   CDiscDirectoryHelper as a StreamDetailsProvider so that it stays agnostic of the disc type.
   \param playlist the playlist to describe
   \param item the item to populate
   */
  void SetPlaylistStreamDetails(unsigned int playlist, CFileItem& item);

  /*!
   \brief Discard the playlists that are not a movie/episode.
   Removes those with no clips, those shorter than a second, those looping over their clips, and
   all but one of each set of identical playlists (the lowest numbered of them is the one kept).
   A playlist loops when it holds a single clip played more than once, or plays its clips
   MIN_LOOPED_CLIP_PLAYS times over on average, as a menu background does. One that merely
   revisits a clip is kept.
   \param playlists the playlists to filter, in place
   \return true if any playlist remains
   */
  static bool FilterPlaylists(std::vector<PlaylistInformation>& playlists);

  /*!
   \brief Get the playlist(s) on the disc as FileItems, without their stream details.
   \param playlist a single playlist to return, or ALL_PLAYLISTS for every valid one
   \return true if any playlist was found
   */
  static bool GetPlaylists(const CURL& url,
                           const std::string& realPath,
                           int flags,
                           int playlist,
                           CFileItemList& items,
                           std::map<unsigned int, ClipInformation>& clipCache);

  /*!
   \brief Describe every playlist on the disc and the clips they share, caching the result.
   \return true if the information was read from the disc and cached
   */
  static bool GetPlaylistsInformation(const CURL& url,
                                      const std::string& realPath,
                                      int flags,
                                      CFileItemList& allTitles,
                                      ClipMap& clips,
                                      PlaylistMap& playlists,
                                      std::map<unsigned int, ClipInformation>& clipCache);

  enum class DiscInfo : uint8_t
  {
    TITLE,
    ID
  };

  /*!
   \brief Resolve the disc's path through its directory handler, without touching the disc.
   */
  void SetRealPath(const std::string& root);

  /*!
   \brief Open the disc with libbluray, unless already open.
   Opening costs a dozen round trips to the disc (index.bdmv, the BDMV/META localisations,
   CERTIFICATE/id.bdmv and the AACS probe), which is why it is deferred until a caller needs
   something only libbluray can answer. SetRealPath must have been called first.
   \return true if libbluray has the disc open
   */
  bool EnsureBlurayOpen();

  /*!
   \brief Get whether this disc supports menus, opening it only on the first call per disc.
   */
  bool HasMenuSupport();

  /*!
   \brief Get the main playlist named in the disc's disc.inf, reading it only once per disc.
   \return the playlist number, or -1 if the disc names none
   */
  int GetMainPlaylist();

  void Dispose();
  std::string GetDiscInfoString(DiscInfo info);
  const BLURAY_DISC_INFO* GetDiscInfo() const;

  CURL m_url;
  std::string m_realPath;
  BLURAY* m_bd{nullptr};
  bool m_blurayInitialized{false};

#if defined(HAS_UDFREAD)
  //! Keeps a disc image's UDF volume mounted for as long as this disc is in use
  std::optional<CUDFMount> m_udfMount;
#endif

  std::map<unsigned int, ClipInformation> m_clipCache;
};
} // namespace XFILE
