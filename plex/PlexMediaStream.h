#pragma once

#include <boost/shared_ptr.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/algorithm/string.hpp>
#include <string>

class PlexMediaStream
{
public:
  PlexMediaStream(int id, const std::string& key, int streamType, const std::string& codec, int anIndex, int subIndex, bool selected, const std::string& language)
  : id(id), key(key), streamType(streamType), codec(codec), subIndex(subIndex), selected(selected), language(language)
  {
    index = anIndex;	  // This looks crazy, but VS seems to barf at the initialization list.
  }
  
  std::string channelName()
  {
    if (channels == 1)
      return "Mono";
    else if (channels == 2)
      return "Stereo";
    
    return boost::lexical_cast<std::string>(channels - 1) + ".1";
  }
  
  std::string codecName()
  {
    if (codec == "dca")
      return "DTS";
    
    return boost::to_upper_copy(codec);
  }


  int         id;
  std::string key;
  int         streamType;
  std::string codec;
  int         index;
  int         subIndex;
  bool        selected;
  std::string language;
  
  int         channels;
};

typedef boost::shared_ptr<PlexMediaStream> PlexMediaStreamPtr;

