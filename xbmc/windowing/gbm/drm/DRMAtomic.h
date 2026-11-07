/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "DRMUtils.h"
#include "threads/CriticalSection.h"
#include "utils/log.h"

#include <cstdint>
#include <deque>
#include <map>
#include <memory>
#include <vector>

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
  bool SupportsFencing() override { return true; }
  bool AddProperty(CDRMObject* object, const char* name, uint64_t value);
  /*!
   * \brief Queue a blob property. The queued request shares ownership of the blob, so it is
   * only destroyed once every request that references its id (including the last successful
   * request kept as fallback) has been released.
   */
  bool AddBlobProperty(CDRMObject* object,
                       const char* name,
                       const std::shared_ptr<CDRMPropertyBlob>& blob);

  // Probe whether any DRM device supports atomic modesetting, with no
  // connector required. Used to decide atomic-vs-legacy before falling back.
  static bool SupportsAtomicModesetting();

private:
  void DrmAtomicCommit(int fb_id, int flags, bool rendered, bool videoLayer);

  bool m_need_modeset{true};
  bool m_active = true;

  class CDRMAtomicRequest
  {
  public:
    CDRMAtomicRequest();
    ~CDRMAtomicRequest() = default;
    CDRMAtomicRequest(const CDRMAtomicRequest& right) = delete;

    drmModeAtomicReqPtr Get() const { return m_atomicRequest.get(); }

    bool AddProperty(CDRMObject* object, const char* name, uint64_t value);
    void HoldBlob(std::shared_ptr<CDRMPropertyBlob> blob) { m_blobs.push_back(std::move(blob)); }
    void CacheProperties();
    void LogAtomicRequest();

    static void LogAtomicDiff(CDRMAtomicRequest* current, CDRMAtomicRequest* old);

  private:
    static void LogAtomicRequest(
        uint8_t logLevel, std::map<CDRMObject*, std::map<uint32_t, uint64_t>>& atomicRequestItems);

    std::map<CDRMObject*, std::map<uint32_t, uint64_t>> m_atomicRequestItems;
    // blobs referenced by id in this request, must outlive it
    std::vector<std::shared_ptr<CDRMPropertyBlob>> m_blobs;

    struct DrmModeAtomicReqDeleter
    {
      void operator()(drmModeAtomicReqPtr p) const;
    };

    std::unique_ptr<drmModeAtomicReq, DrmModeAtomicReqDeleter> m_atomicRequest;
  };

  CDRMAtomicRequest* m_req = nullptr;
  // Guards m_req: properties may be queued from the video thread (e.g. live HDR metadata
  // refresh) while the render thread builds and commits the request in FlipPage().
  CCriticalSection m_reqLock;
  std::deque<std::unique_ptr<CDRMAtomicRequest>> m_atomicRequestQueue;
};

} // namespace GBM
} // namespace WINDOWING
} // namespace KODI
