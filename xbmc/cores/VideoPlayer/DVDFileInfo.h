/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

class CFileItem;
class CDVDDemux;
class CStreamDetails;
class CStreamDetailSubtitle;
class CDVDInputStream;
class CTexture;
class CTextureDetails;

class CDVDFileInfo
{
public:
  static std::unique_ptr<CTexture> ExtractThumbToTexture(const CFileItem& fileItem,
                                                         int chapterNumber = 0);

  /*!
   * @brief Can a thumbnail image and file stream details be extracted from this file item?
  */
  static bool CanExtract(const CFileItem& fileItem);

  /*!
   * @brief Is \p path close enough to decode frames from: local, or on the LAN for HTTP and FTP?
  */
  static bool IsExtractableLocation(const std::string& path);

  /*!
   * @brief Probe the file's streams, look for its external subtitles, and store them in its tag
   * @param listingFlags added to the flags the video's folder is listed with to look for subtitles
   * @param[out] probed if given, the file's own streams, without the external subtitles, which
   * another video of the same file (eg. another episode of it) can be given by AddExternalSubtitles()
   */
  static bool GetFileStreamDetails(CFileItem* pItem,
                                   int listingFlags = 0,
                                   CStreamDetails* probed = nullptr);

  /*!
   * @brief Store streams probed already in an item's tag, with the item's own external subtitles
   * @param streams the streams of the item's file, as GetFileStreamDetails() probed them
   */
  static void AddExternalSubtitles(CFileItem& item, CStreamDetails streams, int listingFlags = 0);

  static bool GetFileDuration(const std::string& path, int& duration);

private:
  static bool DemuxerToStreamDetails(const std::shared_ptr<CDVDInputStream>& pInputStream,
                                     CDVDDemux* pDemux,
                                     CStreamDetails& details,
                                     const std::string& path = "");

  /** \brief Probe the file's internal and external streams and store the info in the StreamDetails parameter.
  *   \param[out] details The file's StreamDetails consisting of internal streams and external subtitle streams.
  */
  static bool DemuxerToStreamDetails(const std::shared_ptr<CDVDInputStream>& pInputStream,
                                     CDVDDemux* pDemuxer,
                                     const std::vector<CStreamDetailSubtitle>& subs,
                                     CStreamDetails& details);

  /** \brief Probe the streams of an external subtitle file and store the info in the StreamDetails parameter.
  *   \param[out] details The external subtitle file's StreamDetails.
  */
  static bool AddExternalSubtitleToDetails(const std::string &path, CStreamDetails &details, const std::string& filename, const std::string& subfilename = "");

  /** \brief Checks external subtitles for a given item and adds any existing ones to the stream details
  *   \param item The video item
  *   \param[out] details The stream details to add the subtitles to
  *   \sa AddExternalSubtitleToDetails
  */
  static void ProcessExternalSubtitles(const CFileItem& item,
                                       CStreamDetails& details,
                                       int listingFlags = 0);
};
