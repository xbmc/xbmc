/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

enum RenderMethod
{
  RENDER_GLSL = 0x01,
  RENDER_CUSTOM = 0x02
};

enum RenderQuality
{
  RQ_LOW = 1,
  RQ_SINGLEPASS,
  RQ_MULTIPASS,
};

enum FieldType
{
  FIELD_FULL,
  FIELD_TOP,
  FIELD_BOT
};
