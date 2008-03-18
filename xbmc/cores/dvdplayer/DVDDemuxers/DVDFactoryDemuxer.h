
#pragma once

class CDVDDemux;
class CDVDInputStream;

class CDVDFactoryDemuxer
{
public:
  static CDVDDemux* CreateDemuxer(CDVDInputStream* pInputStream);
};
