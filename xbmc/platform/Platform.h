/*
 *  Copyright (C) 2016-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

/**\brief Class for the Platform object
 *
 * Contains method which retrieve platform specific information
 * and methods for doing platform specific environment preparation/initialisation
 */
class CPlatform
{
public:
  /**\brief Creates the Platform object
   *
   *@return the platform object
  */
  static CPlatform *CreateInstance();

  /**\brief C'tor */
  CPlatform();

  /**\brief D'tor */
  virtual ~CPlatform();

  /**\brief Called at an early stage of application startup
   *
   * This method can be used to do platform specific environment preparation
   * or initialisation (like setting environment variables for example)
   */
  virtual bool Init();
};
