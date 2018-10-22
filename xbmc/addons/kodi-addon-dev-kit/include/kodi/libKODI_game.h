/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
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
    m_callbacks->toKodi.CloseGame(m_callbacks->toKodi.kodiInstance);
  }

  /*!
   * \brief Create a stream for gameplay data
   *
   * \param properties The stream properties
   *
   * \return A stream handle, or NULL on failure
   */
  void* OpenStream(const game_stream_properties &properties)
  {
    return m_callbacks->toKodi.OpenStream(m_callbacks->toKodi.kodiInstance, &properties);
  }

  /*!
   * \brief Get a buffer for zero-copy stream data
   *
   * \param stream The stream handle
   * \param width The framebuffer width, or 0 for no width specified
   * \param height The framebuffer height, or 0 for no height specified
   * \param[out] buffer The buffer, or unmodified if false is returned
   *
   * If this returns true, buffer must be freed using ReleaseStreamBuffer().
   *
   * \return True if buffer was set, false otherwise
   */
  bool GetStreamBuffer(void *stream, unsigned int width, unsigned int height, game_stream_buffer &buffer)
  {
    return m_callbacks->toKodi.GetStreamBuffer(m_callbacks->toKodi.kodiInstance, stream, width, height, &buffer);
  }

  /*!
   * \brief Add a data packet to a stream
   *
   * \param stream The target stream
   * \param packet The data packet
   */
  void AddStreamData(void *stream, const game_stream_packet &packet)
  {
    m_callbacks->toKodi.AddStreamData(m_callbacks->toKodi.kodiInstance, stream, &packet);
  }

  /*!
   * \brief Free an allocated buffer
   *
   * \param stream The stream handle
   * \param buffer The buffer returned from GetStreamBuffer()
   */
  void ReleaseStreamBuffer(void *stream, game_stream_buffer &buffer)
  {
    m_callbacks->toKodi.ReleaseStreamBuffer(m_callbacks->toKodi.kodiInstance, stream, &buffer);
  }

  /*!
   * \brief Free the specified stream
   *
   * \param stream The stream to close
   */
  void CloseStream(void *stream)
  {
    m_callbacks->toKodi.CloseStream(m_callbacks->toKodi.kodiInstance, stream);
  }

  // -- Hardware rendering callbacks -------------------------------------------

  /*!
   * \brief Get a symbol from the hardware context
   *
   * \param sym The symbol's name
   *
   * \return A function pointer for the specified symbol
   */
  game_proc_address_t HwGetProcAddress(const char* sym)
  {
    return m_callbacks->toKodi.HwGetProcAddress(m_callbacks->toKodi.kodiInstance, sym);
  }

  // --- Input callbacks -------------------------------------------------------

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
