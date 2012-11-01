#ifndef _CCDDARIPPER_H
#define _CCDDARIPPER_H

/*
 *      Copyright (C) 2005-2012 Team XBMC
 *      http://www.xbmc.org
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

#include "Encoder.h"
#include "utils/JobManager.h"

class CFileItem;

namespace MUSIC_INFO
{
  class CMusicInfoTag;
}

/*! \brief Rip an entire CD or a single track

 The CCDDARipper class is used to rip an entire CD or just a single track.
 Tracks are stored in a folder constructed from two user settings: audiocds.recordingpath and 
 audiocds.trackpathformat. The former is the absolute file system path for the root folder 
 where ripped music is stored, and the latter specifies the format for the album subfolder and 
 for the track file name.
 Format used to encode ripped tracks is defined by the audiocds.encoder user setting, and 
 there are several choices: wav, ogg vorbis and mp3.
 */
class CCDDARipper : public CJobQueue
{
public:
  /*!
   \brief The only way through which the global instance of the CDDARipper should be accessed.
   \return the global instance.
   */
  static CCDDARipper& GetInstance();

  /*! \brief Rip a single track
   \param[in] pItem CFileItem representing a track to rip
   \return true if success, false if failure
   */
  bool RipTrack(CFileItem* pItem);

  /*! \brief Rip an entire CD
   \return true if success, false if failure
   */
  bool RipCD();

  virtual void OnJobComplete(unsigned int jobID, bool success, CJob* job);

private:
  // private construction and no assignments
  CCDDARipper();
  CCDDARipper(const CCDDARipper&);
  virtual ~CCDDARipper();
  CCDDARipper const& operator=(CCDDARipper const&);
  
  /*! \brief Return track file name extension for the given encoder type
   \param[in] iEncoder encoder type (see CDDARIP_ENCODER_... constants)
   \return file extension string (i.e. ".wav", ".mp3", ...)
   */
  const char* GetExtension(int iEncoder);

  /*! \brief Create folder where CD tracks will be stored
   \param[in]  infoTag music info tags for the CD, used to format album name
   \param[out] strDirectory full path of the created folder
   \param[out] legalType created directory type (see LEGAL_... constants)
   \return true if success, false if failure
   */
  bool CreateAlbumDir(const MUSIC_INFO::CMusicInfoTag& infoTag, CStdString& strDirectory, int& legalType);

  /*! \brief Return formatted album subfolder for rip path
   \param infoTag music info tags for the CD, used to format album name
   \return album subfolder path name
   */
  CStdString GetAlbumDirName(const MUSIC_INFO::CMusicInfoTag& infoTag);

  /*! \brief Return file name for the track
   \param item CFileItem representing a track
   \return track file name
   */
  CStdString GetTrackName(CFileItem *item);
};

#endif // _CCDDARIPPERMP3_H
