/*
 *      Copyright (C) 2009 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef _MD5_H_
#define _MD5_H_ 1

#include <openssl/evp.h>
#include "StdString.h"

namespace XBMC
{
  class XBMC_MD5
  {
  public:
    XBMC_MD5(void);
    ~XBMC_MD5(void);
    void append(const void *inBuf, size_t inLen);
    void append(const CStdString& str);
    void getDigest(unsigned char digest[16]);
    void getDigest(CStdString& digest);

  private:
    EVP_MD_CTX m_ctx;
    unsigned int m_mdlen;
  };
}

#endif
