
#pragma once

class CDVDInputStream;
class IDVDPlayer;

class CDVDFactoryInputStream
{
public:
  static CDVDInputStream* CreateInputStream(IDVDPlayer* pPlayer, const char* strFile);
};
