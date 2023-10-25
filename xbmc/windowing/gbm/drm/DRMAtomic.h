/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DRMUtils.h"

#include <cstdint>
#include <deque>
#include <map>
#include <memory>

namespace KODI
{
namespace WINDOWING
{
namespace GBM
{

class CDRMAtomic : public CDRMUtils
{
public:
  CDRMAtomic() = default;
  ~CDRMAtomic() override = default;
  void FlipPage(struct gbm_bo* bo, bool rendered, bool videoLayer, bool async) override;
  bool SetVideoMode(const RESOLUTION_INFO& res, struct gbm_bo* bo) override;
  bool SetActive(bool active) override;
  bool InitDrm() override;
  void DestroyDrm() override;
  bool AddProperty(CDRMObject* object, const char* name, uint64_t value);

  bool DisplayHardwareScalingEnabled();

private:
  void DrmAtomicCommit(int fb_id, int flags, bool rendered, bool videoLayer);

  bool SetScalingFilter(CDRMObject* object, const char* name, const char* type);

  bool m_need_modeset;
  bool m_active = true;

  class CDRMAtomicRequest
  {
  public:
    CDRMAtomicRequest();
    ~CDRMAtomicRequest() = default;
    CDRMAtomicRequest(const CDRMAtomicRequest& right) = delete;

    drmModeAtomicReqPtr Get() const { return m_atomicRequest.get(); }

    bool AddProperty(CDRMObject* object, const char* name, uint64_t value);
    void LogAtomicRequest();

    static void LogAtomicDiff(CDRMAtomicRequest* current, CDRMAtomicRequest* old);

  private:
    static void LogAtomicRequest(
        uint8_t logLevel, std::map<CDRMObject*, std::map<uint32_t, uint64_t>>& atomicRequestItems);

    std::map<CDRMObject*, std::map<uint32_t, uint64_t>> m_atomicRequestItems;

    struct DrmModeAtomicReqDeleter
    {
      void operator()(drmModeAtomicReqPtr p) const;
    };

    std::unique_ptr<drmModeAtomicReq, DrmModeAtomicReqDeleter> m_atomicRequest;
  };

  CDRMAtomicRequest* m_req = nullptr;
  std::deque<std::unique_ptr<CDRMAtomicRequest>> m_atomicRequestQueue;
};

}
}
}
