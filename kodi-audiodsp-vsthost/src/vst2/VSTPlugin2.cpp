/*
 * VSTPlugin2.cpp — VST2 plugin host wrapper implementation
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 *
 * Key implementation notes:
 *  - effSetSampleRate passes the rate through `opt` (float), NOT `value`.
 *  - getParameterName allocates a 64-byte buffer even though the VST2 spec
 *    says 8 bytes; almost every real plugin overruns the 8-byte limit.
 *  - callPluginMainSafe() uses SEH (__try/__except) and is kept as its own
 *    function with no C++ objects in scope to avoid MSVC C4509 warnings
 *    ("nonstandard extension used: SEH and C++ destructors").
 */

#include "VSTPlugin2.h"

#include <algorithm>
#include <cstring>
#include <filesystem>

// ---------------------------------------------------------------------------
// Constructor / Destructor
// ---------------------------------------------------------------------------

VSTPlugin2::VSTPlugin2(const std::string& path)
    : m_path(path)
{
}

VSTPlugin2::~VSTPlugin2()
{
    unload();
}

// ---------------------------------------------------------------------------
// callPluginMainSafe — SEH wrapper
//
// This MUST be a standalone function (not a lambda, not inlined into load())
// because MSVC prohibits mixing C++ object destructors and __try/__except in
// the same function scope.  By isolating the SEH here we keep the rest of
// load() clean.
// ---------------------------------------------------------------------------

AEffect* VSTPlugin2::callPluginMainSafe(VSTENTRYPROC proc)
{
    AEffect* result = nullptr;
    __try {
        result = proc(staticAudioMaster);
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        result = nullptr;
    }
    return result;
}

// ---------------------------------------------------------------------------
// load()
// ---------------------------------------------------------------------------

bool VSTPlugin2::load(double sampleRate, int maxBlockSize, int numChannels)
{
    if (m_loaded)
        unload();

    m_sampleRate  = sampleRate;
    m_blockSize   = maxBlockSize;
    m_numChannels = numChannels;

    // --- 1. Load the DLL -------------------------------------------------------
    // Convert UTF-8 path to wide string for LoadLibraryW
    int wlen = MultiByteToWideChar(CP_UTF8, 0, m_path.c_str(), -1, nullptr, 0);
    std::wstring wpath(wlen, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, m_path.c_str(), -1, wpath.data(), wlen);

    m_hModule = LoadLibraryW(wpath.c_str());
    if (!m_hModule)
        return false;

    // --- 2. Locate entry point -------------------------------------------------
    VSTENTRYPROC proc = reinterpret_cast<VSTENTRYPROC>(
        GetProcAddress(m_hModule, "VSTPluginMain"));
    if (!proc)
        proc = reinterpret_cast<VSTENTRYPROC>(
            GetProcAddress(m_hModule, "main"));

    if (!proc) {
        FreeLibrary(m_hModule);
        m_hModule = nullptr;
        return false;
    }

    // --- 3. Instantiate the plugin (SEH-guarded) --------------------------------
    m_effect = callPluginMainSafe(proc);
    if (!m_effect) {
        FreeLibrary(m_hModule);
        m_hModule = nullptr;
        return false;
    }

    // --- 4. Validate magic number -----------------------------------------------
    if (m_effect->magic != kEffectMagic) {
        FreeLibrary(m_hModule);
        m_hModule = nullptr;
        m_effect  = nullptr;
        return false;
    }

    // --- 5. Store host context so audioMaster callback can recover this instance
    m_effect->user = this;

    // --- 6. Open the plugin -----------------------------------------------------
    m_effect->dispatcher(m_effect, effOpen, 0, 0, nullptr, 0.0f);

    // --- 7. Set sample rate — CRITICAL: pass via `opt` (float), NOT `value`! ----
    m_effect->dispatcher(m_effect, effSetSampleRate, 0, 0, nullptr,
                         static_cast<float>(m_sampleRate));

    // --- 8. Set block size — pass via `value` ------------------------------------
    m_effect->dispatcher(m_effect, effSetBlockSize, 0,
                         static_cast<VstIntPtr>(m_blockSize), nullptr, 0.0f);

    // --- 9. Resume (mains active) ------------------------------------------------
    m_effect->dispatcher(m_effect, effMainsChanged, 0, 1, nullptr, 0.0f);

    // --- 10. Retrieve plugin name -------------------------------------------------
    {
        char nameBuf[64] = {};
        m_effect->dispatcher(m_effect, effGetEffectName, 0, 0, nameBuf, 0.0f);
        if (nameBuf[0] != '\0') {
            m_name = nameBuf;
        } else {
            // Fall back to the filename without extension
            try {
                m_name = std::filesystem::path(m_path).stem().string();
            } catch (...) {
                m_name = m_path;
            }
        }
    }

    // --- 11. Retrieve vendor string ----------------------------------------------
    {
        char vendorBuf[64] = {};
        m_effect->dispatcher(m_effect, effGetVendorString, 0, 0, vendorBuf, 0.0f);
        m_vendor = vendorBuf;
    }

    // --- 12. Allocate scratch buffers --------------------------------------------
    allocateScratchBuffers();

    // --- 13. Mark as loaded and active ------------------------------------------
    m_loaded = true;
    m_active = true;

    return true;
}

