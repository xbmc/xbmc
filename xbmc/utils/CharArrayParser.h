/*
 *  Copyright (C) 2005-2021 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <cstdint>
#include <string>

/*!
 *\brief Wraps a char array, providing a set of methods for parsing data from it.
 */
class CCharArrayParser
{
public:
  CCharArrayParser() = default;
  ~CCharArrayParser() = default;

  /*!
   * \brief Sets the position and limit to zero
   */
  void Reset();

  /*!
   * \brief Updates the instance to wrap the specified data and resets the position to zero
   * \param data The data
   * \param limit The limit of length of the data
   */
  void Reset(const char* data, int limit);

  /*!
   * \brief Return the number of chars yet to be read
   */
  int CharsLeft();

  /*!
   * \brief Returns the current offset in the array
   */
  int GetPosition();

  /*!
   * \brief Set the reading offset in the array
   * \param position The new offset position
   * \return True if success, otherwise false
   */
  bool SetPosition(int position);

  /*!
   * \brief Skip a specified number of chars
   * \param nChars The number of chars
   * \return True if success, otherwise false
   */
  bool SkipChars(int nChars);

  /*!
   * \brief Reads the next unsigned char (it is assumed that the caller has
   * already checked the availability of the data for its length)
   * \return The unsigned char value
   */
  uint8_t ReadNextUnsignedChar();

  /*!
   * \brief Reads the next two chars as unsigned short value (it is assumed
   * that the caller has already checked the availability of the data for its length)
   * \return The unsigned short value
   */
  uint16_t ReadNextUnsignedShort();

  /*!
   * \brief Reads the next four chars as unsigned int value (it is assumed 
   * that the caller has already checked the availability of the data for its length)
   * \return The unsigned int value
   */
  uint32_t ReadNextUnsignedInt();

  /*!
   * \brief Reads the next string of specified length (it is assumed that
   * the caller has already checked the availability of the data for its length)
   * \param length The length to be read
   * \return The string value
   */
  std::string ReadNextString(int length);

  /*!
   * \brief Reads the next chars array of specified length (it is assumed that
   * the caller has already checked the availability of the data for its length)
   * \param length The length to be read
   * \param data[OUT] The data read
   * \return True if success, otherwise false
   */
  bool ReadNextArray(int length, char* data);

  /*!
   * \brief Reads a line of text.
   * A line is considered to be terminated by any one of a carriage return ('\\r'),
   * a line feed ('\\n'), or a carriage return followed by a line feed ('\\r\\n'),
   * this method discards leading UTF-8 byte order marks, if present.
   * \param line [OUT] The line read without line-termination characters
   * \return True if read, otherwise false if the end of the data has already
   *         been reached
   */
  bool ReadNextLine(std::string& line);

  /*!
   * \brief Get the current data
   * \return The char pointer to the current data
   */
  const char* GetData() { return m_data; };

private:
  const char* m_data{nullptr};
  int m_position{0};
  int m_limit{0};
};
