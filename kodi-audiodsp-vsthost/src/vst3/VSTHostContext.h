#pragma once
/*
 * VSTHostContext.h — IHostApplication implementation for Kodi VST3 host
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include "pluginterfaces/vst/ivsthostapplication.h"
#include "pluginterfaces/vst/ivstmessage.h"
#include <string>

/// Minimal IHostApplication implementation required by the VST3 SDK's
/// PlugProvider to identify the host.  Only getName() and queryInterface()
/// carry real logic; reference counting is trivially inlined because the
/// object is always owned by VSTPlugin3 on the stack / as a member.
class VSTHostContext : public Steinberg::Vst::IHostApplication
{
public:
    // -------------------------------------------------------------------------
    // IHostApplication
    // -------------------------------------------------------------------------

    /// Returns the host's human-readable name ("Kodi VST Host").
    Steinberg::tresult PLUGIN_API getName(Steinberg::Vst::String128 name) override;

    /// Plugin can request host-side service objects here; we return
    /// kNotImplemented for everything — no message-bus, no progress UI, etc.
    Steinberg::tresult PLUGIN_API createInstance(Steinberg::TUID cid,
                                                  Steinberg::TUID iid,
                                                  void**          obj) override;

    // -------------------------------------------------------------------------
    // FUnknown — trivial ref-counting (stack / member lifetime)
    // -------------------------------------------------------------------------
    Steinberg::uint32 PLUGIN_API addRef()  override { return 1; }
    Steinberg::uint32 PLUGIN_API release() override { return 1; }
    Steinberg::tresult PLUGIN_API queryInterface(const Steinberg::TUID iid,
                                                  void**               obj) override;
};
