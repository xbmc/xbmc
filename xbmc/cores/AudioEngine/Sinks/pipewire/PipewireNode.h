/*
 *  Copyright (C) 2010-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "PipewireProxy.h"

#include <memory>
#include <set>

#include <pipewire/node.h>
#include <spa/param/audio/iec958.h>
#include <spa/param/audio/raw.h>

namespace KODI
{
namespace PIPEWIRE
{

class CPipewire;
class CPipewireRegistry;

class CPipewireNode : public CPipewireProxy
{
public:
  explicit CPipewireNode(CPipewireRegistry& registry, uint32_t id, const char* type);
  CPipewireNode() = delete;
  ~CPipewireNode() override;

  void EnumerateFormats();

  pw_node_info* GetInfo() { return m_info.get(); }

  std::set<spa_audio_format>& GetFormats() { return m_formats; }
  std::set<spa_audio_channel>& GetChannels() { return m_channels; }
  std::set<uint32_t>& GetRates() { return m_rates; }
  std::set<spa_audio_iec958_codec>& GetIEC958Codecs() { return m_iec958Codecs; }

private:
  void Parse(uint32_t type, void* body, uint32_t size);

  static void Info(void* userdata, const struct pw_node_info* info);
  static void Param(void* userdata,
                    int seq,
                    uint32_t id,
                    uint32_t index,
                    uint32_t next,
                    const struct spa_pod* param);

  static pw_node_events CreateNodeEvents();

  const pw_node_events m_nodeEvents;

  spa_hook m_objectListener;

  struct PipewireNodeInfoDeleter
  {
    void operator()(pw_node_info* p) { pw_node_info_free(p); }
  };

  std::unique_ptr<pw_node_info, PipewireNodeInfoDeleter> m_info;

  std::set<spa_audio_format> m_formats;
  std::set<spa_audio_channel> m_channels;
  std::set<uint32_t> m_rates;
  std::set<spa_audio_iec958_codec> m_iec958Codecs;
};

} // namespace PIPEWIRE
} // namespace KODI
