/*
 *      Copyright (C) 2005-2013 Team XBMC
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

#include "File.h"

namespace XBMCAddon
{
  namespace xbmcvfs
  {
    XbmcCommons::Buffer File::readBytes(unsigned long numBytes)
    {
      DelayedCallGuard dg(languageHook);
      int64_t size = file->GetLength();
      if ((!numBytes || (((int64_t)numBytes) > size)) && (size >= 0))
        numBytes = (unsigned long) size;

      
      XbmcCommons::Buffer ret(numBytes);

      if (numBytes == 0)
        return ret;

      while(ret.remaining() > 0)
      {
        ssize_t bytesRead = file->Read(ret.curPosition(), ret.remaining());
        if (bytesRead <= 0) // we consider this a failure or a EOF, can't tell which,
        {                   //  return whatever we have already.
          ret.flip();
          return ret;
        }
        ret.forward(bytesRead);
      }
      ret.flip();
      return ret;
    }

    bool File::write(XbmcCommons::Buffer& buffer)
    {
      DelayedCallGuard dg(languageHook);
      while (buffer.remaining() > 0)
      {
        ssize_t bytesWritten = file->Write( buffer.curPosition(), buffer.remaining());
        if (bytesWritten == 0)       // this could be a failure (see HDFile, and XFileUtils) or
                                     //  it could mean something else when a negative number means an error
                                     //  (see CCurlFile). There is no consistency so we can only assume we're
                                     //  done when we get a 0.
          return false;
        else if (bytesWritten < 0)   // But, if we get something less than zero, we KNOW it's an error.
          return false;
        buffer.forward(bytesWritten);// Otherwise, we advance the buffer by the amount written.
      }
      return true;
    }

  }
}
