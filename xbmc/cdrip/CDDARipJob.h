/*
 *  Copyright (C) 2012-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "music/tags/MusicInfoTag.h"
#include "utils/Job.h"

namespace XFILE
{
class CFile;
}

namespace KODI
{
namespace CDRIP
{

class CEncoder;

class CCDDARipJob : public CJob
{
public:
  /*!
   * \brief Construct a ripper job
   *
   * \param[in] input The input file url
   * \param[in] output The output file url
   * \param[in] tag The music tag to attach to track
   * \param[in] encoder The encoder to use. See Encoder.h
   * \param[in] eject Should we eject tray on finish?
   * \param[in] rate The sample rate of the input
   * \param[in] channels Number of audio channels in input
   * \param[in] bps The bits per sample for input
   */
  CCDDARipJob(const std::string& input,
              const std::string& output,
              const MUSIC_INFO::CMusicInfoTag& tag,
              int encoder,
              bool eject = false,
              unsigned int rate = 44100,
              unsigned int channels = 2,
              unsigned int bps = 16);

  ~CCDDARipJob() override;

  const char* GetType() const override { return "cdrip"; }
  bool operator==(const CJob* job) const override;
  bool DoWork() override;
  std::string GetOutput() const { return m_output; }

protected:
  /*!
   * \brief Setup the audio encoder
   */
  std::unique_ptr<CEncoder> SetupEncoder(XFILE::CFile& reader);

  /*!
   * \brief Helper used if output is a remote url
   */
  std::string SetupTempFile();

  /*!
   * \brief Rip a chunk of audio
   *
   * \param[in] reader The input reader
   * \param[in] encoder The audio encoder
   * \param[out] percent The percentage completed on return
   * \return 0 (CDDARIP_OK) if everything went okay, or
   *         a positive error code from the reader, or
   *         -1 if the encoder failed
   * \sa CCDDARipper::GetData, CEncoder::Encode
   */
  int RipChunk(XFILE::CFile& reader, const std::unique_ptr<CEncoder>& encoder, int& percent);

  unsigned int m_rate; //< The sample rate of the input file
  unsigned int m_channels; //< The number of channels in input file
  unsigned int m_bps; //< The bits per sample of input
  MUSIC_INFO::CMusicInfoTag m_tag; //< Music tag to attach to output file
  std::string m_input; //< The input url
  std::string m_output; //< The output url
  bool m_eject; //< Should we eject tray when we are finished?
  int m_encoder; //< The audio encoder
};

} /* namespace CDRIP */
} /* namespace KODI */
