#pragma once

/*
 *      Copyright (C) 2016 Team Kodi
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
 *  along with Kodi; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include <string>

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
  virtual ~CPlatform() = default;
  
  /**\brief Called at an early stage of application startup
   *
   * This method can be used to do platform specific environment preparation
   * or initialisation (like setting environment variables for example)
   */
  virtual void Init();
  

  /**\brief Geter for the m_uuid member
   *
   *\return the m_uuid member
   */
  std::string GetUniqueHardwareIdentifier();

  /**\brief Called for initing the m_uuid member.
   *
   * This method should set m_uuid to a string which identifies the hardware we
   * are running on. The underlaying source for this information could
   * be a network mac address, hw serial number or other uuid that is provided
   * by the underlaying operating system.
   * The best identifiers are those that never change (not even during operating system upgrades).
   * m_uuid should be filled the the MD5 Hash of the unique identifier!
   */
  virtual void InitUniqueHardwareIdentifier();
  
  // constants
  static const std::string NoValidUUID;
  
  protected:
    std::string m_uuid;
  
};
