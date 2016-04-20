#pragma once
/*
 *      Copyright (C) 2015 Team KODI
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
 *  along with KODI; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "../definitions.hpp"
#include "../.internal/AddonLib_internal.hpp"

API_NAMESPACE

namespace KodiAPI
{
namespace AddOn
{

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_AddOn_SoundPlay Sound Playback (class CSoundPlay)
  /// \ingroup CPP_KodiAPI_AddOn
  /// @{
  /// @brief <b>Background sound playback</b>
  ///
  /// This class is designed for playback of "wave" audio files to point for the
  /// user with sound steps performed or to draw attention to warnings.
  ///
  /// Further allowed this playback on a single selected channel which can be
  /// used to identify or also to test. For larger and more extensive audio
  /// system testing is recommended but the use of "CAddonAE_Stream" since
  /// this is intended specifically for this purpose.
  ///
  /// @warning class supports only <b>wave files (*.wav)</b> for play back!
  ///
  /// It has the header \ref SoundPlay.hpp "#include <kodi/addon.api2/SoundPlay.hpp>" be included
  /// to enjoy it.
  ///
  ///
  /// ------------------------------------------------------------------------
  ///
  /// **Example:**
  /// ~~~~~~~~~~~~~{.cpp}
  /// #include <kodi/api2/addon/SoundPlay.hpp>
  /// #include <kodi/api2/gui/DialogFileBrowser.h>
  ///
  /// ...
  ///
  /// void PlaySelectedWaveFile()
  /// {
  ///   std::string filename;
  ///   if (KodiAPI::AddOn::GUI::DialogFileBrowser::ShowAndGetFile("local", "*.wav",
  ///                                                  "Test File selection and playback of them",
  ///                                                  filename))
  ///   {
  ///     //
  ///     // The example performs a playback with the value "waitUntilEnd" from
  ///     // "KodiAPI::AddOn::SoundPlay::Play(...)" is set to true. This waits for the call
  ///     // until the playback is complete, so that it is then also possible to
  ///     // use the class static as in this example.
  ///     //
  ///     KodiAPI::AddOn::CSoundPlay playback(filename);
  ///     playback->Play();
  ///   }
  /// }
  /// ~~~~~~~~~~~~~
  ///

  //============================================================================
  ///
  /// \defgroup CPP_KodiAPI_AddOn_SoundPlay_Defs Definitions, structures and enumerators
  /// \ingroup CPP_KodiAPI_AddOn_SoundPlay
  /// @brief <b>Library definition values</b>
  ///

  class CSoundPlay
  {
  public:
    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_SoundPlay
    /// @brief Construct class
    ///
    /// Construct the class for background playback of given wave file.
    ///
    /// @param[in] filename The wave file to play
    ///
    CSoundPlay(const std::string& filename);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_SoundPlay
    /// @brief Destructor
    ///
    virtual ~CSoundPlay(void);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_SoundPlay
    /// @brief Start playback
    ///
    /// Starts the playback of wave file. Normally returns this function.
    ///
    /// This function ordinarily comes after him begin the sound reproduction
    /// immediately again back around other steps to permit.
    ///
    void Play();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_SoundPlay
    /// @brief Stop playback
    ///
    /// Stops the playback of wave file
    ///
    void Stop();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_SoundPlay
    /// @brief Set audio channel for playback
    ///
    /// To set channel position the for a single channel playback of wave file.
    /// To reset selection use "AUDIO_CH_INVALID".
    ///
    /// @param[in] channel The channel identifier to play on
    ///
    void SetChannel(audio_channel channel);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_SoundPlay
    /// @brief Get the active channel for playback
    ///
    /// To set channel position the for a single channel playback of wave file.
    ///
    /// @return The channel where currently played
    ///
    audio_channel GetChannel();
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_SoundPlay
    /// @brief Set the volume
    ///
    /// To set the volume of wave file playback.
    ///
    /// @note Volume is independent from Kodi's normal volume control and only
    /// for the wave file. Also is the volume only on this class and not to all
    /// background sounds on Kodi.
    ///
    /// @param[in] volume The playback volume of wave file
    ///
    void SetVolume(float volume);
    //----------------------------------------------------------------------------


    //============================================================================
    ///
    /// @ingroup CPP_KodiAPI_AddOn_SoundPlay
    /// @brief Get the volume
    ///
    /// To get the volume of wave file playback.
    ///
    /// @note Volume is independent from Kodi's normal volume control and only
    /// for the wave file. Also is the volume only on this class and not to all
    /// background sounds on Kodi.
    ///
    /// @return The currently active volume of this wave file
    ///
    float GetVolume();
    //----------------------------------------------------------------------------

  #ifndef DOXYGEN_SHOULD_SKIP_THIS
    IMPL_ADDONSOUNDPLAY;
  #endif
  };
  //@}

} /* namespace AddOn */
} /* namespace KodiAPI */

END_NAMESPACE()
