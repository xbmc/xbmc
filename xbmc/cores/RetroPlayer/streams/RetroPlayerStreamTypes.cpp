/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "RetroPlayerStreamTypes.h"

#include "IRetroPlayerStream.h"

using namespace KODI;
using namespace RETRO;

void DeleteStream::operator()(IRetroPlayerStream* stream)
{
  delete stream;
}
