#pragma once
/*
 *      Copyright (C) 2010-2013 Team XBMC
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

#include <stdint.h>
#include <vector>
#include <string>

#include "AEChannelData.h"

class CAEChannelInfo {
public:
  CAEChannelInfo();
  CAEChannelInfo(const enum AEChannel* rhs);
  CAEChannelInfo(const enum AEStdChLayout rhs);
  ~CAEChannelInfo();
  CAEChannelInfo& operator=(const CAEChannelInfo& rhs);
  CAEChannelInfo& operator=(const enum AEChannel* rhs);
  CAEChannelInfo& operator=(const enum AEStdChLayout rhs);
  bool operator==(const CAEChannelInfo& rhs) const;
  bool operator!=(const CAEChannelInfo& rhs);
  CAEChannelInfo& operator+=(const enum AEChannel& rhs);
  CAEChannelInfo& operator-=(const enum AEChannel& rhs);
  const enum AEChannel operator[](unsigned int i) const;
  operator std::string() const;

  /* remove any channels that dont exist in the provided info */
  void ResolveChannels(const CAEChannelInfo& rhs);
  void Reset();
  inline unsigned int Count() const { return m_channelCount; }
  static const char* GetChName(const enum AEChannel ch);
  bool HasChannel(const enum AEChannel ch) const;
  bool ContainsChannels(const CAEChannelInfo& rhs) const;
  void ReplaceChannel(const enum AEChannel from, const enum AEChannel to);
  int BestMatch(const std::vector<CAEChannelInfo>& dsts, int* score = NULL) const;
  void AddMissingChannels(const CAEChannelInfo& rhs);

private:
  unsigned int   m_channelCount;
  enum AEChannel m_channels[AE_CH_MAX];
};

