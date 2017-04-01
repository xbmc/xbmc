/*
 *      Copyright (C) 2016-2017 Team Kodi
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
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */
#pragma once

namespace GAME
{
  class IGameAudioCallback;

  /*!
   * \ingroup games
   * \brief Class to normalize audio and video timing to avoid audio resampling
   *
   * For example, assume the audio callback supports two sample rates:
   * 32,000 Hz and 44,100 Hz.
   *
   * If the game client reports an audio sample rate of 32040.5, the audio
   * callback will normalize this to 32,000 Hz. The correction factor is then
   * set to (32000 / 32040.5) = 0.9987.
   *
   * After normalization, GetSampleRate() will report (32040.5 * 0.9987) = 32000.
   *
   * If the game client's frame rate is 60.1 fps, after normalization
   * GetFrameRate() will report (60.1 * 0.9987) = 60.024 fps. The game
   * client's frame rate has been slowed slightly to avoid resampling audio.
   *
   * To avoid excessive scaling, normalization will fail if the correction
   * factor exceeds MAX_CORRECTION_FACTOR_PERCENT.
   */
  class CGameClientTiming
  {
  public:
    static const unsigned int MAX_CORRECTION_FACTOR_PERCENT = 7;

    CGameClientTiming() { Reset(); }

    void Reset();

    /*!
     * \brief Calculate normalization factor to avoid audio resampling
     *
     * \param audio Callback capable of normalizing sample rate to one of the
     *              discrete values supported by the audio system
     *
     * \return false If the correction factor exceeds a pre-defined value, true otherwise
     */
    bool NormalizeAudio(IGameAudioCallback* audio);

    // Set frame rate and sample rate reported by the game client
    void SetFrameRate(double framerate) { m_framerate = framerate; }
    void SetSampleRate(double samplerate) { m_samplerate = samplerate; }

    // Get frame rate and sample rate multiplied by the correction factor
    double GetFrameRate() const;
    unsigned int GetSampleRate() const;
    double GetCorrectionFactor() const { return m_audioCorrectionFactor; }

  private:
    double m_framerate;  // Video frame rate (fps)
    double m_samplerate; // Audio sample rate (Hz)
    double m_audioCorrectionFactor; // Factor that audio is normalized by to avoid resampling
  };
}
