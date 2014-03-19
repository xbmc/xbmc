#ifndef CPLEXSERVERVERSION_H
#define CPLEXSERVERVERSION_H

#include <string>

class CPlexServerVersion
{
public:
  CPlexServerVersion();
  CPlexServerVersion(const std::string& versionString);
  bool parse(const std::string& versionString);

  bool operator ==(const CPlexServerVersion& otherVersion)
  {
    return (otherVersion.major == major &&
            otherVersion.minor == minor &&
            otherVersion.micro == micro &&
            otherVersion.patch == patch &&
            otherVersion.build == build &&
            otherVersion.gitrev == gitrev);
  }

  friend bool operator >(const CPlexServerVersion& version,
                         const CPlexServerVersion& otherVersion)
  {
    return (version.major > otherVersion.major ||
            version.minor > otherVersion.minor ||
            version.micro > otherVersion.micro ||
            version.patch > otherVersion.patch ||
            version.build > otherVersion.build);
  }

  friend bool operator <(const CPlexServerVersion& version,
                         const CPlexServerVersion& otherVersion)
  {
    return (version.major < otherVersion.major ||
            version.minor < otherVersion.minor ||
            version.micro < otherVersion.micro ||
            version.patch < otherVersion.patch ||
            version.build < otherVersion.build);
  }


  int major;
  int minor;
  int micro;
  int patch;
  int build;
  bool isDev;
  bool isValid;
  std::string gitrev;
};

#endif // CPLEXSERVERVERSION_H
