#pragma once
/*
 * ParamQueue.h — Lock-free SPSC ring buffer for parameter automation
 * Used to safely pass parameter changes from GUI thread to audio thread.
 * Part of audiodsp.vsthost — Kodi Audio DSP addon
 * License: GPL-2.0-or-later
 */
#include <atomic>
#include <array>
#include <cstddef>

/// Single-Producer Single-Consumer lock-free ring buffer.
/// Producer (GUI thread) calls push(); Consumer (audio thread) calls pop().
/// Capacity must be a power of 2.
template<typename T, size_t Capacity>
class RingBuffer
{
    static_assert((Capacity & (Capacity - 1)) == 0, "Capacity must be power of 2");
public:
    RingBuffer() : m_read(0), m_write(0) {}

    /// Push an item (producer side). Returns false if full.
    bool push(const T& item)
    {
        const size_t write = m_write.load(std::memory_order_relaxed);
        const size_t next  = (write + 1) & (Capacity - 1);
        if (next == m_read.load(std::memory_order_acquire))
            return false;  // full
        m_buf[write] = item;
        m_write.store(next, std::memory_order_release);
        return true;
    }

    /// Pop an item (consumer side). Returns false if empty.
    bool pop(T& item)
    {
        const size_t read = m_read.load(std::memory_order_relaxed);
        if (read == m_write.load(std::memory_order_acquire))
            return false;  // empty
        item = m_buf[read];
        m_read.store((read + 1) & (Capacity - 1), std::memory_order_release);
        return true;
    }

    /// True if the buffer is empty (approximate, safe to call from either thread).
    bool empty() const
    {
        return m_read.load(std::memory_order_acquire) ==
               m_write.load(std::memory_order_acquire);
    }

private:
    std::array<T, Capacity> m_buf;
    alignas(64) std::atomic<size_t> m_read;
    alignas(64) std::atomic<size_t> m_write;
};

// Convenience type aliases used by VSTPlugin2 and VSTPlugin3
struct ParamChange2 {
    int   index;
    float value;
};

struct ParamChange3 {
    uint32_t paramID;
    double   value;
    int32_t  sampleOffset;  // for sample-accurate automation (set to 0 for now)
};
