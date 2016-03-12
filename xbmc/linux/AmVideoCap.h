/*
 *      Copyright (C) 2015 Team Kodi
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

#ifndef AMVIDEOCAP_H_
#define AMVIDEOCAP_H_
#if defined(HAS_LIBAMCODEC)

class CAmVideoCap
{
  public:
    CAmVideoCap();
    ~CAmVideoCap();

    bool CaptureVideoFrame(int destWidth, int destHeight, unsigned char *pixels);
    void CancelCapture();

  private:
    int m_captureFd;
    bool m_deviceOpen;
    int m_bufferSize;
};
#endif //defined(HAS_LIBAMCODEC)
#endif //AMVIDEOCAP_H_
