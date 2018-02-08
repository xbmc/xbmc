/*
 *      Copyright (C) 2018 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "InputTranslator.h"

using namespace KODI;
using namespace INPUT;

#define TAN_1_8_PI  0.4142136f // tan(1/8*PI)
#define TAN_3_8_PI  2.4142136f // tan(3/8*PI)

CARDINAL_DIRECTION CInputTranslator::VectorToCardinalDirection(float x, float y)
{
  if      (y >= -x && y >  x) return CARDINAL_DIRECTION::UP;
  else if (y <=  x && y > -x) return CARDINAL_DIRECTION::RIGHT;
  else if (y <= -x && y <  x) return CARDINAL_DIRECTION::DOWN;
  else if (y >=  x && y < -x) return CARDINAL_DIRECTION::LEFT;

  return CARDINAL_DIRECTION::NONE;
}

INTERCARDINAL_DIRECTION CInputTranslator::VectorToIntercardinalDirection(float x, float y)
{
  if      (y >= TAN_3_8_PI * -x && y > TAN_3_8_PI *  x) return INTERCARDINAL_DIRECTION::UP;
  else if (y <= TAN_3_8_PI *  x && y > TAN_1_8_PI *  x) return INTERCARDINAL_DIRECTION::RIGHTUP;
  else if (y <= TAN_1_8_PI *  x && y > TAN_1_8_PI * -x) return INTERCARDINAL_DIRECTION::RIGHT;
  else if (y <= TAN_1_8_PI * -x && y > TAN_3_8_PI * -x) return INTERCARDINAL_DIRECTION::RIGHTDOWN;
  else if (y <= TAN_3_8_PI * -x && y < TAN_3_8_PI *  x) return INTERCARDINAL_DIRECTION::DOWN;
  else if (y >= TAN_3_8_PI *  x && y < TAN_1_8_PI *  x) return INTERCARDINAL_DIRECTION::LEFTDOWN;
  else if (y >= TAN_1_8_PI *  x && y < TAN_1_8_PI * -x) return INTERCARDINAL_DIRECTION::LEFT;
  else if (y >= TAN_1_8_PI * -x && y < TAN_3_8_PI * -x) return INTERCARDINAL_DIRECTION::LEFTUP;

  return INTERCARDINAL_DIRECTION::NONE;
}
