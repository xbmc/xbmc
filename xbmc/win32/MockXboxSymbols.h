
// I'm lazy. Anyone want to fill these methods?

extern "C" int WSAAPI XNetRandom( BYTE *pb, UINT cb)
{
  return 0;
}

extern "C" DWORD WSAAPI XNetGetTitleXnAddr( void *pxna )
{
  return 0;
}

extern "C" int WSAAPI XNetDnsRelease( void *pxndns )
{
  return 0;
}

extern "C" int WSAAPI XNetDnsLookup( const char *pszHost, WSAEVENT hEvent, void **ppxndns )
{
  return 0;
}