#pragma once

#include <boost/shared_ptr.hpp>
#include "PlexMediaStream.h"
#include <vector>
#include <string>

class PlexMediaPart
{
public:
  PlexMediaPart(int id, const std::string& key, int duration)
  : id(id), key(key), duration(duration) {}

  int id;
  int duration;
  std::string key;
  std::vector<PlexMediaStreamPtr> mediaStreams;
};

typedef boost::shared_ptr<PlexMediaPart> PlexMediaPartPtr;
