/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "JSONRPCUtils.h"
#include "playlists/PlayListTypes.h"

#include <optional>

class CVariant;

namespace JSONRPC
{

/*! \brief Translate the "shuffle" parameter, a state or "toggle", into the state it asks for.
 \param shuffle the parameter as given by the client
 \param current whether the target is shuffled at the moment
 \return the requested state, or nothing when that is the current state already
 */
std::optional<bool> ParseShuffleState(const CVariant& shuffle, bool current);

/*! \brief Translate the "repeat" parameter into the state it names.
 \param repeat the parameter as given by the client
 \return the named state, or RepeatState::NONE for anything else
 */
KODI::PLAYLIST::RepeatState ParseRepeatState(const CVariant& repeat);

/*! \brief Translate the "repeat" parameter into the state it asks for.
 Also accepts "cycle", which steps none -> all -> one -> none from the current state.
 \param repeat the parameter as given by the client
 \param current the state of the target at the moment
 \return the requested state
 */
KODI::PLAYLIST::RepeatState ParseRepeatState(const CVariant& repeat,
                                             KODI::PLAYLIST::RepeatState current);

/*! \brief Apply the "shuffle" parameter to a playlist.
 \param playlistId the playlist
 \param shuffle the parameter as given by the client
 */
void ApplyShuffle(KODI::PLAYLIST::Id playlistId, const CVariant& shuffle);

/*! \brief Apply the "repeat" parameter to a playlist.
 \param playlistId the playlist
 \param repeat the parameter as given by the client
 */
void ApplyRepeat(KODI::PLAYLIST::Id playlistId, const CVariant& repeat);

/*! \brief Apply the "shuffle" parameter to the running slideshow, which cannot be unshuffled.
 \param shuffle the parameter as given by the client
 \return FailedToExecute when asked to unshuffle it
 */
JSONRPC_STATUS ShuffleSlideshow(const CVariant& shuffle);

} // namespace JSONRPC
