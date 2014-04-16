/*
 *      Copyright (C) 2005-2013 Team XBMC
 *      http://xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#pragma once

#include <string>
#include <vector>

class CFileItem;
class CDVDDemux;
class CStreamDetails;
class CStreamDetailSubtitle;
class CDVDInputStream;
class CTextureDetails;

class CDVDFileInfo
{
public:
  // Extract a thumbnail immage from the media at strPath, optionally populating a streamdetails class with the data
  static bool ExtractThumb(const std::string &strPath,
                           CTextureDetails &details,
                           CStreamDetails *pStreamDetails, int pos=-1);

  // Probe the files streams and store the info in the VideoInfoTag
  static bool GetFileStreamDetails(CFileItem *pItem);
  static bool DemuxerToStreamDetails(CDVDInputStream* pInputStream, CDVDDemux *pDemux, CStreamDetails &details, const std::string &path = "");

  /** \brief Probe the file's internal and external streams and store the info in the StreamDetails parameter.
  *   \param[out] details The file's StreamDetails consisting of internal streams and external subtitle streams.
  */
  static bool DemuxerToStreamDetails(CDVDInputStream *pInputStream, CDVDDemux *pDemuxer, const std::vector<CStreamDetailSubtitle> &subs, CStreamDetails &details);

  static bool GetFileDuration(const std::string &path, int &duration);

  /** \brief Probe the streams of an external subtitle file and store the info in the StreamDetails parameter.
  *   \param[out] details The external subtitle file's StreamDetails.
  */
  static bool AddExternalSubtitleToDetails(const std::string &path, CStreamDetails &details, const std::string& filename, const std::string& subfilename = "");
};
