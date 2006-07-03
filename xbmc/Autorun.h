//  CAutorun   -  Autorun for different Cd Media
//         like DVD Movies or XBOX Games
//
// by Bobbin007 in 2003
//
//
//

#pragma once
#include "filesystem/factoryDirectory.h"

namespace MEDIA_DETECT
{
class CAutorun
{
public:
  CAutorun();
  virtual ~CAutorun();
  static bool PlayDisc();
  void HandleAutorun();
  void Enable();
  void Disable();
  bool IsEnabled();
protected:
  static void ExecuteXBE(const CStdString &xbeFile);
  void ExecuteAutorun();
  void RunXboxCd();
  void RunCdda();
  void RunISOMedia();
  static bool RunDisc(IDirectory* pDir, const CStdString& strDrive, int& nAddedToPlaylist, bool bRoot, bool bypassSettings = false);
  bool m_bEnable;
};
}
