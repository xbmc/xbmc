---
name: Task 3 — IVSTPlugin + ParamQueue
description: Polymorphic plugin interface location, ParamQueue template usage, thread safety contract
type: project
---

## Files created
- src/plugin/IVSTPlugin.h — pure abstract base for VSTPlugin2 and VSTPlugin3
- src/util/ParamQueue.h — RingBuffer<T,N> template + ParamChange2/ParamChange3 types

## Thread safety contract
- load()/unload()/reinitialize() — settings/stream thread only
- process() — audio render thread ONLY; no allocations, no locks, no I/O
- setParameter() — any thread; implementations MUST queue via RingBuffer
- getParameter() — any thread (approximately safe for display)
- saveState()/loadState() — settings thread; never overlap with process()

## RingBuffer usage
```cpp
RingBuffer<ParamChange2, 256> m_queue;
// GUI thread:
m_queue.push({paramIndex, normalizedValue});
// Audio thread (before processReplacing):
ParamChange2 pc;
while (m_queue.pop(pc)) { effect->setParameter(effect, pc.index, pc.value); }
```

## IVSTPlugin::PluginFormat enum
- PluginFormat::VST2 — returned by VSTPlugin2::getFormat()
- PluginFormat::VST3 — returned by VSTPlugin3::getFormat()
Used in PluginSettings to dispatch factory (make_unique<VSTPlugin2> vs make_unique<VSTPlugin3>)

## saveState() byte format
- VST2 chunk: first byte 'C', remaining bytes = chunk blob
- VST2 params: first byte 'P', remaining bytes = numParams floats
- VST3: raw IBStream bytes (no prefix)
