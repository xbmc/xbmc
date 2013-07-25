#pragma once
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

#include "xbmc_pvr_types.h"
#include <map>

namespace ADDON
{
  class XbmcStreamProperties
  {
  public:
    XbmcStreamProperties(void)
    {
      Clear();
    }

    virtual ~XbmcStreamProperties(void)
    {
    }

    int GetStreamId(unsigned int iPhysicalId)
    {
      std::map<unsigned int, unsigned int>::iterator it = m_streamIndex.find(iPhysicalId);
      if (it != m_streamIndex.end())
        return it->second;
      return -1;
    }

    void GetStreamData(unsigned int iPhysicalId, PVR_STREAM_PROPERTIES::PVR_STREAM* stream)
    {
      std::map<unsigned int, unsigned int>::iterator it = m_streamIndex.find(iPhysicalId);
      if (it != m_streamIndex.end())
      {
        memcpy(stream, &m_streams.stream[it->second], sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
      }
      else
      {
        memset(stream, 0, sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
        stream->iIdentifier = -1;
        stream->iPhysicalId = iPhysicalId;
      }
    }

    PVR_STREAM_PROPERTIES::PVR_STREAM* GetStreamById(unsigned int iPhysicalId)
    {
      std::map<unsigned int, unsigned int>::iterator it = m_streamIndex.find(iPhysicalId);
      if (it != m_streamIndex.end())
        return &m_streams.stream[it->second];
      return NULL;
    }

    bool GetProperties(PVR_STREAM_PROPERTIES* props) const
    {
      props->iStreamCount = m_streams.iStreamCount;
      for (unsigned int i = 0; i < m_streams.iStreamCount; i++)
      {
        props->stream[i].iPhysicalId    = m_streams.stream[i].iPhysicalId;
        props->stream[i].iCodecType     = m_streams.stream[i].iCodecType;
        props->stream[i].iCodecId       = m_streams.stream[i].iCodecId;
        props->stream[i].strLanguage[0] = m_streams.stream[i].strLanguage[0];
        props->stream[i].strLanguage[1] = m_streams.stream[i].strLanguage[1];
        props->stream[i].strLanguage[2] = m_streams.stream[i].strLanguage[2];
        props->stream[i].strLanguage[3] = m_streams.stream[i].strLanguage[3];
        props->stream[i].iIdentifier    = m_streams.stream[i].iIdentifier;
        props->stream[i].iFPSScale      = m_streams.stream[i].iFPSScale;
        props->stream[i].iFPSRate       = m_streams.stream[i].iFPSRate;
        props->stream[i].iHeight        = m_streams.stream[i].iHeight;
        props->stream[i].iWidth         = m_streams.stream[i].iWidth;
        props->stream[i].fAspect        = m_streams.stream[i].fAspect;
        props->stream[i].iChannels      = m_streams.stream[i].iChannels;
        props->stream[i].iSampleRate    = m_streams.stream[i].iSampleRate;
        props->stream[i].iBlockAlign    = m_streams.stream[i].iBlockAlign;
        props->stream[i].iBitRate       = m_streams.stream[i].iBitRate;
        props->stream[i].iBitsPerSample = m_streams.stream[i].iBitsPerSample;
      }

      return (props->iStreamCount > 0);
    }

    void Clear(void)
    {
      memset(&m_streams, 0, sizeof(PVR_STREAM_PROPERTIES));
      for (unsigned int i = 0; i < PVR_STREAM_MAX_STREAMS; i++)
        m_streams.stream[i].iCodecType = XBMC_CODEC_TYPE_UNKNOWN;
    }

    unsigned int NextFreeIndex(void)
    {
      unsigned int i;
      for (i = 0; i < PVR_STREAM_MAX_STREAMS; i++)
      {
        if (m_streams.stream[i].iCodecType == XBMC_CODEC_TYPE_UNKNOWN)
          break;
      }
      return i;
    }

    static std::map<unsigned int, unsigned int> CreateIndex(PVR_STREAM_PROPERTIES streams)
    {
      std::map<unsigned int, unsigned int> retval;
      for (unsigned int i = 0; i < PVR_STREAM_MAX_STREAMS && i < streams.iStreamCount; i++)
        retval.insert(std::make_pair(streams.stream[i].iPhysicalId, i));
      return retval;
    }

    static std::map<unsigned int, unsigned int> CreateIndex(const std::vector<PVR_STREAM_PROPERTIES::PVR_STREAM>& streams)
    {
      std::map<unsigned int, unsigned int> retval;
      for (unsigned int i = 0; i < PVR_STREAM_MAX_STREAMS && i < streams.size(); i++)
        retval.insert(std::make_pair(streams.at(i).iPhysicalId, i));
      return retval;
    }

    static void ClearStream(PVR_STREAM_PROPERTIES::PVR_STREAM* stream)
    {
      memset(stream, 0, sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
      stream->iCodecType = XBMC_CODEC_TYPE_UNKNOWN;
      stream->iCodecId   = XBMC_INVALID_CODEC_ID;
    }

    void UpdateStreams(const std::vector<PVR_STREAM_PROPERTIES::PVR_STREAM>& newStreams)
    {
      std::map<unsigned int, unsigned int> newIndex = CreateIndex(newStreams);

      // delete streams we don't have in newStreams
      std::map<unsigned int, unsigned int>::iterator ito = m_streamIndex.begin();
      std::map<unsigned int, unsigned int>::iterator itn;
      while (ito != m_streamIndex.end())
      {
        itn = newIndex.find(ito->first);
        if (itn == newIndex.end())
        {
          memset(&m_streams.stream[ito->second], 0, sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
          m_streams.stream[ito->second].iCodecType = XBMC_CODEC_TYPE_UNKNOWN;
          m_streams.stream[ito->second].iCodecId   = XBMC_INVALID_CODEC_ID;
          m_streamIndex.erase(ito);
          ito = m_streamIndex.begin();
        }
        else
          ++ito;
      }

      // copy known streams
      for (ito = m_streamIndex.begin(); ito != m_streamIndex.end(); ++ito)
      {
        itn = newIndex.find(ito->first);
        memcpy(&m_streams.stream[ito->second], &newStreams.at(itn->second), sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
        newIndex.erase(itn);
      }

      // place video stream at pos 0
      for (itn = newIndex.begin(); itn != newIndex.end(); ++itn)
      {
        if (newStreams.at(itn->second).iCodecType == XBMC_CODEC_TYPE_VIDEO)
        {
          m_streamIndex[itn->first] = 0;
          memcpy(&m_streams.stream[0], &newStreams.at(itn->second), sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
          newIndex.erase(itn);
          break;
        }
      }

      // fill the gaps or append after highest index
      while (!newIndex.empty())
      {
        // find first unused index
        unsigned int i = NextFreeIndex();
        itn = newIndex.begin();
        m_streamIndex[itn->first] = i;
        memcpy(&m_streams.stream[i], &newStreams.at(itn->second), sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
        newIndex.erase(itn);
      }

      // set streamCount
      m_streams.iStreamCount = 0;
      for (ito = m_streamIndex.begin(); ito != m_streamIndex.end(); ++ito)
      {
        if (ito->second > m_streams.iStreamCount)
          m_streams.iStreamCount = ito->second;
      }
      if (!m_streamIndex.empty())
        m_streams.iStreamCount++;
    }

    size_t Size(void) const
    {
      return m_streamIndex.size();
    }

    std::map<unsigned int, unsigned int> m_streamIndex;
    PVR_STREAM_PROPERTIES                m_streams;
  };
}
