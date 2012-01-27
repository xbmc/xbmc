#pragma once
#include "utils/ScraperUrl.h"

// single episode information
struct EPISODE
{
  std::pair<int,int> key;
  CDateTime cDate;
  CScraperUrl cScraperUrl;
};

typedef std::vector<EPISODE> EPISODELIST;

