/*
 *  Copyright (C) 2005-2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "IDecoder.h"

#include <memory>

namespace heif
{
class Context;
}

class HEIFDecoder : public IDecoder
{
public:
  HEIFDecoder();
  ~HEIFDecoder() override;

  bool CanDecode(const std::string& filename) override;
  bool LoadFile(const std::string& filename, DecodedFrames& frames) override;
  const char* GetImageFormatName() override { return "AVIF"; }
  const char* GetDecoderName() override { return "libheif"; }

private:
  std::unique_ptr<heif::Context> m_ctx;
};
