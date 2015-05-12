#pragma once
/*
 *      Copyright (C) 2015 Team Kodi
 *      http://kodi.tv
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
