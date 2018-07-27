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
  CMediaLibraryEvent(const MediaType& mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, EventLevel level = EventLevel::Information);
  CMediaLibraryEvent(const MediaType& mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, const std::string& icon, EventLevel level = EventLevel::Information);
  CMediaLibraryEvent(const MediaType& mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, EventLevel level = EventLevel::Information);
  CMediaLibraryEvent(const MediaType& mediaType, const std::string& mediaPath, const CVariant& label, const CVariant& description, const std::string& icon, const CVariant& details, const CVariant& executionLabel, EventLevel level = EventLevel::Information);
  ~CMediaLibraryEvent() override = default;

  const char* GetType() const override { return "MediaLibraryEvent"; }
  std::string GetExecutionLabel() const override;

  bool CanExecute() const override { return !m_mediaType.empty(); }
  bool Execute() const override;

protected:
  MediaType m_mediaType;
  std::string m_mediaPath;
};
