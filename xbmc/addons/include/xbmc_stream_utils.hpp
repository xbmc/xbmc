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
#include <algorithm>
#include <map>

namespace ADDON
{
  /**
   * Represents a single stream. It extends the PODS to provide some operators 
   * overloads.
   */
  class XbmcPvrStream : public PVR_STREAM_PROPERTIES::PVR_STREAM
  {
  public:
    XbmcPvrStream()
    {
      Clear();
    }
    
    XbmcPvrStream(const XbmcPvrStream &other)
    {
      memcpy(this, &other, sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
    }
    
    XbmcPvrStream& operator=(const XbmcPvrStream &other)
    {
      memcpy(this, &other, sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
      return *this;
    }
    
    /**
     * Compares this stream based on another stream
     * @param other
     * @return
     */
    inline bool operator==(const XbmcPvrStream &other) const
    {
      return iPhysicalId == other.iPhysicalId && iCodecId == other.iCodecId;
    }

    /**
     * Compares this stream with another one so that video streams are sorted
     * before any other streams and the others are sorted by the physical ID
     * @param other
     * @return
     */
    bool operator<(const XbmcPvrStream &other) const
    {
      if (iCodecType == XBMC_CODEC_TYPE_VIDEO)
        return true;
      else if (other.iCodecType != XBMC_CODEC_TYPE_VIDEO)
        return iPhysicalId < other.iPhysicalId;
      else
        return false;
    }
    
    /**
     * Clears the stream
     */
    void Clear()
    {
      memset(this, 0, sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
      iCodecId = XBMC_INVALID_CODEC_ID;
      iCodecType = XBMC_CODEC_TYPE_UNKNOWN;
    }
    
    /**
     * Checks whether the stream has been cleared
     * @return 
     */
    inline bool IsCleared() const
    {
      return iCodecId   == XBMC_INVALID_CODEC_ID &&
             iCodecType == XBMC_CODEC_TYPE_UNKNOWN;
    }
  };
  
  class XbmcStreamProperties
  {
  public:
    typedef std::vector<XbmcPvrStream> stream_vector;
    
    XbmcStreamProperties(void)
    {
      // make sure the vector won't have to resize itself later
      m_streamVector = new stream_vector();
      m_streamVector->reserve(PVR_STREAM_MAX_STREAMS);
    }

    virtual ~XbmcStreamProperties(void)
    {
      delete m_streamVector;
    }
    
    /**
     * Resets the streams
     */
    void Clear(void)
    {
      m_streamVector->clear();
      m_streamIndex.clear();
    }

    /**
     * Returns the index of the stream with the specified physical ID, or -1 if 
     * there no stream is found. This method is called very often which is why 
     * we keep a separate map for this.
     * @param iPhysicalId
     * @return
     */
    int GetStreamId(unsigned int iPhysicalId) const
    {
      std::map<unsigned int, int>::const_iterator it = m_streamIndex.find(iPhysicalId);
      if (it != m_streamIndex.end())
        return it->second;

      return -1;
    }
    
    /**
     * Returns the stream with the specified physical ID, or null if no such 
     * stream exists
     * @param iPhysicalId
     * @return
     */
    XbmcPvrStream* GetStreamById(unsigned int iPhysicalId) const
    {
      int position = GetStreamId(iPhysicalId);
      return position != -1 ? &m_streamVector->at(position) : NULL;
    }

    /**
     * Populates the specified stream with the stream having the specified 
     * physical ID. If the stream is not found only target stream's physical ID 
     * will be populated.
     * @param iPhysicalId
     * @param stream
     */
    void GetStreamData(unsigned int iPhysicalId, XbmcPvrStream* stream)
    {
      XbmcPvrStream *foundStream = GetStreamById(iPhysicalId);
      if (foundStream)
        *stream = *foundStream;
      else
      {
        stream->iIdentifier = -1;
        stream->iPhysicalId = iPhysicalId;
      }
    }

    /**
     * Populates props with the current streams and returns whether there are 
     * any streams at the moment or not.
     * @param props
     * @return
     */
    bool GetProperties(PVR_STREAM_PROPERTIES* props)
    {
      unsigned int i = 0;
      for (stream_vector::const_iterator it = m_streamVector->begin(); 
           it != m_streamVector->end(); ++it, ++i)
      {
        memcpy(&props->stream[i], &(*it), sizeof(PVR_STREAM_PROPERTIES::PVR_STREAM));
      }
      
      props->iStreamCount = m_streamVector->size();
      return (props->iStreamCount > 0);
    }

    /**
     * Merges new streams into the current list of streams. Identical streams 
     * will retain their respective indexes and new streams will replace unused 
     * indexes or be appended.
     * @param newStreams
     */
    void UpdateStreams(stream_vector &newStreams)
    {
      // sort the new streams
      std::sort(newStreams.begin(), newStreams.end());
      
      // ensure we never have more than PVR_STREAMS_MAX_STREAMS streams
      if (newStreams.size() > PVR_STREAM_MAX_STREAMS)
      {
        while (newStreams.size() > PVR_STREAM_MAX_STREAMS)
          newStreams.pop_back();
        
        XBMC->Log(LOG_ERROR, "%s - max amount of streams reached", __FUNCTION__);
      }
      
      stream_vector::iterator newStreamPosition;
      for (stream_vector::iterator it = m_streamVector->begin(); it != m_streamVector->end(); ++it)
      {
        newStreamPosition = std::find(newStreams.begin(), newStreams.end(), *it);

        // if the current stream no longer exists we clear it, otherwise we 
        // copy it and remove it from newStreams
        if (newStreamPosition == newStreams.end())
          it->Clear();
        else
        {
          *it = *newStreamPosition;
          newStreams.erase(newStreamPosition);
        }
      }

      // replace cleared streams with new streams
      for (stream_vector::iterator it = m_streamVector->begin();
           it != m_streamVector->end() && !newStreams.empty(); ++it)
      {
        if (it->IsCleared())
        {
          *it = newStreams.front();
          newStreams.erase(newStreams.begin());
        }
      }

      // append any remaining new streams
      m_streamVector->insert(m_streamVector->end(), newStreams.begin(), newStreams.end());
      
      // remove trailing cleared streams
      while (m_streamVector->back().IsCleared())
        m_streamVector->pop_back();
      
      // update the index
      UpdateIndex();
    }

  private:
    stream_vector               *m_streamVector;
    std::map<unsigned int, int> m_streamIndex;
    
    /**
     * Updates the stream index
     */
    void UpdateIndex()
    {
      m_streamIndex.clear();
      
      int i = 0;
      for (stream_vector::const_iterator it = m_streamVector->begin(); it != m_streamVector->end(); ++it, ++i)
        m_streamIndex[it->iPhysicalId] = i;
    }
  };
}
