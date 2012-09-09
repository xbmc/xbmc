#pragma once
/*
*      Copyright (C) 2012 Team XBMC
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

#include "utils/Job.h"
#include "utils/StdString.h"
#include "music/tags/MusicInfoTag.h"

class CEncoder;

namespace XFILE
{
class CFile;
}

class CCDDARipJob : public CJob
{
public:
  //! \brief Construct a ripper job
  //! \param input The input file url
  //! \param output The output file url
  //! \param tag The music tag to attach to track
  //! \param encoder The encoder to use. See GUISettings.h
  //! \param eject Should we eject tray on finish?
  //! \param rate The sample rate of the input
  //! \param channels Number of audio channels in input
  //! \param bps The bits per sample for input
  CCDDARipJob(const CStdString& input, const CStdString& output,
              const MUSIC_INFO::CMusicInfoTag& tag, int encoder,
              bool eject=false, unsigned int rate=44100,
              unsigned int channels=2, unsigned int bps=16);

  virtual ~CCDDARipJob();

  virtual const char* GetType() const { return "cdrip"; };
  virtual bool operator==(const CJob *job) const;
  virtual bool DoWork();
protected:
  //! \brief Setup the audio encoder
  CEncoder* SetupEncoder(XFILE::CFile& reader);

  //! \brief Helper used if output is a remote url
  CStdString SetupTempFile();

  //! \brief Rip a chunk of audio
  //! \param reader The input reader
  //! \param encoder The audio encoder
  //! \param percent The percentage completed on return
  //! \return 0 (CDDARIP_OK) if everything went okay, or
  //!         a positive error code from the reader, or
  //!         -1 if the encoder failed
  //! \sa CCDDARipper::GetData, CEncoder::Encode
  int RipChunk(XFILE::CFile& reader, CEncoder* encoder, int& percent);

  unsigned int m_rate; //< The sample rate of the input file 
  unsigned int m_channels; //< The number of channels in input file
  unsigned int m_bps; //< The bits per sample of input
  MUSIC_INFO::CMusicInfoTag m_tag; //< Music tag to attach to output file
  CStdString m_input; //< The input url
  CStdString m_output; //< The output url
  bool m_eject; //< Should we eject tray when we are finished?
  int m_encoder; //< The audio encoder
};

