# Full Code Review: PR #27402 - Dynamic Plane Selection

## Summary

I've conducted a comprehensive review of this PR implementing dynamic DRM plane selection. The implementation demonstrates solid architectural understanding of DRM/atomic modesetting, but I've identified **6 critical issues**, **4 high-severity issues**, and **2 medium-severity issues** that need addressing.

**Decision: NOT APPROVED** (pending fixes to critical issues)

---

## Critical Issues (Must Fix Before Merge)

### Issue 1: Bit shift overflow vulnerability
**Location**: xbmc/windowing/gbm/drm/DRMPlane.cpp:102
**Severity**: Critical

The plane format selection code has undefined behavior when `i >= 64`:
```cpp
if (mod[j].formats & 1ULL << i)
```

If `header->count_formats > 64`, this creates undefined behavior. DRM format modifier blobs can contain arbitrary format counts. The loop at line 97 iterates `header->count_formats` times without bounds checking the shift operation.

**Fix**: Add bounds check: `if (i < 64 && (mod[j].formats & (1ULL << i)))`

---

### Issue 2: CRTC bitmask shift overflow
**Location**: xbmc/windowing/gbm/drm/DRMPlane.cpp:116
**Severity**: Critical

Similar overflow risk in CRTC validation:
```cpp
if (crtc != nullptr && !(m_plane->possible_crtcs & (1 << crtc->GetOffset())))
```

If CRTC offset ≥ 32, undefined behavior occurs. While unlikely in practice, it's not validated.

**Fix**: Add validation: `if (crtc != nullptr && (crtc->GetOffset() >= 32 || !(m_plane->possible_crtcs & (1 << crtc->GetOffset()))))`

---

### Issue 3: Property cache corruption on test commit failure
**Location**: xbmc/windowing/gbm/drm/DRMAtomic.cpp:113-143
**Severity**: Critical

When the test-only atomic commit fails, the code falls back to the old request but then modifies it:
```cpp
auto ret = drmModeAtomicCommit(m_fd, m_req->Get(), flags | DRM_MODE_ATOMIC_TEST_ONLY, nullptr);
if (ret < 0)
{
  auto oldRequest = m_atomicRequestQueue.front().get();
  m_req = oldRequest;  // Switch to old request

  if (rendered)
    AddProperty(m_gui_plane, "FB_ID", fb_id);  // Modifies OLD request with NEW fb_id!
}

ret = drmModeAtomicCommit(m_fd, m_req->Get(), flags, nullptr);
// ...
m_req->CacheProperties();  // Caches hybrid state
```

This creates a hybrid state where the old atomic request is modified with the new FB_ID. When `CacheProperties()` is called after successful commit, it caches this mixed state, causing `GetPropertyValue()` to return incorrect property values.

**Impact**: Incorrect plane cleanup logic on subsequent operations, potential display artifacts, failed subsequent commits.

**Fix**: Either (A) create a fresh atomic request with only the FB_ID property when falling back, or (B) cache properties before fallback and restore them after.

---

### Issue 4: Property cache not updated on commit failure
**Location**: xbmc/windowing/gbm/drm/DRMAtomic.cpp:134
**Severity**: Critical

When atomic commit fails, the current request is removed without caching:
```cpp
if (ret < 0)
{
  CLog::LogF(LOGERROR, "atomic commit failed: {}", strerror(errno));
  m_atomicRequestQueue.pop_back();  // Deleted without caching
}
```

This leaves the property cache with stale values. Subsequent `GetPropertyValue()` calls return incorrect data.

**Fix**: Consider whether partial cache update is needed, or document why stale cache is acceptable after commit failure.

---

### Issue 5: Unsafe blob data access
**Location**: xbmc/windowing/gbm/drm/DRMPlane.cpp:87-106
**Severity**: Critical

The code casts blob data to `drm_format_modifier_blob*` without validating that `blob->length` is sufficient:
```cpp
drmModePropertyBlobPtr blob = drmModeGetPropertyBlob(m_fd, blob_id);
if (!blob)
  return;

drm_format_modifier_blob* header = static_cast<drm_format_modifier_blob*>(blob->data);
// No validation that blob->length >= sizeof(drm_format_modifier_blob)
uint32_t* formats = reinterpret_cast<uint32_t*>(
    reinterpret_cast<char*>(header) + header->formats_offset);
```

If blob is truncated or malformed, `formats_offset` and `modifiers_offset` could point outside blob data bounds.

**Fix**: Add blob size validation:
```cpp
if (blob->length < sizeof(drm_format_modifier_blob))
{
  drmModeFreePropertyBlob(blob);
  return;
}

// Validate offsets are within blob bounds
if (header->formats_offset + header->count_formats * sizeof(uint32_t) > blob->length ||
    header->modifiers_offset + header->count_modifiers * sizeof(drm_format_modifier) > blob->length)
{
  drmModeFreePropertyBlob(blob);
  return;
}
```

---

### Issue 6: Incorrect plane cleanup logic
**Location**: xbmc/windowing/gbm/drm/DRMAtomic.cpp:39-62
**Severity**: Critical

The plane cleanup code has multiple issues:
```cpp
for (const auto& plane : m_planes)
{
  // ... skip gui/video planes ...

  uint64_t planeid = plane->GetPropertyValue("CRTC_ID").value_or(0);
  if (planeid == m_crtc->GetId() || planeid == m_old_crtc->GetId())
  {
    AddProperty(plane.get(), "CRTC_ID", 0);  // First disable
    AddProperty(plane.get(), "FB_ID", 0);
  }

  if (!(HasQuirk(QUIRK_NEEDSPRIMARY)))
  {
    AddProperty(plane.get(), "CRTC_ID", 0);  // Second disable (redundant!)
    AddProperty(plane.get(), "FB_ID", 0);
  }
}
```

