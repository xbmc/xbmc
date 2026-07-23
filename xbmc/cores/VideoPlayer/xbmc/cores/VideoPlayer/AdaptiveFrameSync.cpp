#include "AdaptiveFrameSync.h"
#include "utils/log.h"

#ifdef TARGET_ANDROID
#include <android/api-level.h>
#endif

using namespace KODI::VIDEO;

bool AdaptiveFrameSync::IsEnabled()
{
  // Draft: hardcode false for now. We'll hook to a real setting later.
  return false;
}

AFSProbe AdaptiveFrameSync::ProbeCapabilities()
{
  AFSProbe p;
#ifdef TARGET_ANDROID
  // TODO: query DisplayManager/SurfaceFlinger via JNI for real modes
  // Stub values to prove the pipeline and logs work
  p.displayHasNative24 = false;
  p.displayHas120 = true;
  p.device = "Android-Stub";
#else
  p.device = "Non-Android";
#endif
  p.passthroughPossible = true; // optimistic default
  return p;
}

AFSPolicy AdaptiveFrameSync::SelectPolicy(const AFSProbe& p)
{
  if (!IsEnabled()) return AFSPolicy::DISABLED;
  if (p.displayHasNative24) return AFSPolicy::TRUE_MATCH;
  if (p.displayHas120)      return AFSPolicy::EXACT_MULTIPLE;
  return AFSPolicy::NEAR_MULTIPLE;
}

void AdaptiveFrameSync::LogDecision(const AFSProbe& p, AFSPolicy policy)
{
  const char* name = "DISABLED";
  switch (policy)
  {
    case AFSPolicy::TRUE_MATCH:     name = "TRUE_MATCH"; break;
    case AFSPolicy::EXACT_MULTIPLE: name = "EXACT_MULTIPLE"; break;
    case AFSPolicy::NEAR_MULTIPLE:  name = "NEAR_MULTIPLE"; break;
    case AFSPolicy::SMOOTH_SYNC:    name = "SMOOTH_SYNC"; break;
    case AFSPolicy::DISABLED:       name = "DISABLED"; break;
  }

  CLog::Log(LOGINFO,
            "AFS: device='{}' native24={} 120hz={} passthrough={} -> policy={}",
            p.device, p.displayHasNative24, p.displayHas120, p.passthroughPossible, name);
}
