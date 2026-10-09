/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "events/UniqueEvent.h"
#include "media/MediaType.h"

class CMediaLibraryEvent : public CUniqueEvent
{
public:
  CMediaLibraryEvent(KODI::MEDIA::TYPE mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, EventLevel level = EventLevel::Information);
  CMediaLibraryEvent(KODI::MEDIA::TYPE mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, EventLevel level = EventLevel::Information);
  ~CMediaLibraryEvent() override = default;

  const char* GetType() const override { return "MediaLibraryEvent"; }
  std::string GetExecutionLabel() const override;

  bool CanExecute() const override { return m_mediaType != KODI::MEDIA::TYPE::NONE; }
  bool Execute() const override;

protected:
  KODI::MEDIA::TYPE m_mediaType;
  std::string m_mediaPath;
};
