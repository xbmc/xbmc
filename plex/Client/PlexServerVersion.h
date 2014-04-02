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
    return std::lexicographical_compare(version.shortString().begin(), version.shortString().end(),
                                        otherVersion.shortString().begin(),
                                        otherVersion.shortString().end());
  }

  CStdString shortString() const
  {
    CStdString shortStr;
    CStdString dev;

    if (!isDev)
      dev.Format(".%d", build);

    shortStr.Format("%d.%d.%d.%d%s", major, minor, micro, patch, dev);

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
