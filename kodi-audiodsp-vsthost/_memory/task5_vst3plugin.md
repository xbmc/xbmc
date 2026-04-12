---
name: Task 5 — VSTPlugin3 + VSTHostContext
description: VST3 plugin wrapper, host application context, state machine, param mapping, process data setup, state serialization
type: project
---

## Files created

| File | Description |
|------|-------------|
| `src/vst3/VSTHostContext.h` | IHostApplication declaration (header) |
| `src/vst3/VSTHostContext.cpp` | IHostApplication implementation |
| `src/vst3/VSTPlugin3.h` | VSTPlugin3 class declaration (IVSTPlugin subclass) |
| `src/vst3/VSTPlugin3.cpp` | VSTPlugin3 full implementation |

---

## VST3 state machine

```
[constructed]
     │
     ▼
  load()
  ─────────────────────────────────────────────────────
  1. Module::create(path)           → m_module
  2. factory = module->getFactory()
  3. new PlugProvider(factory, null, true)
  4. provider->setupPlugin()
  5. component  = provider->getComponent()
  6. processor  = FUnknownPtr<IAudioProcessor>(component)
  7. controller = QI from component OR provider->getController()
  8. processor->setupProcessing(ProcessSetup)
  9. component->activateBus(audio, in/out, 0, true)
  10. processData.prepare(component, blockSize, kSample32)
  11. component->setActive(true)
  12. processor->setProcessing(true)
  13. buildParamIndex()
  ─────────────────────────────────────────────────────
     │
     ▼
[loaded / ready]
     │
     ├──── process() ──────────────────────────────────
     │     1. drainParamQueue()
     │     2. if bypassed: memcpy in→out, return
     │     3. fill processData (numSamples, buffers)
     │     4. processor->process(processData)
     │     5. clear processData.inputParameterChanges
     │
     ├──── setParameter(index, value) ── push to RingBuffer
     │
     ├──── saveState() / loadState() ── MemoryStream
     │
     ▼
  unload()
  ─────────────────────────────────────────────────────
  1. processor->setProcessing(false)
  2. component->setActive(false)
  3. controller = nullptr
  4. processor  = nullptr
  5. component  = nullptr
  6. provider   = nullptr
  7. module.reset()
  8. m_loaded = false
  ─────────────────────────────────────────────────────
```

---

## IEditController param index mapping

VST3 parameters are identified by opaque 32-bit `ParamID` values that are
plugin-defined and non-contiguous. The host interface (`IVSTPlugin`) uses a
simple 0-based integer index for API consistency with VST2.

`buildParamIndex()` is called once during `load()` and creates the mapping:

```cpp
// m_paramIDByIndex[integerIndex] == vst3ParamID
int count = m_controller->getParameterCount();
m_paramIDByIndex.resize(count);
for (int i = 0; i < count; i++) {
    Steinberg::Vst::ParameterInfo info{};
    m_controller->getParameterInfo(i, info);
    m_paramIDByIndex[i] = info.id;   // uint32_t
}
```

`setParameter(index, value)` converts from integer index → ParamID and
pushes a `ParamChange3{paramID, (double)value, 0}` into the SPSC ring buffer.

`getParameter(index)` calls `m_controller->getParamNormalized(paramID)`
directly (reads from controller, not processor — approximately thread-safe
for display purposes).

---

## HostProcessData setup pattern

`Steinberg::Vst::HostProcessData::prepare()` (from `processdata.h`) allocates
the `AudioBusBuffers` arrays based on the component's declared bus count.
It must be called after buses are activated and before the first `process()`.

```cpp
// In setupBuses():
m_processData.prepare(*m_component, m_blockSize, Steinberg::Vst::kSample32);

// Each process() block:
m_processData.numSamples = samples;
m_processData.inputs[0].numChannels      = m_numChannels;
m_processData.inputs[0].channelBuffers32 = in;   // float** from IVSTPlugin::process
m_processData.outputs[0].numChannels     = m_numChannels;
m_processData.outputs[0].channelBuffers32 = out;
```

`HostProcessData` also holds `inputParameterChanges` which is set transiently
during `drainParamQueue()` and cleared to `nullptr` after each `process()` call.

---

## MemoryStream state serialization format

State bytes are stored as a flat stream:

```
[component state bytes ...][uint32_t ctrlSize LE][controller state bytes ...]
```

- Component state: raw bytes from `IComponent::getState(&stream)`.
- `ctrlSize`: 4-byte little-endian length of the following controller block.
  Written as `0` when no controller is present or when `getState()` fails.
- Controller state: raw bytes from `IEditController::getState(&ctrlStream)`.

On `loadState()`:
1. Wrap the input vector in a `MemoryStream` (read mode via const_cast).
2. Call `IComponent::setState(&stream)` — stream is consumed up to the
   component's own state boundary (SDK-internal seek position).
3. Read `ctrlSize` via `IBStreamer`.
4. If `ctrlSize > 0`: read that many bytes, wrap in a second `MemoryStream`,
   call `IEditController::setState(&ctrlStream)`.

`Steinberg::MemoryStream` header: `public.sdk/source/common/memorystream.h`
`Steinberg::IBStreamer` header: `base/source/fstreamer.h`

---

## VSTHostContext queryInterface pattern

The VST3 SDK uses a custom RTTI system (`FUnknownPrivate::iidEqual`) rather
than C++ RTTI.  `VSTHostContext::queryInterface` must explicitly check for
every interface it implements:

```cpp
if (FUnknownPrivate::iidEqual(iid, Steinberg::Vst::IHostApplication::iid)) {
    *obj = static_cast<Steinberg::Vst::IHostApplication*>(this);
    return kResultOk;
}
if (FUnknownPrivate::iidEqual(iid, Steinberg::FUnknown::iid)) {
    *obj = static_cast<Steinberg::FUnknown*>(this);
    return kResultOk;
}
*obj = nullptr;
return kNoInterface;
```

Reference counting (`addRef` / `release`) returns the stub value `1` because
`VSTHostContext` is a member of `VSTPlugin3` and its lifetime is managed by
the containing object, not by the SDK's smart-pointer mechanism.

---

## Thread safety summary (inherited from task3_interface.md)

| Method | Thread |
|--------|--------|
| `load()` / `unload()` | Settings/stream thread only |
| `process()` | Audio render thread ONLY |
| `setParameter()` | Any thread (queued via RingBuffer) |
| `getParameter()` | Any thread (approximate / display) |
| `saveState()` / `loadState()` | Settings thread |

`drainParamQueue()` is called from `process()` on the audio thread and
consumes the SPSC `RingBuffer<ParamChange3, 256>`.  The GUI/settings thread
produces into the same buffer via `setParameter()`.  No locks are held.
