/*
 *  Copyright (C) 2010-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <vector>
#include <string>

#include "AEChannelData.h"

class CHelper_libKODI_audioengine;

class CAEChannelInfo {
  friend class CHelper_libKODI_audioengine;

public:
  CAEChannelInfo();
  explicit CAEChannelInfo(const enum AEChannel* rhs);
  CAEChannelInfo(const enum AEStdChLayout rhs);
  ~CAEChannelInfo();
  CAEChannelInfo& operator=(const CAEChannelInfo& rhs);
  CAEChannelInfo& operator=(const enum AEChannel* rhs);
  CAEChannelInfo& operator=(const enum AEStdChLayout rhs);
  bool operator==(const CAEChannelInfo& rhs) const;
  bool operator!=(const CAEChannelInfo& rhs) const;
  CAEChannelInfo& operator+=(const enum AEChannel& rhs);
  CAEChannelInfo& operator-=(const enum AEChannel& rhs);
  enum AEChannel operator[](unsigned int i) const;
  operator std::string() const;

  /* remove any channels that dont exist in the provided info */
  void ResolveChannels(const CAEChannelInfo& rhs);
  void Reset();
  inline unsigned int Count() const { return m_channelCount; }
  static const char* GetChName(const enum AEChannel ch);
  bool HasChannel(const enum AEChannel ch) const;
  bool IsChannelValid(const unsigned int pos);
  bool IsLayoutValid();
  bool ContainsChannels(const CAEChannelInfo& rhs) const;
  void ReplaceChannel(const enum AEChannel from, const enum AEChannel to);
  int BestMatch(const std::vector<CAEChannelInfo>& dsts, int* score = NULL) const;
  void AddMissingChannels(const CAEChannelInfo& rhs);

private:
  unsigned int   m_channelCount;
  enum AEChannel m_channels[AE_CH_MAX];
};

