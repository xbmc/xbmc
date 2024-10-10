/*
 *  Copyright (C) 2022 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <stdint.h>
#include <vector>

/*!
 * \brief Abstracts a Closed Caption data block
 */
class CCaptionBlock
{
public:
  /*!
   * \brief Closed Caption block constructor
   *
   * \param[in] pts - the presentation timestamp of this closed caption
   * \param[in] data - the data to store on the block
   * \param[in] size - the size of the data to store
  */
  CCaptionBlock(double pts, uint8_t* data, int size) : m_pts(pts)
  {
    m_data.resize(size);
    std::copy(data, data + size, m_data.data());
  }

  CCaptionBlock(const CCaptionBlock&) = delete;
  CCaptionBlock& operator=(const CCaptionBlock&) = delete;
  ~CCaptionBlock() = default;

  /*!
   * \brief Get the presentation timestamp (pts) of the closed caption block
   *
   * \return the pts of the closed caption block
  */
  double GetPTS() const { return m_pts; }

  /*!
   * \brief Get the data stored in the closed caption block
   *
   * \return the data stored in the closed caption block
  */
  const std::vector<uint8_t>& GetData() const { return m_data; }

private:
  /*!
   * \brief the presentation timestamp of the closed caption block
  */
  double m_pts{0.0};
  /*!
   * \brief the data of the closed caption block
  */
  std::vector<uint8_t> m_data;
};
