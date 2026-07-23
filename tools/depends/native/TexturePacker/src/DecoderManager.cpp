/*
 *  Copyright (C) 2014-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include <cstdio>
#include "DecoderManager.h"

// ADD new decoders here
// include decoders
#include "GIFDecoder.h"
#include "JPGDecoder.h"
#include "KTXDecoder.h"
#include "PNGDecoder.h"

DecoderManager::DecoderManager()
{
  m_decoders.emplace_back(std::make_unique<PNGDecoder>());
  m_decoders.emplace_back(std::make_unique<JPGDecoder>());
  m_decoders.emplace_back(std::make_unique<GIFDecoder>());
  m_decoders.emplace_back(std::make_unique<KTXDecoder>());
}

// returns true for png, bmp, tga, jpg and dds files, otherwise returns false
bool DecoderManager::IsSupportedGraphicsFile(std::string_view filename)
{
  if (filename.length() < 4)
    return false;

  for (const auto& decoder : m_decoders)
  {
    const std::vector<std::string>& extensions = decoder->GetSupportedExtensions();
    for (const auto& extension : extensions)
    {
      if (std::string::npos != filename.rfind(extension, filename.length() - extension.length()))
      {
        return true;
      }
    }
  }
  return false;
}

bool DecoderManager::IsSubstitutionFile(std::string_view filename)
{
  for (const auto& decoder : m_decoders)
  {
    const std::vector<std::pair<std::string, KD_TEX_FMT>>& substitutions =
        decoder->GetSupportedSubstitutions();
    for (const auto& substitution : substitutions)
    {
      if (filename.length() > substitution.first.length() &&
          std::string::npos !=
              filename.rfind(substitution.first, filename.length() - substitution.first.length()))
      {
        return true;
      }
    }
  }
  return false;
}

void DecoderManager::SubstitudeFileName(KD_TEX_FMT& family, std::string& filename)
{
  for (const auto& decoder : m_decoders)
  {
    const std::vector<std::pair<std::string, KD_TEX_FMT>>& substitutions =
        decoder->GetSupportedSubstitutions();
    for (const auto& substitution : substitutions)
    {
      if (filename.length() > substitution.first.length() &&
          std::string::npos !=
              filename.rfind(substitution.first, filename.length() - substitution.first.length()))
      {
        filename.resize(filename.length() - substitution.first.length());
        family = substitution.second;
        return;
      }
    }
  }
}

bool DecoderManager::LoadFile(const std::string &filename, DecodedFrames &frames)
{
  for (const auto& decoder : m_decoders)
  {
    if (decoder->CanDecode(filename))
    {
      if (verbose)
        fprintf(stdout, "This is a %s - lets load it via %s...\n", decoder->GetImageFormatName(),
                decoder->GetDecoderName());
      return decoder->LoadFile(filename, frames);
    }
  }
  return false;
}
