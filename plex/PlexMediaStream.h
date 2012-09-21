#pragma once

#include <boost/shared_ptr.hpp>
#include <string>

class PlexMediaStream
{
public:
  PlexMediaStream(int id, const std::string& key, int streamType, const std::string& codec, int anIndex, int subIndex, bool selected, const std::string& language)
  : id(id), key(key), streamType(streamType), codec(codec), subIndex(subIndex), selected(selected), language(language)
  {
    index = anIndex;	  // This looks crazy, but VS seems to barf at the initialization list.
  }

  int         id;
  std::string key;
  int         streamType;
  std::string codec;
  int         index;
  int         subIndex;
  bool        selected;
  std::string language;
};

typedef boost::shared_ptr<PlexMediaStream> PlexMediaStreamPtr;

