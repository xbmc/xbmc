/*
 *      Copyright (C) 2014-2016 Team Kodi
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
#ifndef KODI_GAME_CALLBACKS_H_
#define KODI_GAME_CALLBACKS_H_

#include "kodi_game_types.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct CB_GameLib
{
  // --- Game callbacks --------------------------------------------------------

  /*!
   * \brief Requests the frontend to stop the current game
   */
  void (*CloseGame)(void* addonData);
  
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
  int (*OpenPixelStream)(void* addonData, GAME_PIXEL_FORMAT format, unsigned int width, unsigned int height, GAME_VIDEO_ROTATION rotation);

  /*!
   * \brief Create a video stream for encoded video data
   *
   * \param codec The video format accepted by this stream
   *
   * \return 0 on success or -1 if a video stream is already created
   */
  int (*OpenVideoStream)(void* addonData, GAME_VIDEO_CODEC codec);

  /*!
   * \brief Create an audio stream for PCM audio data
   *
   * \param format The type of audio data accepted by this stream
   * \param channel_map The channel layout terminated by GAME_CH_NULL
   *
   * \return 0 on success or -1 if an audio stream is already created
   */
  int (*OpenPCMStream)(void* addonData, GAME_PCM_FORMAT format, const GAME_AUDIO_CHANNEL* channel_map);

  /*!
  * \brief Create an audio stream for encoded audio data
  *
  * \param codec The audio format accepted by this stream
  * \param channel_map The channel layout terminated by GAME_CH_NULL
  *
  * \return 0 on success or -1 if an audio stream is already created
  */
  int(*OpenAudioStream)(void* addonData, GAME_AUDIO_CODEC codec, const GAME_AUDIO_CHANNEL* channel_map);

  /*!
   * \brief Add a data packet to an audio or video stream
   *
   * \param stream The target stream
   * \param data The data packet
   * \param size The size of the data
   */
  void (*AddStreamData)(void* addonData, GAME_STREAM_TYPE stream, const uint8_t* data, unsigned int size);

  /*!
   * \brief Free the specified stream
   *
   * \param stream The stream to close
   */
  void (*CloseStream)(void* addonData, GAME_STREAM_TYPE stream);

  // -- Hardware rendering callbacks -------------------------------------------

  /*!
   * \brief Enable hardware rendering
   *
   * \param hw_info A struct of properties for the hardware rendering system
   */
  void (*EnableHardwareRendering)(void* addonData, const game_hw_info* hw_info);

  /*!
   * \brief Get the framebuffer for rendering
   *
   * \return The framebuffer
   */
  uintptr_t (*HwGetCurrentFramebuffer)(void* addonData);

  /*!
   * \brief Get a symbol from the hardware context
   *
   * \param symbol The symbol's name
   *
   * \return A function pointer for the specified symbol
   */
  game_proc_address_t (*HwGetProcAddress)(void* addonData, const char* symbol);

  /*!
   * \brief Called when a frame is being rendered
   */
  void (*RenderFrame)(void* addonData);

  // --- Input callbacks -------------------------------------------------------

  /*!
   * \brief Begin reporting events for the specified joystick port
   *
   * \param port The zero-indexed port number
   *
   * \return true if the port was opened, false otherwise
   */
  bool (*OpenPort)(void* addonData, unsigned int port);

  /*!
   * \brief End reporting events for the specified port
   *
   * \param port The port number passed to OpenPort()
   */
  void (*ClosePort)(void* addonData, unsigned int port);

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
  bool (*InputEvent)(void* addonData, const game_input_event* event);

} CB_GameLib;

#ifdef __cplusplus
}
#endif

#endif // KODI_GAME_CALLBACKS_H_
