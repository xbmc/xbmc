//  CAutorun   -  Autorun for different Cd Media
//         like DVD Movies or XBOX Games
//
// by Bobbin007 in 2003
//
//
//

#pragma once
#include "FileSystem/FactoryDirectory.h"

namespace MEDIA_DETECT
{
class CAutorun
{
public:
  CAutorun();
  virtual ~CAutorun();
  static bool PlayDisc();
  bool IsEnabled() const;
  void Enable();
  void Disable();
  void HandleAutorun();
protected:
  static void ExecuteXBE(const CStdString &xbeFile);
  static void ExecuteAutorun(bool bypassSettings = false, bool ignoreplaying=false);
  static void RunXboxCd(bool bypassSettings = false);
  static void RunCdda();
  static void RunISOMedia(bool bypassSettings = false);
  static bool RunDisc(DIRECTORY::IDirectory* pDir, const CStdString& strDrive, int& nAddedToPlaylist, bool bRoot, bool bypassSettings = false);
  bool m_bEnable;
};
}
