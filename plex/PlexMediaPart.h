#pragma once

#include <boost/shared_ptr.hpp>
#include "PlexMediaStream.h"
#include <vector>
#include <string>

class PlexMediaPart
{
public:
  PlexMediaPart(int id, const std::string& key, int duration)
  : id(id), duration(duration), key(key) {}
  
  int selectedStreamOfType(int streamType)
  {
    for (int i = 0; i < mediaStreams.size(); i++)
    {
      if (mediaStreams[i]->streamType == streamType && mediaStreams[i]->selected)
        return mediaStreams[i]->index;
    }
    
    return -1;
  }
  
  /* Note that this just updates the local data, not the remote */
  void setSelectedStream(int streamType, int id)
  {
    for (int i = 0; i < mediaStreams.size(); i++)
    {
      if (mediaStreams[i]->streamType == streamType)
      {
        if (mediaStreams[i]->id == id)
          mediaStreams[i]->selected = true;
        else
          mediaStreams[i]->selected = false;
      }
    }
  }

  int id;
  int duration;
  std::string key;
  std::vector<PlexMediaStreamPtr> mediaStreams;
};

typedef boost::shared_ptr<PlexMediaPart> PlexMediaPartPtr;