// ---------------------------------------------------------------------------
// unload()
// ---------------------------------------------------------------------------

void VSTPlugin2::unload()
{
    if (m_effect) {
        if (m_active) {
            m_effect->dispatcher(m_effect, effMainsChanged, 0, 0, nullptr, 0.0f);
            m_active = false;
        }
        m_effect->dispatcher(m_effect, effClose, 0, 0, nullptr, 0.0f);
        m_effect = nullptr;
    }

    if (m_hModule) {
        FreeLibrary(m_hModule);
        m_hModule = nullptr;
    }

    m_loaded = false;

    // Release scratch memory
    m_inputBufs.clear();
    m_outputBufs.clear();
    m_inputPtrs.clear();
    m_outputPtrs.clear();
}

// ---------------------------------------------------------------------------
// allocateScratchBuffers()
// ---------------------------------------------------------------------------

void VSTPlugin2::allocateScratchBuffers()
{
    m_inputBufs.assign(m_numChannels, std::vector<float>(m_blockSize, 0.0f));
    m_outputBufs.assign(m_numChannels, std::vector<float>(m_blockSize, 0.0f));

    m_inputPtrs.resize(m_numChannels);
    m_outputPtrs.resize(m_numChannels);

    for (int i = 0; i < m_numChannels; ++i) {
        m_inputPtrs[i]  = m_inputBufs[i].data();
        m_outputPtrs[i] = m_outputBufs[i].data();
    }
}

// ---------------------------------------------------------------------------
// drainParamQueue() — called at the top of process() on the audio thread
// ---------------------------------------------------------------------------

void VSTPlugin2::drainParamQueue()
{
    ParamChange2 change;
    while (m_paramQueue.pop(change)) {
        // VST2 parameter set is a direct function-pointer call, NOT a
        // dispatcher opcode.  effSetParameter does not exist in the ABI.
        if (m_effect)
            m_effect->setParameter(m_effect, change.index, change.value);
    }
}

// ---------------------------------------------------------------------------
// process()
// ---------------------------------------------------------------------------

int VSTPlugin2::process(float** in, float** out, int samples)
{
    // Apply queued parameter changes before touching audio
    drainParamQueue();

    // Bypass: pass input straight through
    if (!m_loaded || m_bypassed) {
        for (int ch = 0; ch < m_numChannels; ++ch)
            std::copy(in[ch], in[ch] + samples, out[ch]);
        return samples;
    }

    // Determine actual I/O channel counts reported by the plugin
    const int pluginIns  = m_effect->numInputs;
    const int pluginOuts = m_effect->numOutputs;

    // Fill input scratch buffers from caller's input pointers
    const int inChannels = std::min(pluginIns, m_numChannels);
    for (int ch = 0; ch < inChannels; ++ch)
        std::copy(in[ch], in[ch] + samples, m_inputPtrs[ch]);

    // Zero any extra plugin input channels that the host doesn't supply
    for (int ch = inChannels; ch < pluginIns && ch < m_numChannels; ++ch)
        std::fill(m_inputPtrs[ch], m_inputPtrs[ch] + samples, 0.0f);

    // Clear output scratch buffers
    for (int ch = 0; ch < m_numChannels; ++ch)
        std::fill(m_outputPtrs[ch], m_outputPtrs[ch] + samples, 0.0f);

    // Run the plugin (guard against legacy VST2 plugins with null processReplacing)
    if (m_effect->processReplacing)
    {
        m_effect->processReplacing(m_effect,
                                   m_inputPtrs.data(),
                                   m_outputPtrs.data(),
                                   samples);
    }
    else
    {
        // Very old VST2 plugin — no processReplacing; passthrough.
        for (int ch = 0; ch < m_numChannels; ++ch)
            std::copy(m_inputPtrs[ch], m_inputPtrs[ch] + samples, m_outputPtrs[ch]);
    }

    // Copy plugin outputs to caller's output buffers
    const int outChannels = std::min(pluginOuts, m_numChannels);
    for (int ch = 0; ch < outChannels; ++ch)
        std::copy(m_outputPtrs[ch], m_outputPtrs[ch] + samples, out[ch]);

    // Mono fold-down: if plugin has 1 output but host expects 2 channels,
    // duplicate the mono output to the second channel
    if (pluginOuts == 1 && m_numChannels >= 2)
        std::copy(out[0], out[0] + samples, out[1]);

    return samples;
}

