/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

bool OMXPlayerUnsuitable(bool m_HasVideo, bool m_HasAudio, CDVDDemux* m_pDemuxer, std::shared_ptr<CDVDInputStream> m_pInputStream, CSelectionStreams &m_SelectionStreams);
bool OMXDoProcessing(struct SOmxPlayerState &m_OmxPlayerState, int m_playSpeed, IDVDStreamPlayerVideo *m_VideoPlayerVideo, IDVDStreamPlayerAudio *m_VideoPlayerAudio,
                     CCurrentStream m_CurrentAudio, CCurrentStream m_CurrentVideo, bool m_HasVideo, bool m_HasAudio, CProcessInfo &processInfo);
bool OMXStillPlaying(bool waitVideo, bool waitAudio, bool eosVideo, bool eosAudio);
