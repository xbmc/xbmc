/*
 *      Copyright (C) 2013 Team XBMC
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

#include "PlayListMov.h"
#include "URL.h"
#include "utils/URIUtils.h"
#include "DVDInputStreams/DVDInputStream.h"
#include "utils/log.h"
#include "filesystem/File.h"
#include "PlayListM3U.h"

using namespace PLAYLIST;

// parse mov refernce files based on http://wiki.multimedia.cx/index.php?title=QuickTime_container#rmcd
bool CPlayListMov::HandleRedirects(CDVDInputStream *inputStream, unsigned int bandwidth)
{
  unsigned int maxBandwidth = 0;
  bool redirected = false;

  if (!inputStream)
    return redirected;

  CURL url= CURL(inputStream->GetURL());

  // convert bandwidth specified in kbps to bps used by the mov reference
  bandwidth *= 1000;
                                                
  if (url.GetFileType() == "mov")
  {
    
    int64_t size = inputStream->GetLength();;
    if (size < 1000000)// allow a max of 1000000 bytes for a reference mov - treat others as real movs
    {
      uint8_t *buff = new uint8_t[size];
      unsigned int readbytes = 0;
      
      readbytes = inputStream->Read(buff, size);
      unsigned int offset = 0;
      bool moovHeaderFound = false;
      bool referencHeaderFound = false;
      while (readbytes && offset < readbytes)
      {
        if (!moovHeaderFound)
        {
          if (buff[offset++] == 'm' && buff[offset++] == 'o' && buff[offset++] == 'o' && buff[offset++] == 'v')
          {
            offset += 4;//skip size info
            moovHeaderFound = true;
          }
        }
        
        if(moovHeaderFound && !referencHeaderFound)
        {
          if (buff[offset++] == 'r' && buff[offset++] == 'm' && buff[offset++] == 'r' && buff[offset++] == 'a')
          {
            offset += 4;//skip size info
            referencHeaderFound = true;
          }
        }
        
        if(referencHeaderFound)
        {
          if (buff[offset++] == 'r' && buff[offset++] == 'm' && buff[offset++] == 'd' && buff[offset++] == 'a')
          {
            offset += 4;//skip size info
            char fourcc[4] = {buff[offset++], buff[offset++], buff[offset++], buff[offset++]};
            // rmda can contain 
            // 'rdrf' for urls/alias, 
            // 'rmdr' minimum datarate in bits per sec
            if (fourcc[0] == 'r' && fourcc[1] == 'd' && fourcc[2] == 'r' && fourcc[3] == 'f')
            {
              offset += 4;//skip size info
              if (buff[offset++] == 'u' && buff[offset++] == 'r' && buff[offset++] == 'l' && buff[offset++] == ' ')
              {
                unsigned  int urlLength = ((unsigned  int)buff[offset++] << 24);//get url length
                urlLength |= ((unsigned  int)buff[offset++] << 16);
                urlLength |= ((unsigned  int)buff[offset++] << 8);
                urlLength |= ((unsigned  int)buff[offset++]);
                
                if (urlLength > 0)
                {
                  //get the url data which is null terminated!
                  std::string urlStr = (const char *)&buff[offset];
                  if (CURL::IsFullPath(urlStr))// if its absolute - exchange hostname and filename
                  {
                    CURL tmpUrl = CURL(urlStr);
                    url.SetHostName(tmpUrl.GetHostName());
                    url.SetFileName(tmpUrl.GetFileName());
                  }
                  else // relative url
                  {
                    std::string tmpStr = URIUtils::GetDirectory(url.GetFileName());
                    url.SetFileName(URIUtils::AddFileToFolder(tmpStr, urlStr));// replace filename
                  }
                  offset += urlLength;
                }
                offset += 4;//skip size info
              }
              fourcc[0] = buff[offset++];
              fourcc[1] = buff[offset++];
              fourcc[2] = buff[offset++];
              fourcc[3] = buff[offset++];                  
            }
            
            unsigned  int dataRate = 0;
            // minimum datarate
            if (fourcc[0] == 'r' && fourcc[1] == 'm' && fourcc[2] == 'd' && fourcc[3] == 'r')
            {
              offset += 4; //skip size info
              dataRate = ((unsigned int)buff[offset++] << 24);//get datarate
              dataRate |= ((unsigned  int)buff[offset++] << 16);
              dataRate |= ((unsigned  int)buff[offset++] << 8);
              dataRate |= ((unsigned  int)buff[offset++]);
            }
            
            if ((maxBandwidth < dataRate) && (dataRate <= bandwidth))
            {
              // store the max bandwidth
              maxBandwidth = dataRate;
              redirected = true;
            }
          }
        }
      }
      delete [] buff;
    }

    if (!redirected)
      inputStream->Seek(0, SEEK_SET);
    else 
    {
      CLog::Log(LOGINFO, "Auto-selecting %s based on configured bandwidth.", url.Get().c_str());
      inputStream->Open(url.Get(), "");
    }
  }                                               

  return redirected;
}
