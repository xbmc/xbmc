/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

/*! \brief Pod structure which represents the current Bluray state */
struct BlurayState
{
  /*! The current playlist id */
  int32_t playlistId = -1;
};

/*! \brief Auxiliar class to serialize/deserialize the Bluray state (into/from XML)
*/
class CBlurayStateSerializer
{
public:
  /*! \brief Default constructor */
  CBlurayStateSerializer() = default;

  /*! \brief Default destructor */
  ~CBlurayStateSerializer() = default;

  /*! \brief Provided the state in xml format, fills a BlurayState struct representing the Bluray state and returns the
  * success status of the operation
  * \param[in,out] state the Bluray state struct to be filled
  * \param xmlstate a string describing the Bluray state (XML)
  * \return true if it was possible to fill the state struct based on the XML content, false otherwise
  */
  bool XMLToBlurayState(BlurayState& state, const std::string& xmlstate);

  /*! \brief Provided the BlurayState struct of the current playing dvd, serializes the struct to XML
  * \param[in,out] xmlstate a string describing the Bluray state (XML)
  * \param state the Bluray state struct
  * \return true if it was possible to serialize the struct into XML, false otherwise
  */
  bool BlurayStateToXML(std::string& xmlstate, const BlurayState& state);
};
