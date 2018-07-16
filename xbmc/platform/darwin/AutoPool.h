/*
 *  Copyright (C) 2009-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

class CCocoaAutoPool
{
  public:
    CCocoaAutoPool();
    ~CCocoaAutoPool();
  private:
    void *m_opaque_pool;
};

#ifdef __cplusplus
extern "C"
{
#endif
  // Pools.
  //
  void* Cocoa_Create_AutoReleasePool(void);
  void Cocoa_Destroy_AutoReleasePool(void* pool);

#ifdef __cplusplus
}
#endif

