#pragma once

#include "StdString.h"
#include <vector>
#include <boost/shared_ptr.hpp>

class CPlexServer;
typedef boost::shared_ptr<CPlexServer> CPlexServerPtr;

#include "PlexConnection.h"

class CPlexServer
{
public:
  CPlexServer(const CStdString& uuid, const CStdString& name, bool owned)
    : m_owned(owned), m_uuid(uuid), m_name(name) {}

  CPlexServer() {}

  void CollectDataFromRoot(const CStdString xmlData);
  CStdString toString() const;

private:
  bool m_owned;
  CStdString m_uuid;
  CStdString m_name;
  CStdString m_version;
  CStdString m_owner;
  CStdString m_serverClass;

  bool m_supportsDeletion;
  bool m_supportsAudioTranscoding;
  bool m_supportsVideoTranscoding;

  std::vector<std::string> m_transcoderQualities;
  std::vector<std::string> m_transcoderBitrates;
  std::vector<std::string> m_transcoderResolutions;
};
