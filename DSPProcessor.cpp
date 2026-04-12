/*
 * DSPProcessor.cpp — Bridge between the Kodi ADSP callback API and DSPChain
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include "DSPProcessor.h"
#include "kodi_adsp_types.h"

#include <algorithm>
#include <cstring>
#include <fstream>
#include <sstream>
#include <cstdio>

// =============================================================================
//  Static data
// =============================================================================

const char* DSPProcessor::s_processorName = "Kodi VST Host";

// =============================================================================
//  Constructor / Destructor
// =============================================================================

DSPProcessor::DSPProcessor()  = default;
DSPProcessor::~DSPProcessor() = default;

// =============================================================================
//  streamCreate
// =============================================================================

bool DSPProcessor::streamCreate(const AE_DSP_SETTINGS* settings)
{
    if (!settings)
        return false;

    m_streamID    = static_cast<int>(settings->iStreamID);
    m_sampleRate  = static_cast<int>(settings->iProcessSamplerate);
    m_numChannels = settings->iInChannels;

    int blockSize = settings->iProcessFrames;
    if (blockSize <= 0)
        blockSize = 1024;

    if (!m_chain.initialize(static_cast<double>(m_sampleRate), blockSize, m_numChannels))
    {
        std::fprintf(stderr,
            "[DSPProcessor] streamCreate: chain initialization failed "
            "(streamID=%d, sampleRate=%d, channels=%d)\n",
            m_streamID, m_sampleRate, m_numChannels);
        return false;
    }

    // Best-effort — an empty chain (no config) is perfectly acceptable.
    loadConfig();

    m_active = true;
    return true;
}

// =============================================================================
//  streamDestroy
// =============================================================================

void DSPProcessor::streamDestroy()
{
    m_active = false;
    m_chain.shutdown();
}

// =============================================================================
//  masterProcess
// =============================================================================

int DSPProcessor::masterProcess(float** in, float** out, unsigned int samples)
{
    if (!m_active)
    {
        // Passthrough when not active.
        for (int ch = 0; ch < m_numChannels; ++ch)
            std::memcpy(out[ch], in[ch], static_cast<size_t>(samples) * sizeof(float));
        return static_cast<int>(samples);
    }

    return m_chain.process(in, out, static_cast<int>(samples));
}

// =============================================================================
//  masterProcessGetOutChannels
//
//  Maps m_numChannels to a bitmask of AE_DSP_PRSNT_CH_* flags.
//
//  Channel ordering follows the standard AE_DSP layout:
//    1  — Mono    : FC
//    2  — Stereo  : FL FR
//    3  — 2.1     : FL FR LFE
//    4  — 4.0     : FL FR BL BR
//    5  — 5.0     : FL FR FC BL BR
//    6  — 5.1     : FL FR FC LFE BL BR
//    7  — 6.1     : FL FR FC LFE BL BR BC
//    8  — 7.1     : FL FR FC LFE BL BR SL SR
//   >8  — return all flags up to the 8-channel set (best effort)
// =============================================================================

unsigned long DSPProcessor::masterProcessGetOutChannels() const
{
    switch (m_numChannels)
    {
        case 1:
            return static_cast<unsigned long>(AE_DSP_PRSNT_CH_FC);

        case 2:
            return static_cast<unsigned long>(AE_DSP_PRSNT_CH_FL)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_FR);

        case 3:
            return static_cast<unsigned long>(AE_DSP_PRSNT_CH_FL)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_FR)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_LFE);

        case 4:
            return static_cast<unsigned long>(AE_DSP_PRSNT_CH_FL)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_FR)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_BL)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_BR);

        case 5:
            return static_cast<unsigned long>(AE_DSP_PRSNT_CH_FL)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_FR)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_FC)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_BL)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_BR);

        case 6:
            return static_cast<unsigned long>(AE_DSP_PRSNT_CH_FL)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_FR)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_FC)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_LFE)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_BL)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_BR);

        case 7:
            return static_cast<unsigned long>(AE_DSP_PRSNT_CH_FL)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_FR)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_FC)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_LFE)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_BL)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_BR)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_BC);

        case 8:
        default:
            return static_cast<unsigned long>(AE_DSP_PRSNT_CH_FL)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_FR)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_FC)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_LFE)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_BL)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_BR)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_SL)
                 | static_cast<unsigned long>(AE_DSP_PRSNT_CH_SR);
    }
}

// =============================================================================
//  masterProcessGetDelay
// =============================================================================

int DSPProcessor::masterProcessGetDelay() const
{
    return m_chain.getTotalLatencySamples();
}

// =============================================================================
//  masterProcessGetName
// =============================================================================

const char* DSPProcessor::masterProcessGetName() const
{
    return s_processorName;
}

// =============================================================================
//  loadConfig
// =============================================================================

bool DSPProcessor::loadConfig()
{
    if (m_configPath.empty())
        return false;

    const std::string filePath = m_configPath + "/chain.json";

    std::ifstream file(filePath);
    if (!file.is_open())
        return false;

    std::ostringstream ss;
    ss << file.rdbuf();
    const std::string json = ss.str();

    if (json.empty())
        return false;

    return m_chain.deserializeFromJson(json);
}

// =============================================================================
//  saveConfig
// =============================================================================

bool DSPProcessor::saveConfig()
{
    if (m_configPath.empty())
        return false;

    const std::string filePath = m_configPath + "/chain.json";
    const std::string json     = m_chain.serializeToJson();

    std::ofstream file(filePath, std::ios::out | std::ios::trunc);
    if (!file.is_open())
    {
        std::fprintf(stderr,
            "[DSPProcessor] saveConfig: cannot open '%s' for writing\n",
            filePath.c_str());
        return false;
    }

    file << json;
    return file.good();
}
