/*
 *  Copyright (C) 2017-2019 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/*!
 * @brief the name of the inputstream add-on that should be used by Kodi to
 * play the stream denoted by STREAM_PROPERTY_STREAMURL. Leave blank to use
 * Kodi's built-in playing capabilities or to allow ffmpeg to handle directly
 * set to STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG.
 */
#define STREAM_PROPERTY_INPUTSTREAM "inputstream"

/*!
 * @brief Identification string for an input stream.
 *
 * This value can be used in addition to @ref STREAM_PROPERTY_INPUTSTREAM. It is
 * used to provide the respective inpustream addon with additional
 * identification.
 *
 * The difference between this and other stream properties is that it is also
 * passed in the associated @ref kodi::addon::CAddonBase::CreateInstance call.
 *
 * This makes it possible to select different processing classes within the
 * associated add-on.
 */
#define STREAM_PROPERTY_INPUTSTREAM_INSTANCE_ID "inputstream-instance-id"

/*!
 * @brief "true" to denote that the stream that should be played is a
 * realtime stream. Any other value indicates that this is not a realtime
 * stream.
 */
#define STREAM_PROPERTY_ISREALTIMESTREAM "isrealtimestream"

/*!
 * @brief special value for STREAM_PROPERTY_INPUTSTREAM to use
 * ffmpeg to directly play a stream URL.
 */
#define STREAM_PROPERTY_VALUE_INPUTSTREAMFFMPEG "inputstream.ffmpeg"

/*!
 * @brief Max number of properties that can be sent to an Inputstream addon
 */
#define STREAM_MAX_PROPERTY_COUNT 30
