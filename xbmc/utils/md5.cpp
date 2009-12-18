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

#include "md5.h"

XBMC::XBMC_MD5::XBMC_MD5(void)
{
  EVP_MD_CTX_init(&m_ctx);
  EVP_DigestInit(&m_ctx, EVP_md5());
}

XBMC::XBMC_MD5::~XBMC_MD5(void)
{}

void XBMC::XBMC_MD5::append(const void *inBuf, size_t inLen)
{
  EVP_DigestUpdate(&m_ctx, inBuf, inLen);
}

void XBMC::XBMC_MD5::append(const CStdString& str)
{
  append((unsigned char*) str.c_str(), (unsigned int) str.length());
}

void XBMC::XBMC_MD5::getDigest(unsigned char digest[16])
{
  EVP_DigestFinal(&m_ctx, digest, &m_mdlen);
}

void XBMC::XBMC_MD5::getDigest(CStdString& digest)
{
  unsigned char szBuf[16] = {'\0'};
  getDigest(szBuf);
  digest.Format("%02X%02X%02X%02X%02X%02X%02X%02X"\
      "%02X%02X%02X%02X%02X%02X%02X%02X", szBuf[0], szBuf[1], szBuf[2],
      szBuf[3], szBuf[4], szBuf[5], szBuf[6], szBuf[7], szBuf[8],
      szBuf[9], szBuf[10], szBuf[11], szBuf[12], szBuf[13], szBuf[14],
      szBuf[15]);
}