// ---------------------------------------------------------------------------
// setParameter() — thread-safe; queues change for audio thread delivery
// ---------------------------------------------------------------------------

void VSTPlugin2::setParameter(int index, float value)
{
    m_paramQueue.push(ParamChange2{index, value});
}

// ---------------------------------------------------------------------------
// getParameter() — approximately thread-safe (read-only, display purposes)
// ---------------------------------------------------------------------------

float VSTPlugin2::getParameter(int index) const
{
    if (!m_loaded || !m_effect)
        return 0.0f;
    return m_effect->getParameter(m_effect, index);
}

// ---------------------------------------------------------------------------
// getParameterCount()
// ---------------------------------------------------------------------------

int VSTPlugin2::getParameterCount() const
{
    return m_effect ? m_effect->numParams : 0;
}

// ---------------------------------------------------------------------------
// getParameterName()
//
// The VST2 spec mandates only 8 bytes, but virtually every plugin writes
// more.  We allocate 64 bytes to avoid buffer overruns.
// ---------------------------------------------------------------------------

std::string VSTPlugin2::getParameterName(int index) const
{
    if (!m_loaded || !m_effect)
        return {};
    char buf[64] = {};
    m_effect->dispatcher(m_effect, effGetParamName, index, 0, buf, 0.0f);
    return std::string(buf);
}

// ---------------------------------------------------------------------------
// getLatencySamples()
// ---------------------------------------------------------------------------

int VSTPlugin2::getLatencySamples() const
{
    return m_effect ? m_effect->initialDelay : 0;
}

// ---------------------------------------------------------------------------
// saveState()
//
// Format (matches task3_interface.md contract):
//   First byte 'C' + chunk blob  — if plugin supports effFlagsProgramChunks
//   First byte 'P' + raw floats  — otherwise
// ---------------------------------------------------------------------------

std::vector<uint8_t> VSTPlugin2::saveState() const
{
    if (!m_loaded || !m_effect)
        return {};

    if (m_effect->flags & effFlagsProgramChunks) {
        // Plugin supports chunk-based preset storage
        void* chunkData = nullptr;
        VstIntPtr chunkSize = m_effect->dispatcher(
            m_effect, effGetChunk,
            0,           // 0 = bank chunk
            0, &chunkData, 0.0f);

        if (chunkData && chunkSize > 0) {
            std::vector<uint8_t> result;
            result.reserve(1 + static_cast<size_t>(chunkSize));
            result.push_back('C');
            const uint8_t* src = static_cast<const uint8_t*>(chunkData);
            result.insert(result.end(), src, src + chunkSize);
            return result;
        }
    }

    // Fall back to saving all parameters as raw floats
    const int count = m_effect->numParams;
    std::vector<uint8_t> result;
    result.reserve(1 + static_cast<size_t>(count) * sizeof(float));
    result.push_back('P');

    for (int i = 0; i < count; ++i) {
        float v = m_effect->getParameter(m_effect, i);
        uint8_t bytes[sizeof(float)];
        std::memcpy(bytes, &v, sizeof(float));
        result.insert(result.end(), bytes, bytes + sizeof(float));
    }

    return result;
}

// ---------------------------------------------------------------------------
// loadState()
// ---------------------------------------------------------------------------

