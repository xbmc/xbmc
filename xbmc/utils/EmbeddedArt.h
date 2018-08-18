/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "IArchivable.h"

class EmbeddedArtInfo : public IArchivable
{
public:
  EmbeddedArtInfo() = default;
  EmbeddedArtInfo(size_t size, const std::string &mime, const std::string& type = "");
  virtual ~EmbeddedArtInfo() = default;

  // implementation of IArchivable
  void Archive(CArchive& ar) override;

  void Set(size_t size, const std::string &mime, const std::string& type = "");
  void Clear();
  bool Empty() const;
  bool Matches(const EmbeddedArtInfo &right) const;
  void SetType(const std::string& type) { m_type = type; }

  size_t m_size = 0;
  std::string m_mime;
  std::string m_type;
};

class EmbeddedArt : public EmbeddedArtInfo
{
public:
  EmbeddedArt() = default;
  EmbeddedArt(const uint8_t *data, size_t size,
              const std::string &mime, const std::string& type = "");

  void Set(const uint8_t *data, size_t size,
           const std::string &mime, const std::string& type = "");

  std::vector<uint8_t> m_data;
};
