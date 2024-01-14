/*
 *  Copyright (C) 2018-2024 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "InputTranslator.h"

using namespace KODI;
using namespace INPUT;

#define TAN_1_8_PI 0.4142136f // tan(1/8*PI)
#define TAN_3_8_PI 2.4142136f // tan(3/8*PI)

CARDINAL_DIRECTION CInputTranslator::VectorToCardinalDirection(float x, float y)
{
  if (y >= -x && y > x)
    return CARDINAL_DIRECTION::UP;
  else if (y <= x && y > -x)
    return CARDINAL_DIRECTION::RIGHT;
  else if (y <= -x && y < x)
    return CARDINAL_DIRECTION::DOWN;
  else if (y >= x && y < -x)
    return CARDINAL_DIRECTION::LEFT;

  return CARDINAL_DIRECTION::NONE;
}

INTERCARDINAL_DIRECTION CInputTranslator::VectorToIntercardinalDirection(float x, float y)
{
  if (y >= TAN_3_8_PI * -x && y > TAN_3_8_PI * x)
    return INTERCARDINAL_DIRECTION::UP;
  else if (y <= TAN_3_8_PI * x && y > TAN_1_8_PI * x)
    return INTERCARDINAL_DIRECTION::RIGHTUP;
  else if (y <= TAN_1_8_PI * x && y > TAN_1_8_PI * -x)
    return INTERCARDINAL_DIRECTION::RIGHT;
  else if (y <= TAN_1_8_PI * -x && y > TAN_3_8_PI * -x)
    return INTERCARDINAL_DIRECTION::RIGHTDOWN;
  else if (y <= TAN_3_8_PI * -x && y < TAN_3_8_PI * x)
    return INTERCARDINAL_DIRECTION::DOWN;
  else if (y >= TAN_3_8_PI * x && y < TAN_1_8_PI * x)
    return INTERCARDINAL_DIRECTION::LEFTDOWN;
  else if (y >= TAN_1_8_PI * x && y < TAN_1_8_PI * -x)
    return INTERCARDINAL_DIRECTION::LEFT;
  else if (y >= TAN_1_8_PI * -x && y < TAN_3_8_PI * -x)
    return INTERCARDINAL_DIRECTION::LEFTUP;

  return INTERCARDINAL_DIRECTION::NONE;
}