Problems:
- Planes get disabled twice when `!QUIRK_NEEDSPRIMARY`
- Comment says "disables planes in other crtcs" but code disables unconditionally in second block
- Uses cached `GetPropertyValue("CRTC_ID")` during atomic operation (cache might be stale)

**Fix**: Clarify intent and remove redundant disables. Consider whether cached property values are reliable during atomic state transitions.

---

## High Severity Issues (Should Fix)

### Issue 7: Partial failure in zpos manipulation
**Location**: xbmc/windowing/gbm/drm/DRMPlane.cpp:185-202
**Severity**: Major

The `MoveOnTopOf` function can leave planes in inconsistent state:
```cpp
bool success = SetProperty("zpos", zpos_max) && other->SetProperty("zpos", other_zpos_min);
```

If first `SetProperty` succeeds but second fails, we return false but first plane's zpos was already modified.

**Fix**: Implement rollback or two-phase commit for zpos changes.

---

### Issue 8: Property blob leak on early return
**Location**: xbmc/windowing/gbm/drm/DRMAtomic.cpp:71-78
**Severity**: Major

If `AddProperty` fails after blob creation, blob is never destroyed:
```cpp
if (drmModeCreatePropertyBlob(m_fd, m_mode, sizeof(*m_mode), &blob_id) != 0)
  return;  // No leak here

if (!AddProperty(m_crtc, "MODE_ID", blob_id))
  return;  // LEAK: blob_id not destroyed until line 157

if (!AddProperty(m_crtc, "ACTIVE", m_active ? 1 : 0))
  return;  // LEAK: blob_id not destroyed
```

**Fix**: Add cleanup before early returns:
```cpp
if (!AddProperty(m_crtc, "MODE_ID", blob_id))
{
  drmModeDestroyPropertyBlob(m_fd, blob_id);
  return;
}
```

---

### Issue 9: Dangling pointer risk
**Location**: xbmc/windowing/gbm/drm/DRMUtils.cpp:195-196, 263-267
**Severity**: Major

Raw pointers to planes owned by `m_planes` vector:
```cpp
m_gui_plane = plane.get();  // Raw pointer from unique_ptr in vector
m_video_plane = plane.get();
```

If `m_planes` vector is reallocated or modified, these become dangling pointers.

**Fix**: Ensure `m_planes` vector is never modified after plane selection, or use indices instead of raw pointers.

---

### Issue 10: Out-of-bounds array access risk
**Location**: xbmc/windowing/gbm/drm/DRMObject.cpp:116, 125
**Severity**: Major

No validation that property arrays are synchronized:
```cpp
return m_props->prop_values[std::ranges::distance(m_propsInfo.begin(), property)];
```

If `m_propsInfo` and `m_props->prop_values` get out of sync, this accesses out-of-bounds memory.

**Fix**: Add debug assertion that array sizes match:
```cpp
assert(m_propsInfo.size() == m_props->count_props);
```

---

## Medium Severity Issues

### Issue 11: O(C × P²) plane selection complexity
**Location**: xbmc/windowing/gbm/drm/DRMUtils.cpp:243-278
**Severity**: Minor

Triple-nested loop in `FindVideoAndGuiPlane`:
```cpp
for (auto& crtc : m_encoder->GetPossibleCrtcs(m_crtcs, m_crtc))
  for (auto& gui_plane : m_planes)
    for (auto& video_plane : m_planes)
```

With many planes/CRTCs, this could be slow.

**Recommendation**: Consider early break conditions or plane capability pre-filtering.

---

### Issue 12: Magic number constants
**Location**: xbmc/windowing/gbm/drm/DRMPlane.cpp:45, 102
**Severity**: Minor

Hardcoded bit positions and vendor IDs:
```cpp
if (modifier >> 56 == DRM_FORMAT_MOD_VENDOR_BROADCOM)  // 56 is vendor field position
if (mod[j].formats & 1ULL << i)  // Assumes 64-bit bitmask
```

**Recommendation**: Use named constants from `drm_fourcc.h` or define locally.

---

## Positive Observations

- Property value caching (commit d1dad58e) is well-designed and addresses the stale cache issue mentioned in comments
- The `string_view` refactoring (61e5c7a7) is good for performance
- CLog::LogF migration (64f98a33) improves code consistency
- The CRTC change handling (387b342d) correctly manages old_crtc cleanup
- Comprehensive testing by community members shows real-world validation

---

## Testing Recommendations

Given the complexity of DRM driver interactions, recommend:

1. Stress test CRTC switching with multiple rapid mode changes
2. Test with deliberately truncated/malformed IN_FORMATS blobs (driver fuzzing)
3. Test plane selection with >64 formats in modifier blob
4. Test with >32 CRTCs (if hardware available)
5. Test atomic commit failures with subsequent recovery
6. Memory leak testing with valgrind during plane switching

---

## Next Steps

The critical issues (1-6) must be fixed before merge. The high-severity issues (7-10) should be addressed for robustness. Consider adding defensive assertions for debugging builds.

Once fixes are applied, I recommend another review pass focusing on the atomic commit state machine and property cache coherence.
