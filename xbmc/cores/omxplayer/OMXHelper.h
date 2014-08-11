/*
 *      Copyright (C) 2014 Team XBMC
 *      http://xbmc.org
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

#pragma once

#include "system.h"

bool OMXPlayerUnsuitable(bool m_HasVideo, bool m_HasAudio, CDVDDemux* m_pDemuxer, CDVDInputStream* m_pInputStream, CSelectionStreams &m_SelectionStreams);
bool OMXDoProcessing(struct SOmxPlayerState &m_OmxPlayerState, int m_playSpeed, IDVDStreamPlayerVideo *m_dvdPlayerVideo, IDVDStreamPlayerAudio *m_dvdPlayerAudio,
                     CCurrentStream m_CurrentAudio, CCurrentStream m_CurrentVideo, bool m_HasVideo, bool m_HasAudio);
bool OMXStillPlaying(bool waitVideo, bool waitAudio, bool eosVideo, bool eosAudio);
