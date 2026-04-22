#pragma once
/*
 * VSTPlugin3.h — VST3 plugin wrapper implementing IVSTPlugin
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 *
 * Lifecycle state machine:
 *   constructed → load() → [process() loop] → unload() → destroyed
 *                      ↑__________________________|  (reinitialize)
 */
#include "plugin/IVSTPlugin.h"
#include "util/ParamQueue.h"
#include "vst3/VSTHostContext.h"

#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "public.sdk/source/vst/hosting/processdata.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstprocesscontext.h"
#include "pluginterfaces/gui/iplugview.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"

#include <string>
#include <vector>
#include <cstdint>

/// Concrete VST3 plugin host wrapper.
///
/// One instance corresponds to one loaded VST3 module (.vst3 bundle / DLL).
/// The plugin path is passed at construction time; format details and module
/// name/vendor strings are populated during load().
class VSTPlugin3 : public IVSTPlugin
{
public:
    /// @param path  Absolute path to the .vst3 file or bundle directory.
    explicit VSTPlugin3(const std::string& path);
    ~VSTPlugin3() override;

    // -------------------------------------------------------------------------
    // IVSTPlugin — lifecycle
    // -------------------------------------------------------------------------
    bool load(double sampleRate, int maxBlockSize, int numChannels) override;
    void unload() override;

    // -------------------------------------------------------------------------
    // IVSTPlugin — audio processing
    // -------------------------------------------------------------------------
    int process(float** in, float** out, int samples) override;

    // -------------------------------------------------------------------------
    // IVSTPlugin — parameters
    // -------------------------------------------------------------------------
    void        setParameter(int index, float value) override;
    float       getParameter(int index) const override;
    int         getParameterCount() const override;
    std::string getParameterName(int index) const override;

    // -------------------------------------------------------------------------
    // IVSTPlugin — query
    // -------------------------------------------------------------------------
    int         getLatencySamples() const override;
    std::string getName()           const override { return m_name; }
    std::string getVendorName()     const override { return m_vendor; }
    std::string getPath()           const override { return m_path; }
    bool        isLoaded()          const override { return m_loaded; }
    PluginFormat getFormat()        const override { return PluginFormat::VST3; }

    // -------------------------------------------------------------------------
    // IVSTPlugin — state persistence
    // -------------------------------------------------------------------------
    std::vector<uint8_t> saveState()                              const override;
    bool                 loadState(const std::vector<uint8_t>& data) override;

    // -------------------------------------------------------------------------
    // IVSTPlugin — editor
    // -------------------------------------------------------------------------
    bool hasEditor()                              const override;
    bool openEditor(void* parentWindow)                 override;
    void closeEditor()                                  override;
    bool getEditorSize(int& width, int& height)   const override;
    void idleEditor()                                   override;

private:
    // -------------------------------------------------------------------------
    // Private helpers
    // -------------------------------------------------------------------------

    /// Populate m_paramIDByIndex from the IEditController parameter list.
    void buildParamIndex();

    /// Drain m_paramQueue into the provided ParameterChanges container.
    /// The container is allocated on the stack in process() so it is
    /// automatically destroyed (reset) at the end of each audio block.
    void drainParamQueue(Steinberg::Vst::ParameterChanges& paramChanges);

    /// Activate the default audio input and output buses on m_component.
    bool setupBuses();

    // -------------------------------------------------------------------------
    // VST3 SDK smart-pointer members
    // -------------------------------------------------------------------------
    VST3::Hosting::Module::Ptr                        m_module;
    Steinberg::IPtr<Steinberg::Vst::PlugProvider>     m_provider;
    Steinberg::IPtr<Steinberg::Vst::IComponent>       m_component;
    Steinberg::IPtr<Steinberg::Vst::IAudioProcessor>  m_processor;
    Steinberg::IPtr<Steinberg::Vst::IEditController>  m_controller;

    /// Cached IPlugView for the VST3 editor (created on first openEditor call).
    Steinberg::IPtr<Steinberg::IPlugView>              m_plugView;

    // -------------------------------------------------------------------------
    // Host-side objects
    // -------------------------------------------------------------------------
    VSTHostContext                  m_hostContext;
    Steinberg::Vst::ProcessSetup    m_processSetup{};
    Steinberg::Vst::HostProcessData m_processData;

    // Parameter automation ring buffer (GUI→audio thread).
    RingBuffer<ParamChange3, 256>   m_paramQueue;

    /// Pre-allocated parameter changes container reused every audio block
    /// to avoid per-block heap allocation in the real-time path.
    Steinberg::Vst::ParameterChanges m_blockParamChanges;

    /// Maps integer param index (0..N-1) → VST3 param ID (plugin-defined uint32).
    std::vector<uint32_t>           m_paramIDByIndex;

    // -------------------------------------------------------------------------
    // Metadata / configuration
    // -------------------------------------------------------------------------
    std::string m_path;
    std::string m_name;
    std::string m_vendor;

    double m_sampleRate  = 44100.0;
    int    m_blockSize   = 512;
    int    m_numChannels = 2;
    bool   m_loaded      = false;
    bool   m_hasEditor   = false;
};
