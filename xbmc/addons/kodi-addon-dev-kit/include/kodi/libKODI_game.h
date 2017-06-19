/*
 *      Copyright (C) 2014-2017 Team Kodi
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

#include "libXBMC_addon.h"
#include "kodi_game_types.h"

#include <string>
#include <stdio.h>

#if defined(ANDROID)
  #include <sys/stat.h>
#endif

class CHelper_libKODI_game
{
public:
  CHelper_libKODI_game(void) :
    m_handle(nullptr),
    m_callbacks(nullptr)
  {
  }

  ~CHelper_libKODI_game(void)
  {
  }

  /*!
    * @brief Resolve all callback methods
    * @param handle Pointer to the add-on
    * @return True when all methods were resolved, false otherwise.
    */
  bool RegisterMe(void* handle)
  {
    m_handle = static_cast<AddonCB*>(handle);
    if (m_handle)
      m_callbacks = (AddonInstance_Game*)m_handle->GameLib_RegisterMe(m_handle->addonData);
    if (!m_callbacks)
      fprintf(stderr, "libKODI_game-ERROR: GameLib_RegisterMe can't get callback table from Kodi !!!\n");

    return m_callbacks != nullptr;
  }

  // --- Game callbacks --------------------------------------------------------

  /*!
   * \brief Requests the frontend to stop the current game
   */
  void CloseGame(void)
  {
    return m_callbacks->toKodi.CloseGame(m_callbacks->toKodi.kodiInstance);
  }

  /*!
   * \brief Create a video stream for pixel data
   *
   * \param format The type of pixel data accepted by this stream
   * \param width The frame width
   * \param height The frame height
   * \param rotation The rotation (counter-clockwise) of the video frames
   *
   * \return 0 on success or -1 if a video stream is already created
   */
  bool OpenPixelStream(GAME_PIXEL_FORMAT format, unsigned int width, unsigned int height, GAME_VIDEO_ROTATION rotation)
  {
    return m_callbacks->toKodi.OpenPixelStream(m_callbacks->toKodi.kodiInstance, format, width, height, rotation) == 0;
  }

  /*!
   * \brief Create a video stream for encoded video data
   *
   * \param codec The video format accepted by this stream
   *
   * \return 0 on success or -1 if a video stream is already created
   */
  bool OpenVideoStream(GAME_VIDEO_CODEC codec)
  {
    return m_callbacks->toKodi.OpenVideoStream(m_callbacks->toKodi.kodiInstance, codec) == 0;
  }

  /*!
   * \brief Create an audio stream for PCM audio data
   *
   * \param format The type of audio data accepted by this stream
   * \param channel_map The channel layout terminated by GAME_CH_NULL
   *
   * \return 0 on success or -1 if an audio stream is already created
   */
  bool OpenPCMStream(GAME_PCM_FORMAT format, const GAME_AUDIO_CHANNEL* channel_map)
  {
    return m_callbacks->toKodi.OpenPCMStream(m_callbacks->toKodi.kodiInstance, format, channel_map) == 0;
  }

  /*!
  * \brief Create an audio stream for encoded audio data
  *
  * \param codec The audio format accepted by this stream
  * \param channel_map The channel layout terminated by GAME_CH_NULL
  *
  * \return 0 on success or -1 if an audio stream is already created
  */
  bool OpenAudioStream(GAME_AUDIO_CODEC codec, const GAME_AUDIO_CHANNEL* channel_map)
  {
    return m_callbacks->toKodi.OpenAudioStream(m_callbacks->toKodi.kodiInstance, codec, channel_map) == 0;
  }

  /*!
   * \brief Add a data packet to an audio or video stream
   *
   * \param stream The target stream
   * \param data The data packet
   * \param size The size of the data
   */
  void AddStreamData(GAME_STREAM_TYPE stream, const uint8_t* data, unsigned int size)
  {
    m_callbacks->toKodi.AddStreamData(m_callbacks->toKodi.kodiInstance, stream, data, size);
  }

  /*!
   * \brief Free the specified stream
   *
   * \param stream The stream to close
   */
  void CloseStream(GAME_STREAM_TYPE stream)
  {
    m_callbacks->toKodi.CloseStream(m_callbacks->toKodi.kodiInstance, stream);
  }

  // -- Hardware rendering callbacks -------------------------------------------

  /*!
   * \brief Enable hardware rendering
   *
   * \param hw_info A struct of properties for the hardware rendering system
   */
  void EnableHardwareRendering(const struct game_hw_info* hw_info)
  {
    return m_callbacks->toKodi.EnableHardwareRendering(m_callbacks->toKodi.kodiInstance, hw_info);
  }

  /*!
   * \brief Get the framebuffer for rendering
   *
   * \return The framebuffer
   */
  uintptr_t HwGetCurrentFramebuffer(void)
  {
    return m_callbacks->toKodi.HwGetCurrentFramebuffer(m_callbacks->toKodi.kodiInstance);
  }

  /*!
   * \brief Get a symbol from the hardware context
   *
   * \param symbol The symbol's name
   *
   * \return A function pointer for the specified symbol
   */
  game_proc_address_t HwGetProcAddress(const char* sym)
  {
    return m_callbacks->toKodi.HwGetProcAddress(m_callbacks->toKodi.kodiInstance, sym);
  }

  /*!
   * \brief Called when a frame is being rendered
   */
  void RenderFrame()
  {
    return m_callbacks->toKodi.RenderFrame(m_callbacks->toKodi.kodiInstance);
  }

  // --- Input callbacks -------------------------------------------------------

  /*!
   * \brief Begin reporting events for the specified joystick port
   *
   * \param port The zero-indexed port number
   *
   * \return true if the port was opened, false otherwise
   */
  bool OpenPort(unsigned int port)
  {
    return m_callbacks->toKodi.OpenPort(m_callbacks->toKodi.kodiInstance, port);
  }

  /*!
   * \brief End reporting events for the specified port
   *
   * \param port The port number passed to OpenPort()
   */
  void ClosePort(unsigned int port)
  {
    return m_callbacks->toKodi.ClosePort(m_callbacks->toKodi.kodiInstance, port);
  }

  /*!
  * \brief Notify the port of an input event
  *
  * \param event The input event
  *
  * Input events can arrive for the following sources:
  *   - GAME_INPUT_EVENT_MOTOR
  *
  * \return true if the event was handled, false otherwise
  */
  bool InputEvent(const game_input_event& event)
  {
    return m_callbacks->toKodi.InputEvent(m_callbacks->toKodi.kodiInstance, &event);
  }

private:
  AddonCB* m_handle;
  AddonInstance_Game* m_callbacks;
};