bool VSTPlugin2::loadState(const std::vector<uint8_t>& data)
{
    if (!m_loaded || !m_effect || data.empty())
        return false;

    const uint8_t tag = data[0];

    if (tag == 'C') {
        // Chunk mode
        if (data.size() < 2)
            return false;
        const size_t chunkSize = data.size() - 1;
        // We need a non-const copy because dispatcher takes void* (not const void*)
        std::vector<uint8_t> blob(data.begin() + 1, data.end());
        m_effect->dispatcher(m_effect, effSetChunk,
                             0,   // 0 = bank chunk
                             static_cast<VstIntPtr>(chunkSize),
                             blob.data(), 0.0f);
        return true;
    }

    if (tag == 'P') {
        // Parameter mode
        const size_t bodySize = data.size() - 1;
        if (bodySize % sizeof(float) != 0)
            return false;
        const int count = static_cast<int>(bodySize / sizeof(float));
        for (int i = 0; i < count && i < m_effect->numParams; ++i) {
            float v;
            std::memcpy(&v, data.data() + 1 + i * sizeof(float), sizeof(float));
            // VST2 parameter set is a direct function-pointer call.
            m_effect->setParameter(m_effect, i, v);
        }
        return true;
    }

    return false;  // Unknown format tag
}

// ---------------------------------------------------------------------------
// Editor support — VST2 native editor window
// ---------------------------------------------------------------------------

bool VSTPlugin2::hasEditor() const
{
    if (!m_loaded || !m_effect)
        return false;
    return (m_effect->flags & effFlagsHasEditor) != 0;
}

bool VSTPlugin2::openEditor(void* parentWindow)
{
    if (!m_loaded || !m_effect || !hasEditor())
        return false;
    m_effect->dispatcher(m_effect, effEditOpen, 0, 0, parentWindow, 0.0f);
    return true;
}

void VSTPlugin2::closeEditor()
{
    if (m_loaded && m_effect)
        m_effect->dispatcher(m_effect, effEditClose, 0, 0, nullptr, 0.0f);
}

bool VSTPlugin2::getEditorSize(int& width, int& height) const
{
    if (!m_loaded || !m_effect || !hasEditor())
        return false;

    ERect* rect = nullptr;
    m_effect->dispatcher(m_effect, effEditGetRect, 0, 0, &rect, 0.0f);
    if (!rect)
        return false;

    width  = static_cast<int>(rect->right  - rect->left);
    height = static_cast<int>(rect->bottom - rect->top);
    return (width > 0 && height > 0);
}

void VSTPlugin2::idleEditor()
{
    if (m_loaded && m_effect)
        m_effect->dispatcher(m_effect, effEditIdle, 0, 0, nullptr, 0.0f);
}

// ---------------------------------------------------------------------------
// staticAudioMaster — trampoline; recovers VSTPlugin2* from AEffect::user
// ---------------------------------------------------------------------------

VstIntPtr VSTCALLBACK VSTPlugin2::staticAudioMaster(
    AEffect*  effect,
    VstInt32  opcode,
    VstInt32  index,
    VstIntPtr value,
    void*     ptr,
    float     opt)
{
    if (effect && effect->user)
        return static_cast<VSTPlugin2*>(effect->user)->audioMaster(
            effect, opcode, index, value, ptr, opt);

    // Handle opcodes that arrive before the effect is fully constructed
    // (e.g. audioMasterVersion queried inside VSTPluginMain itself)
    if (opcode == audioMasterVersion)
        return 2400;

    return 0;
}

// ---------------------------------------------------------------------------
// audioMaster() — per-instance host callback dispatcher
// ---------------------------------------------------------------------------

VstIntPtr VSTPlugin2::audioMaster(
    AEffect*  /*effect*/,
    VstInt32  opcode,
    VstInt32  /*index*/,
    VstIntPtr /*value*/,
    void*     ptr,
    float     /*opt*/)
{
    switch (opcode)
    {
    case audioMasterVersion:
        return 2400;

    case audioMasterGetSampleRate:
        return static_cast<VstIntPtr>(m_sampleRate);

    case audioMasterGetBlockSize:
        return static_cast<VstIntPtr>(m_blockSize);

    case audioMasterGetCurrentProcessLevel:
        return kVstProcessLevelRealtime;  // 2

    case audioMasterGetVendorString:
        if (ptr)
            strcpy_s(static_cast<char*>(ptr), 64, "Kodi VST Host");
        return 1;

    case audioMasterGetProductString:
        if (ptr)
            strcpy_s(static_cast<char*>(ptr), 64, "Kodi AudioDSP");
        return 1;

    case audioMasterGetVendorVersion:
        return 1000;

    case audioMasterCanDo:
        // We do not support MIDI, time info, or any other optional features
        return 0;

    default:
        return 0;
    }
}
