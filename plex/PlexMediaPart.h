#pragma once

#include <boost/shared_ptr.hpp>
#include <vector>
#include <string>
#include "FileItem.h"

class CPlexMediaStreams : public CFileItemList
{
};

class CPlexMediaParts : public CFileItemList
{
  public:
    CPlexMediaStreams m_streams;
};
