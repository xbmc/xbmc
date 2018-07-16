/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CDVDStateSerializer
{
public:
  static bool DVDToXMLState( std::string &xmlstate, const dvd_state_t *state );
  static bool XMLToDVDState( dvd_state_t *state, const std::string &xmlstate );

  static bool test( const dvd_state_t *state );
};

