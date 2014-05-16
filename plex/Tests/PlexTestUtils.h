#ifndef PLEXTESTUTILS_H
#define PLEXTESTUTILS_H

#include "PlexTest.h"
#include "Client/PlexServer.h"
#include "FileSystem/PlexDirectory.h"

///////////////////////////////////////////////////////////////////////////////////////////////////
class PlexServerManagerTestUtility : public ::testing::Test
{
public:
  virtual void SetUp();
  virtual void TearDown();

  CPlexServerPtr server;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
class PlexDirectoryFakeDataTest : public XFILE::CPlexDirectory
{
public:
  PlexDirectoryFakeDataTest(const std::string& fakedata) : CPlexDirectory(), m_fakedata(fakedata) {}

  bool GetXMLData(CStdString& data)
  {
    data = m_fakedata;
    return true;
  }

  std::string m_fakedata;
};

///////////////////////////////////////////////////////////////////////////////////////////////////
namespace PlexTestUtils
{
  bool listFromXML(const CStdString& xml, CFileItemList& list);
}

#endif // PLEXTESTUTILS_H
