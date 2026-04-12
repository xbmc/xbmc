/*
 * VSTHostContext.cpp — IHostApplication implementation for Kodi VST3 host
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include "VSTHostContext.h"

#include "pluginterfaces/vst/ivsthostapplication.h"

#include <cstring>   // std::wcsncpy / memcpy
#include <cwchar>    // wcslen

// ---------------------------------------------------------------------------
// IHostApplication::getName
// ---------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API
VSTHostContext::getName(Steinberg::Vst::String128 name)
{
    // String128 is char16_t[128] (Steinberg::char16 is char16_t on all
    // platforms the SDK supports).  We copy the literal using a simple loop
    // so we don't need any locale/codec machinery.
    static const char16_t kHostName[] = u"Kodi VST Host";
    constexpr size_t kLen = sizeof(kHostName) / sizeof(kHostName[0]);  // includes '\0'

    static_assert(kLen <= 128, "Host name too long for String128");

    for (size_t i = 0; i < kLen; ++i)
        name[i] = kHostName[i];

    return Steinberg::kResultOk;
}

// ---------------------------------------------------------------------------
// IHostApplication::createInstance
// ---------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API
VSTHostContext::createInstance(Steinberg::TUID /*cid*/,
                               Steinberg::TUID /*iid*/,
                               void**          obj)
{
    if (obj)
        *obj = nullptr;
    return Steinberg::kNotImplemented;
}

// ---------------------------------------------------------------------------
// FUnknown::queryInterface
// ---------------------------------------------------------------------------
Steinberg::tresult PLUGIN_API
VSTHostContext::queryInterface(const Steinberg::TUID iid, void** obj)
{
    // Respond to IHostApplication queries so PlugProvider can cast us.
    if (Steinberg::FUnknownPrivate::iidEqual(
            iid, Steinberg::Vst::IHostApplication::iid))
    {
        *obj = static_cast<Steinberg::Vst::IHostApplication*>(this);
        return Steinberg::kResultOk;
    }

    // Respond to FUnknown queries (base interface).
    if (Steinberg::FUnknownPrivate::iidEqual(iid, Steinberg::FUnknown::iid))
    {
        *obj = static_cast<Steinberg::FUnknown*>(this);
        return Steinberg::kResultOk;
    }

    *obj = nullptr;
    return Steinberg::kNoInterface;
}
