---
name: Task 7 — DSPChain + DSPProcessor
description: Ping-pong buffer chain design, chain.json format, plugin lifecycle, Kodi ADSP bridge, latency reporting, channel flag mapping
type: project
---

## Files created

| File | Purpose |
|------|---------|
| `src/dsp/DSPChain.h`       | Class declaration — ordered plugin chain with ping-pong buffers |
| `src/dsp/DSPChain.cpp`     | Full implementation — processing, serialization, base64, JSON parser |
| `src/dsp/DSPProcessor.h`   | Class declaration — Kodi ADSP master-stage bridge |
| `src/dsp/DSPProcessor.cpp` | Full implementation — Kodi callbacks, channel mapping, file I/O |

---

## Ping-pong buffer design

Two scratch buffer sets (`m_bufA`, `m_bufB`) are allocated once in `initialize()`
and never touched during `process()`.  Each set is `numChannels` vectors of
`maxBlockSize` floats, with a parallel `vector<float*>` for raw-pointer access.

```
Kodi in[ch] → memcpy → m_bufA

current = &m_ptrA   (input side)
next    = &m_ptrB   (output side)

for each plugin:
    plugin->process(current->data(), next->data(), samples)
    swap(current, next)

memcpy (*current)[ch] → Kodi out[ch]
```

After an even number of plugins `current == m_ptrA`; after an odd number it
equals `m_ptrB`.  The final `memcpy` always reads from the correct side.

**Bypass** is handled entirely inside `IVSTPlugin::process()` — when
`m_bypassed` is set, the plugin copies `in` to `out` and returns immediately.
The chain always calls `process()` and always swaps, so bypass adds no branch
to the chain loop.

**Empty chain** — if `m_plugins` is empty the chain does a direct `memcpy`
from `in` to `out` and skips all buffer machinery.

---

## chain.json format

```json
{
  "version": 1,
  "sampleRate": 44100.0,
  "numChannels": 2,
  "plugins": [
    {
      "path": "C:\\VST\\MyPlugin.dll",
      "format": "vst2",
      "bypassed": false,
      "state": "<standard base64>"
    },
    {
      "path": "C:\\VST3\\MyPlugin.vst3",
      "format": "vst3",
      "bypassed": true,
      "state": ""
    }
  ]
}
```

- `format`  — `"vst2"` or `"vst3"`.
- `state`   — standard base64 encoding of `IVSTPlugin::saveState()` bytes.
  VST2: first byte is `'C'` (chunk) or `'P'` (param dump), remainder is data.
  VST3: raw IBStream bytes (component state + uint32_t ctrlSize + controller state).
  Empty string when the plugin is unloaded or has no state.

The parser is hand-rolled (no external JSON library).  It handles JSON escape
sequences `\"`, `\\`, `\n`, `\r`, `\t` in string values and supports both
`true`/`false` for booleans.  It is robust to extra whitespace and trailing
commas between objects.  It does NOT support nested objects within plugin
entries beyond the defined schema.

---

## Plugin lifecycle on add / remove

### addPlugin()
1. Construct `VSTPlugin2` or `VSTPlugin3` from the path.
2. If `m_initialized`, call `plugin->load(sampleRate, blockSize, numChannels)`.
3. Append `ChainPlugin` to `m_plugins`.
4. Return false (and discard) if `load()` fails while initialized.

### removePlugin()
1. Bounds-check index.
2. Call `plugin->unload()`.
3. Erase from `m_plugins`.

### initialize()
1. Store sampleRate / blockSize / numChannels.
2. Call `allocatePingPong()`.
3. For each existing plugin: call `plugin->load()`.  Failures are logged but
   processing continues for the remaining plugins.
4. Set `m_initialized = true`.

### shutdown()
1. Call `plugin->unload()` for every slot.
2. Set `m_initialized = false`.
3. Scratch buffers are NOT freed — they are reused on the next `initialize()`.

### clear()
1. Call `unload()` on every slot.
2. `m_plugins.clear()`.

---

## DSPProcessor — bridge to Kodi ADSP API

`DSPProcessor` owns one `DSPChain` and maps the flat Kodi ADSP callback
signatures to the chain's typed interface.

```
Kodi callback                    DSPProcessor method
──────────────────────────────── ───────────────────────────────────────────
StreamCreate(settings, ...)  →   streamCreate(settings)
StreamDestroy(handle)        →   streamDestroy()
MasterProcess(in, out, n)    →   masterProcess(in, out, n)
MasterProcessGetOutChannels  →   masterProcessGetOutChannels()
MasterProcessGetDelay        →   masterProcessGetDelay()  →  getTotalLatencySamples()
MasterProcessGetStreamInfo   →   masterProcessGetName()
```

Fields read from `AE_DSP_SETTINGS`:

| Field                  | Stored as         |
|------------------------|-------------------|
| `iStreamID`            | `m_streamID`      |
| `iProcessSamplerate`   | `m_sampleRate`    |
| `iInChannels`          | `m_numChannels`   |
| `iProcessFrames`       | `blockSize` (→ chain) |

---

## Latency reporting

`DSPChain::getTotalLatencySamples()` sums `IVSTPlugin::getLatencySamples()`
for every slot where `slot.bypassed == false`.

`DSPProcessor::masterProcessGetDelay()` returns this value directly.  Kodi
uses it for A/V sync compensation (lip-sync delay).

---

## Channel flag mapping for masterProcessGetOutChannels()

`AE_DSP_PRSNT_CH_*` flags from `kodi_adsp_types.h`:

| Channels | Layout  | Flags returned                              |
|----------|---------|---------------------------------------------|
| 1        | Mono    | FC                                          |
| 2        | Stereo  | FL FR                                       |
| 3        | 2.1     | FL FR LFE                                   |
| 4        | 4.0     | FL FR BL BR                                 |
| 5        | 5.0     | FL FR FC BL BR                              |
| 6        | 5.1     | FL FR FC LFE BL BR                          |
| 7        | 6.1     | FL FR FC LFE BL BR BC                       |
| 8+       | 7.1     | FL FR FC LFE BL BR SL SR                    |

The mapping is implemented as a `switch` on `m_numChannels` in
`DSPProcessor::masterProcessGetOutChannels()`.  For channel counts above 8,
the 7.1 mask is returned as a best effort.

---

## Base64 encoding

Implemented inline in `DSPChain.cpp` using the standard alphabet
`A-Za-z0-9+/=`.  No external library dependency.  Decoder filters CR/LF/space
before processing to tolerate line-wrapped base64 text (e.g., from manual edits
to chain.json).

---

## Thread safety

| Method | Thread |
|--------|--------|
| `DSPChain::addPlugin / removePlugin / movePlugin / clear` | Settings / stream thread (audio stopped) |
| `DSPChain::initialize / shutdown` | Settings / stream thread |
| `DSPChain::process` | Audio render thread ONLY — no alloc, no lock |
| `DSPChain::setPluginParameter` | Any thread (delegates to IVSTPlugin) |
| `DSPChain::getPluginParameter*` | Any thread (approximately safe for display) |
| `DSPChain::serializeToJson / deserializeFromJson` | Settings thread |
| `DSPProcessor::streamCreate / streamDestroy` | Stream / setup thread |
| `DSPProcessor::masterProcess` | Audio render thread ONLY |
| `DSPProcessor::loadConfig / saveConfig` | Settings thread |
