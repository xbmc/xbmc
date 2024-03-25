/*
 *  Copyright (C) 2023 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

struct IPTCInfo
{
  IPTCInfo() = default;
  IPTCInfo(const IPTCInfo&) = default;
  IPTCInfo(IPTCInfo&&) = default;

  IPTCInfo& operator=(const IPTCInfo&) = default;
  IPTCInfo& operator=(IPTCInfo&&) = default;

  std::string RecordVersion;
  std::string SupplementalCategories;
  std::string Keywords;
  std::string Caption;
  std::string Author;
  std::string Headline;
  std::string SpecialInstructions;
  std::string Category;
  std::string Byline;
  std::string BylineTitle;
  std::string Credit;
  std::string Source;
  std::string CopyrightNotice;
  std::string ObjectName;
  std::string City;
  std::string State;
  std::string Country;
  std::string TransmissionReference;
  std::string Date;
  std::string Urgency;
  std::string ReferenceService;
  std::string CountryCode;
  std::string TimeCreated;
  std::string SubLocation;
  std::string ImageType;
};
