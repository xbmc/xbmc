/*
 * VSTPlugin3.cpp — VST3 plugin wrapper implementing IVSTPlugin
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include "VSTPlugin3.h"

#include "public.sdk/source/vst/hosting/module.h"
#include "public.sdk/source/vst/hosting/plugprovider.h"
#include "public.sdk/source/vst/hosting/processdata.h"
#include "public.sdk/source/vst/hosting/parameterchanges.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "pluginterfaces/vst/ivsteditcontroller.h"
#include "pluginterfaces/vst/ivstcomponent.h"
#include "pluginterfaces/vst/ivstparameterchanges.h"
#include "pluginterfaces/base/ibstream.h"
#include "base/source/fstreamer.h"
#include "base/source/fstring.h"

// Steinberg MemoryStream for state serialization
#include "pluginterfaces/base/ibstream.h"
#include "public.sdk/source/common/memorystream.h"

#include <algorithm>
#include <cstring>
#include <cwchar>
#include <string>

// ---------------------------------------------------------------------------
// Internal logging shim — replace with your real logging layer if desired.
// ---------------------------------------------------------------------------
#include <cstdio>
#define VST3_LOG(fmt, ...) std::fprintf(stderr, "[VSTPlugin3] " fmt "\n", ##__VA_ARGS__)

// ---------------------------------------------------------------------------
// Helper: convert a Steinberg String128 (char16_t[128]) to std::string (UTF-8
// via a narrow ASCII pass — sufficient for parameter names in practice).
// ---------------------------------------------------------------------------
static std::string string128ToStdString(const Steinberg::Vst::String128 src)
{
    // Use Steinberg's own string conversion utilities if available; fall back
    // to a safe ASCII-range narrowing loop that works for all typical names.
    std::string result;
    result.reserve(128);
    for (int i = 0; i < 128 && src[i] != 0; ++i)
    {
        char16_t c = src[i];
        if (c < 0x80)
            result += static_cast<char>(c);
        else
            result += '?';  // non-ASCII: substitute
    }
    return result;
}

// ===========================================================================
// Construction / destruction
// ===========================================================================

VSTPlugin3::VSTPlugin3(const std::string& path)
    : m_path(path)
{
}

VSTPlugin3::~VSTPlugin3()
{
    unload();
}

// ===========================================================================
// load()
// ===========================================================================

bool VSTPlugin3::load(double sampleRate, int maxBlockSize, int numChannels)
{
    if (m_loaded)
        unload();

    m_sampleRate  = sampleRate;
    m_blockSize   = maxBlockSize;
    m_numChannels = numChannels;

    // -----------------------------------------------------------------------
    // 1. Load the VST3 module (DLL / bundle)
    // -----------------------------------------------------------------------
    std::string errMsg;
    m_module = VST3::Hosting::Module::create(m_path, errMsg);
    if (!m_module)
    {
        VST3_LOG("Module::create failed for '%s': %s", m_path.c_str(), errMsg.c_str());
        return false;
    }

    // -----------------------------------------------------------------------
    // 2. Obtain the plugin factory
    // -----------------------------------------------------------------------
    auto factory = m_module->getFactory();

    // -----------------------------------------------------------------------
    // 3. Create a PlugProvider — it handles component creation and cleanup
    // -----------------------------------------------------------------------
    m_provider = Steinberg::owned(
        new Steinberg::Vst::PlugProvider(factory, nullptr, true));
    if (!m_provider)
    {
        VST3_LOG("Failed to create PlugProvider for '%s'", m_path.c_str());
        m_module.reset();
        return false;
    }

    // -----------------------------------------------------------------------
    // 4. Setup the plugin (instantiates the component, controller, etc.)
    // -----------------------------------------------------------------------
    if (m_provider->setupPlugin(Steinberg::Vst::kDefaultFactoryFlags) !=
        Steinberg::kResultOk)
    {
        VST3_LOG("PlugProvider::setupPlugin failed for '%s'", m_path.c_str());
        m_provider = nullptr;
        m_module.reset();
        return false;
    }

    // -----------------------------------------------------------------------
    // 5. Retrieve IComponent
    // -----------------------------------------------------------------------
    m_component = m_provider->getComponent();
    if (!m_component)
    {
        VST3_LOG("PlugProvider::getComponent returned null for '%s'", m_path.c_str());
        m_provider = nullptr;
        m_module.reset();
        return false;
    }

    // -----------------------------------------------------------------------
    // 6. Retrieve IAudioProcessor via FUnknownPtr cast
    // -----------------------------------------------------------------------
    {
        Steinberg::FUnknownPtr<Steinberg::Vst::IAudioProcessor> procPtr(m_component);
        m_processor = procPtr;
    }
    if (!m_processor)
    {
        VST3_LOG("IComponent does not implement IAudioProcessor: '%s'", m_path.c_str());
        m_component = nullptr;
        m_provider  = nullptr;
        m_module.reset();
        return false;
    }

    // -----------------------------------------------------------------------
    // 7. Retrieve IEditController
    //    Try querying from m_component first (single-component plugins), then
    //    fall back to createController() on the factory.
    // -----------------------------------------------------------------------
    {
        // Try direct QI first (some plugins implement both in one object).
        Steinberg::FUnknownPtr<Steinberg::Vst::IEditController> ctrlPtr(m_component);
        m_controller = ctrlPtr;
    }

    if (!m_controller)
    {
        // Ask the PlugProvider for a separately-created controller.
        m_controller = m_provider->getController();
    }

    if (!m_controller)
    {
        VST3_LOG("Warning: no IEditController found for '%s' — param support disabled",
                 m_path.c_str());
        // Not fatal — we continue without controller (parameter support absent).
    }

    // -----------------------------------------------------------------------
    // 8. Configure ProcessSetup
    // -----------------------------------------------------------------------
    m_processSetup.processMode        = Steinberg::Vst::kRealtime;
    m_processSetup.symbolicSampleSize = Steinberg::Vst::kSample32;
    m_processSetup.maxSamplesPerBlock = maxBlockSize;
    m_processSetup.sampleRate         = sampleRate;

    // -----------------------------------------------------------------------
    // 9. Communicate the setup to the processor
    // -----------------------------------------------------------------------
    if (m_processor->setupProcessing(m_processSetup) != Steinberg::kResultOk)
    {
        VST3_LOG("IAudioProcessor::setupProcessing failed for '%s'", m_path.c_str());
        m_controller = nullptr;
        m_processor  = nullptr;
        m_component  = nullptr;
        m_provider   = nullptr;
        m_module.reset();
        return false;
    }

    // -----------------------------------------------------------------------
    // 10. Activate buses
    // -----------------------------------------------------------------------
    if (!setupBuses())
    {
        VST3_LOG("setupBuses failed for '%s'", m_path.c_str());
        m_controller = nullptr;
        m_processor  = nullptr;
        m_component  = nullptr;
        m_provider   = nullptr;
        m_module.reset();
        return false;
    }

    // -----------------------------------------------------------------------
    // 11. Activate the component
    // -----------------------------------------------------------------------
    if (m_component->setActive(true) != Steinberg::kResultOk)
    {
        VST3_LOG("IComponent::setActive(true) failed for '%s'", m_path.c_str());
        m_controller = nullptr;
        m_processor  = nullptr;
        m_component  = nullptr;
        m_provider   = nullptr;
        m_module.reset();
        return false;
    }

    // -----------------------------------------------------------------------
    // 12. Start processing
    // -----------------------------------------------------------------------
    if (m_processor->setProcessing(true) != Steinberg::kResultOk)
    {
        // Some plugins return kNotImplemented here — treat that as OK.
        VST3_LOG("Warning: IAudioProcessor::setProcessing(true) did not return kResultOk"
                 " for '%s' (may be normal)", m_path.c_str());
    }

    // -----------------------------------------------------------------------
    // 13. Build parameter index table
    // -----------------------------------------------------------------------
    if (m_controller)
        buildParamIndex();

    // -----------------------------------------------------------------------
    // Populate name / vendor from factory info
    // -----------------------------------------------------------------------
    {
        auto& classInfoList = factory.classInfos();
        for (const auto& ci : classInfoList)
        {
            if (ci.category() == kVstAudioEffectClass)
            {
                m_name   = ci.name();
                m_vendor = ci.vendor();
                break;
            }
        }
        if (m_name.empty())
        {
            // Fallback: derive name from module path
            auto pos = m_path.find_last_of("/\\");
            m_name = (pos != std::string::npos) ? m_path.substr(pos + 1) : m_path;
        }
    }

    m_loaded = true;
    VST3_LOG("Loaded '%s' (vendor: '%s', params: %d, sr: %.0f, bs: %d, ch: %d)",
             m_name.c_str(), m_vendor.c_str(),
             static_cast<int>(m_paramIDByIndex.size()),
             sampleRate, maxBlockSize, numChannels);
    return true;
}

// ===========================================================================
// unload()
// ===========================================================================

void VSTPlugin3::unload()
{
    if (!m_loaded && !m_processor && !m_component)
        return;

    // 1. Stop processing
    if (m_processor)
    {
        m_processor->setProcessing(false);
    }

    // 2. Deactivate component
    if (m_component)
    {
        m_component->setActive(false);
    }

    // 3. Release smart pointers in reverse acquisition order
    m_controller = nullptr;
    m_processor  = nullptr;
    m_component  = nullptr;
    m_provider   = nullptr;

    // 4. Unload the module (closes the DLL)
    m_module.reset();

    // 5. Clear derived state
    m_paramIDByIndex.clear();
    m_name.clear();
    m_vendor.clear();

    m_loaded = false;
    VST3_LOG("Unloaded '%s'", m_path.c_str());
}

// ===========================================================================
// process()
// ===========================================================================

int VSTPlugin3::process(float** in, float** out, int samples)
{
    if (!m_loaded || !m_processor)
        return 0;

    // 1. Drain the parameter automation queue into a stack-local container
    //    so it is automatically reset (destroyed) at the end of this block.
    Steinberg::Vst::ParameterChanges blockParamChanges;
    drainParamQueue(blockParamChanges);
    if (blockParamChanges.getParameterCount() > 0)
        m_processData.inputParameterChanges = &blockParamChanges;

    // 2. Bypass: copy input to output and return early.
    if (m_bypassed)
    {
        for (int ch = 0; ch < m_numChannels; ++ch)
            std::memcpy(out[ch], in[ch], sizeof(float) * static_cast<size_t>(samples));
        return samples;
    }

    // 3. Set up HostProcessData for this block.
    m_processData.numSamples = samples;

    // Input bus
    if (m_processData.numInputs > 0)
    {
        m_processData.inputs[0].numChannels      = m_numChannels;
        m_processData.inputs[0].channelBuffers32 = in;
        m_processData.inputs[0].silenceFlags     = 0;
    }

    // Output bus
    if (m_processData.numOutputs > 0)
    {
        m_processData.outputs[0].numChannels      = m_numChannels;
        m_processData.outputs[0].channelBuffers32 = out;
        m_processData.outputs[0].silenceFlags     = 0;
    }

    // 4. Hand off to the plugin.
    m_processor->process(m_processData);

    // 5. Clear per-block parameter changes so the next block starts clean.
    m_processData.inputParameterChanges = nullptr;

    return samples;
}

// ===========================================================================
// drainParamQueue()  — called from audio thread
// ===========================================================================

void VSTPlugin3::drainParamQueue(Steinberg::Vst::ParameterChanges& paramChanges)
{
    if (!m_controller)
        return;

    // Drain the ring buffer into the caller-provided ParameterChanges container.
    // The container is a stack-local in process() so it is fresh every audio
    // block — no need to reset it here.
    ParamChange3 change;
    while (m_paramQueue.pop(change))
    {
        Steinberg::int32 queueIndex = 0;
        auto* queue = paramChanges.addParameterData(
            static_cast<Steinberg::Vst::ParamID>(change.paramID), queueIndex);
        if (queue)
        {
            Steinberg::int32 pointIndex = 0;
            queue->addPoint(change.sampleOffset, change.value, pointIndex);
        }
    }
}

// ===========================================================================
// setParameter() / getParameter() / getParameterCount() / getParameterName()
// ===========================================================================

void VSTPlugin3::setParameter(int index, float value)
{
    if (index < 0 || static_cast<size_t>(index) >= m_paramIDByIndex.size())
        return;

    ParamChange3 change{
        m_paramIDByIndex[static_cast<size_t>(index)],
        static_cast<double>(value),
        0  // sampleOffset = 0 (block-start accuracy)
    };
    m_paramQueue.push(change);
}

float VSTPlugin3::getParameter(int index) const
{
    if (!m_controller)
        return 0.0f;
    if (index < 0 || static_cast<size_t>(index) >= m_paramIDByIndex.size())
        return 0.0f;

    return static_cast<float>(
        m_controller->getParamNormalized(
            static_cast<Steinberg::Vst::ParamID>(
                m_paramIDByIndex[static_cast<size_t>(index)])));
}

int VSTPlugin3::getParameterCount() const
{
    return static_cast<int>(m_paramIDByIndex.size());
}

std::string VSTPlugin3::getParameterName(int index) const
{
    if (!m_controller)
        return {};
    if (index < 0 || static_cast<size_t>(index) >= m_paramIDByIndex.size())
        return {};

    Steinberg::Vst::ParameterInfo info{};
    if (m_controller->getParameterInfo(index, info) != Steinberg::kResultOk)
        return {};

    return string128ToStdString(info.title);
}

// ===========================================================================
// getLatencySamples()
// ===========================================================================

int VSTPlugin3::getLatencySamples() const
{
    if (!m_processor)
        return 0;
    return static_cast<int>(m_processor->getLatencySamples());
}

// ===========================================================================
// saveState() / loadState()
// ===========================================================================

std::vector<uint8_t> VSTPlugin3::saveState() const
{
    if (!m_loaded || !m_component)
        return {};

    // Use Steinberg's MemoryStream as a writable IBStream.
    Steinberg::MemoryStream stream;

    // Serialize component (audio/DSP) state.
    if (m_component->getState(&stream) != Steinberg::kResultOk)
    {
        VST3_LOG("Warning: IComponent::getState failed for '%s'", m_path.c_str());
    }

    // Serialize controller (GUI/parameter) state if available.
    if (m_controller)
    {
        Steinberg::MemoryStream ctrlStream;
        if (m_controller->getState(&ctrlStream) == Steinberg::kResultOk)
        {
            // Append controller bytes after component bytes.
            // We prefix the controller block with a 4-byte little-endian length
            // so loadState() can split the two regions.
            Steinberg::int64 ctrlSize = ctrlStream.getSize();
            auto ctrlSizeU32 = static_cast<uint32_t>(ctrlSize);

            // Write controller size marker into the main stream.
            Steinberg::IBStreamer mainStreamer(&stream, Steinberg::kLittleEndian);
            mainStreamer.writeInt32u(ctrlSizeU32);

            // Append controller bytes.
            const uint8_t* ctrlData =
                reinterpret_cast<const uint8_t*>(ctrlStream.getData());
            stream.write(const_cast<uint8_t*>(ctrlData),
                         static_cast<Steinberg::int32>(ctrlSize), nullptr);
        }
        else
        {
            // Write zero-length marker so the format stays consistent.
            Steinberg::IBStreamer mainStreamer(&stream, Steinberg::kLittleEndian);
            mainStreamer.writeInt32u(0u);
        }
    }

    // Copy stream contents into a plain byte vector.
    Steinberg::int64 totalSize = stream.getSize();
    if (totalSize <= 0)
        return {};

    const uint8_t* rawData = reinterpret_cast<const uint8_t*>(stream.getData());
    return std::vector<uint8_t>(rawData, rawData + static_cast<size_t>(totalSize));
}

bool VSTPlugin3::loadState(const std::vector<uint8_t>& data)
{
    if (!m_loaded || !m_component || data.empty())
        return false;

    // Wrap the input bytes in a MemoryStream.
    // MemoryStream::open takes a non-const void*, so we need a copy or
    // a const_cast.  The SDK treats it as read-only in read mode, so
    // const_cast is safe here.
    Steinberg::MemoryStream stream(
        const_cast<void*>(static_cast<const void*>(data.data())),
        static_cast<Steinberg::int32>(data.size()));

    // Restore component state.
    // The stream is positioned at the start; getState() when loading is via
    // setState().
    bool ok = (m_component->setState(&stream) == Steinberg::kResultOk);
    if (!ok)
        VST3_LOG("Warning: IComponent::setState failed for '%s'", m_path.c_str());

    // Restore controller state if a length marker is present.
    if (m_controller)
    {
        Steinberg::IBStreamer streamer(&stream, Steinberg::kLittleEndian);
        uint32_t ctrlSize = 0;
        if (streamer.readInt32u(ctrlSize) && ctrlSize > 0)
        {
            std::vector<uint8_t> ctrlBytes(ctrlSize);
            Steinberg::int32 numBytesRead = 0;
            if (stream.read(ctrlBytes.data(),
                            static_cast<Steinberg::int32>(ctrlSize),
                            &numBytesRead) == Steinberg::kResultOk
                && static_cast<uint32_t>(numBytesRead) == ctrlSize)
            {
                Steinberg::MemoryStream ctrlStream(
                    ctrlBytes.data(),
                    static_cast<Steinberg::int32>(ctrlSize));
                if (m_controller->setState(&ctrlStream) != Steinberg::kResultOk)
                    VST3_LOG("Warning: IEditController::setState failed for '%s'",
                             m_path.c_str());
            }
        }
    }

    return ok;
}

// ===========================================================================
// buildParamIndex()  — private
// ===========================================================================

void VSTPlugin3::buildParamIndex()
{
    if (!m_controller)
        return;

    int count = m_controller->getParameterCount();
    m_paramIDByIndex.resize(static_cast<size_t>(count));

    for (int i = 0; i < count; ++i)
    {
        Steinberg::Vst::ParameterInfo info{};
        m_controller->getParameterInfo(i, info);
        m_paramIDByIndex[static_cast<size_t>(i)] = info.id;
    }
}

// ===========================================================================
// setupBuses()  — private
// ===========================================================================

bool VSTPlugin3::setupBuses()
{
    if (!m_component)
        return false;

    // Activate the first stereo audio input bus (index 0).
    Steinberg::tresult inResult = m_component->activateBus(
        Steinberg::Vst::kAudio,
        Steinberg::Vst::kInput,
        0,      // bus index
        true);  // active

    if (inResult != Steinberg::kResultOk && inResult != Steinberg::kNotImplemented)
    {
        VST3_LOG("Warning: activateBus(input) returned %d for '%s'",
                 static_cast<int>(inResult), m_path.c_str());
    }

    // Activate the first stereo audio output bus (index 0).
    Steinberg::tresult outResult = m_component->activateBus(
        Steinberg::Vst::kAudio,
        Steinberg::Vst::kOutput,
        0,
        true);

    if (outResult != Steinberg::kResultOk && outResult != Steinberg::kNotImplemented)
    {
        VST3_LOG("Warning: activateBus(output) returned %d for '%s'",
                 static_cast<int>(outResult), m_path.c_str());
    }

    // Prepare HostProcessData bus arrays.
    m_processData.prepare(*m_component, m_blockSize,
                          Steinberg::Vst::kSample32);

    return true;
}

// ===========================================================================
// Editor support — VST3 native editor window via IPlugView
// ===========================================================================

bool VSTPlugin3::hasEditor() const
{
    if (!m_loaded || !m_controller)
        return false;

    // Try to create a view to check if the plugin has an editor.
    // We immediately release it — the real view is created in openEditor().
    auto* view = m_controller->createView("editor");
    if (view)
    {
        view->release();
        return true;
    }
    return false;
}

bool VSTPlugin3::openEditor(void* parentWindow)
{
    if (!m_loaded || !m_controller)
        return false;

    // Close any existing editor first
    closeEditor();

    auto* view = m_controller->createView("editor");
    if (!view)
    {
        VST3_LOG("openEditor: createView returned nullptr for '%s'", m_path.c_str());
        return false;
    }

    m_plugView = Steinberg::owned(view);

    if (m_plugView->isPlatformTypeSupported("HWND") != Steinberg::kResultOk)
    {
        VST3_LOG("openEditor: HWND platform not supported for '%s'", m_path.c_str());
        m_plugView = nullptr;
        return false;
    }

    if (m_plugView->attached(parentWindow, "HWND") != Steinberg::kResultOk)
    {
        VST3_LOG("openEditor: attached() failed for '%s'", m_path.c_str());
        m_plugView = nullptr;
        return false;
    }

    return true;
}

void VSTPlugin3::closeEditor()
{
    if (m_plugView)
    {
        m_plugView->removed();
        m_plugView = nullptr;
    }
}

bool VSTPlugin3::getEditorSize(int& width, int& height) const
{
    if (!m_loaded || !m_controller)
        return false;

    // Create a temporary view just to query size if no view is currently open
    Steinberg::IPlugView* view = nullptr;
    bool tempView = false;

    if (m_plugView)
    {
        view = m_plugView.get();
    }
    else
    {
        view = m_controller->createView("editor");
        if (!view)
            return false;
        tempView = true;
    }

    Steinberg::ViewRect rect{};
    Steinberg::tresult result = view->getSize(&rect);

    if (tempView)
        view->release();

    if (result != Steinberg::kResultOk)
        return false;

    width  = static_cast<int>(rect.right  - rect.left);
    height = static_cast<int>(rect.bottom - rect.top);
    return (width > 0 && height > 0);
}

void VSTPlugin3::idleEditor()
{
    // VST3 editors are self-pumping via COM/message loop — no idle needed.
}
