#ifndef CPLEXSERVERVERSION_H
#define CPLEXSERVERVERSION_H

#include <string>
#include "StdString.h"

class CPlexServerVersion
{
public:
  CPlexServerVersion();
  CPlexServerVersion(const std::string& versionString);
  bool parse(const std::string& versionString);

  bool operator==(const CPlexServerVersion& otherVersion)
  {
    return (otherVersion.major == major && otherVersion.minor == minor &&
            otherVersion.micro == micro && otherVersion.patch == patch &&
            otherVersion.build == build && otherVersion.gitrev == gitrev);
  }

  friend bool operator>(const CPlexServerVersion& version, const CPlexServerVersion& otherVersion)
  {
    return !(version < otherVersion);
  }

  friend bool operator<(const CPlexServerVersion& version, const CPlexServerVersion& otherVersion)
  {
    std::string ver1 = version.shortString();
    std::string ver2 = otherVersion.shortString();
    return std::lexicographical_compare(ver1.begin(), ver1.end(), ver2.begin(), ver2.end());
  }

  CStdString shortString() const
  {
    CStdString shortStr;
    CStdString dev;

    if (!isDev)
      dev.Format(".%05d", build);

    shortStr.Format("%02d.%02d.%02d.%02d%s", major, minor, micro, patch, dev);

    return shortStr;
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
