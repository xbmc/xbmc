/*
 *      Copyright (C) 2009-2013 Team XBMC
 *      http://xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#ifndef _MD5_H_
#define _MD5_H_

#include <string>
#include <stdint.h>

struct MD5Context {
	uint32_t buf[4];
	uint32_t bytes[2];
	uint32_t in[16];
};

namespace XBMC
{
  class XBMC_MD5
  {
  public:
    XBMC_MD5(void);
    ~XBMC_MD5(void);
    void append(const void *inBuf, size_t inLen);
    void append(const std::string& str);
    void getDigest(unsigned char digest[16]);
    std::string getDigest();

    /*! \brief Get the MD5 digest of the given text
     \param text text to compute the MD5 for
     \return MD5 digest
     */
    static std::string GetMD5(const std::string &text);
private:
    MD5Context m_ctx;
  };
}

#endif
