/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <memory>

class CDVDDemux;
class CDVDInputStream;

class CDVDFactoryDemuxer
{
public:
  static CDVDDemux* CreateDemuxer(const std::shared_ptr<CDVDInputStream>& pInputStream,
                                  bool fileinfo = false);
};
