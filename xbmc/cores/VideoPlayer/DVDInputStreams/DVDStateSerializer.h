/*
 *  Copyright (C) 2005-2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace tinyxml2
{
class XMLElement;
}

/*! \brief Pod structure which represents the current dvd state with respect to dvdnav properties */
struct DVDState
{
  /*! The current title being played */
  int32_t title = -1;
  /*! Program number */
  int32_t pgn = -1;
  /*! Program cell number */
  int32_t pgcn = -1;
  /*! Current playing angle */
  int32_t current_angle = -1;
  /*! Physical subtitle id set in dvdnav  */
  int8_t subp_num = -1;
  /*! Physical audio stream id set in dvdnav  */
  int8_t audio_num = -1;
  /*! If subtitles are enabled or disabled  */
  bool sub_enabled = false;
};

/*! \brief Auxiliar class to serialize/deserialize the dvd state (into/from XML)
*/
class CDVDStateSerializer
{
public:
  /*! \brief Default constructor */
  CDVDStateSerializer() = default;

  /*! \brief Default destructor */
  ~CDVDStateSerializer() = default;

  /*! \brief Provided the state in xml format, fills a DVDState struct representing the DVD state and returns the
  * success status of the operation
  * \param[in,out] state the DVD state struct to be filled
  * \param xmlstate a string describing the dvd state (XML)
  * \return true if it was possible to fill the state struct based on the XML content, false otherwise
  */
  bool XMLToDVDState(DVDState& state, const std::string& xmlstate);

  /*! \brief Provided the DVDState struct of the current playing dvd, serializes the struct to XML
  * \param[in,out] xmlstate a string describing the dvd state (XML)
  * \param state the DVD state struct
  * \return true if it was possible to serialize the struct into XML, false otherwise
  */
  bool DVDStateToXML(std::string& xmlstate, const DVDState& state);

private:
  /*! \brief Appends a new element with the given name and value to a provided root XML element
  * \param[in,out] root the root xml element to append the new element
  * \param name the new element name
  * \param value the new element value
  */
  void AddXMLElement(tinyxml2::XMLElement& root, const std::string& name, const std::string& value);
};
