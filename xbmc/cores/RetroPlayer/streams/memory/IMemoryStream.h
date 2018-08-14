/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

namespace KODI
{
namespace RETRO
{
  /*!
   * \brief Stream of serialized states from game clients
   *
   * A memory stream is composed of "frames" of memory representing serialized
   * states of the game client. For each video frame run by the game loop, the
   * game client's state is serialized into a buffer provided by this interface.
   *
   * Implementation of three types of memory streams are provided:
   *
   *   - Basic memory stream: has only a current frame, and supports neither
   *         rewind nor forward seeking.
   *
   *         \sa CBasicMemoryStream
   *
   *   - Linear memory stream: can grow in one direction. It is possible to
   *         rewind, but not fast-forward.
   *
   *         \sa CLinearMemoryStream
   *
   *   - Nonlinear memory stream: can have frames both ahead of and behind
   *         the current frame. If a stream is rewound, it is possible to
   *         recover these frames by seeking forward again.
   *
   *         \sa CNonlinearMemoryStream (TODO)
   */
  class IMemoryStream
  {
  public:
    virtual ~IMemoryStream() = default;

    /*!
     * \brief Initialize memory stream
     *
     * \param frameSize The size of the serialized memory state
     * \param maxFrameCount The maximum number of frames this steam can hold
     */
    virtual void Init(size_t frameSize, uint64_t maxFrameCount) = 0;

    /*!
     * \brief Free any resources used by this stream
     */
    virtual void Reset() = 0;

    /*!
     * \brief Return the frame size passed to Init()
     */
    virtual size_t FrameSize() const = 0;

    /*!
     * \brief Return the current max frame count
     */
    virtual uint64_t MaxFrameCount() const = 0;

    /*!
     * \brief Update the max frame count
     *
     * Old frames may be deleted if the max frame count is reduced.
     */
    virtual void SetMaxFrameCount(uint64_t maxFrameCount) = 0;

    /*!
     * \ brief Get a pointer to which FrameSize() bytes can be written
     *
     * The buffer exposed by this function is passed to the game client, which
     * fills it with a serialization of its current state.
     */
    virtual uint8_t* BeginFrame() = 0;

    /*!
     * \brief Indicate that a frame of size FrameSize() has been written to the
     *        location returned from BeginFrame()
     */
    virtual void SubmitFrame() = 0;

    /*!
     * \brief Get a pointer to the current frame
     *
     * This function must have no side effects. The pointer is valid until the
     * stream is modified.
     *
     * \return A buffer of size FrameSize(), or nullptr if the stream is empty
     */
    virtual const uint8_t* CurrentFrame() const = 0;

    /*!
     * \brief Return the number of frames ahead of the current frame
     *
     * If the stream supports forward seeking, frames that are passed over
     * during a "rewind" operation can be recovered again.
     */
    virtual uint64_t FutureFramesAvailable() const = 0;

    /*!
     * \brief Seek ahead the specified number of frames
     *
     * \return The number of frames advanced
     */
    virtual uint64_t AdvanceFrames(uint64_t frameCount) = 0;

    /*!
     * \brief Return the number of frames behind the current frame
     */
    virtual uint64_t PastFramesAvailable() const = 0;

    /*!
     * \brief Seek backwards the specified number of frames
     *
     * \return The number of frames rewound
     */
    virtual uint64_t RewindFrames(uint64_t frameCount) = 0;

    /*!
     * \brief Get the total number of frames played until the current frame
     *
     * \return The history of the current frame, or 0 for unknown
     */
    virtual uint64_t GetFrameCounter() const = 0;

    /*!
     * \brief Set the total number of frames played until the current frame
     *
     * \param frameCount The history of the current frame
     */
    virtual void SetFrameCounter(uint64_t frameCount) = 0;
  };
}
}
