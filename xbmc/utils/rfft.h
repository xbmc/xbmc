/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "contrib/kissfft/kiss_fftr.h"

#include <vector>

//! \brief Class performing a RFFT of interleaved stereo data.
class RFFT
{
public:
  //! \brief The constructor creates a RFFT plan.
  //! \brief size Length of time data for a single channel.
  //! \brief windowed Whether or not to apply a Hann window to data.
  RFFT(int size, bool windowed=false);

  //! \brief Free the RFFT plan
  ~RFFT();

  //! \brief Calculate FFTs
  //! \param input Input data of size 2*m_size
  //! \param output Output data of size m_size.
  void calc(const float* input, float* output);
protected:
  //! \brief Apply a Hann window to a buffer.
  //! \param data Vector with data to apply window to.
  static void hann(std::vector<kiss_fft_scalar>& data);

  size_t m_size;       //!< Size for a single channel.
  bool m_windowed;     //!< Whether or not a Hann window is applied.
  kiss_fftr_cfg m_cfg; //!< FFT plan
};
