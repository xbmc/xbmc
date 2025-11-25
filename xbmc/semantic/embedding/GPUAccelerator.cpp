/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "GPUAccelerator.h"

#include "utils/log.h"

#include <algorithm>
#include <sstream>

#ifdef HAS_ONNXRUNTIME
#include <onnxruntime_cxx_api.h>
#endif

// Platform-specific GPU detection headers
#if defined(TARGET_WINDOWS)
#include <windows.h>
#elif defined(TARGET_LINUX)
#include <dlfcn.h>
#elif defined(TARGET_DARWIN)
#include <sys/sysctl.h>
#endif

namespace KODI
{
namespace SEMANTIC
{

// PIMPL implementation class
class CGPUAccelerator::Impl
{
public:
  Impl() = default;
  ~Impl() { Shutdown(); }

  GPUConfig m_config;
  GPUStatus m_status{GPUStatus::DISABLED};
  GPUBackend m_activeBackend{GPUBackend::NONE};
  GPUDeviceInfo m_activeDevice;
  std::string m_statusMessage;
  bool m_initialized{false};

  bool Initialize(const GPUConfig& config)
  {
    m_config = config;

    if (!config.enabled)
    {
      m_status = GPUStatus::DISABLED;
      m_statusMessage = "GPU acceleration disabled by configuration";
      CLog::Log(LOGINFO, "GPUAccelerator: GPU acceleration disabled");
      return true; // Not an error
    }

#ifndef HAS_ONNXRUNTIME
    m_status = GPUStatus::NOT_AVAILABLE;
    m_statusMessage = "ONNX Runtime not available (HAS_ONNXRUNTIME not defined)";
    CLog::Log(LOGERROR, "GPUAccelerator: {}", m_statusMessage);
    return false;
#else
    m_status = GPUStatus::INITIALIZING;
    CLog::Log(LOGINFO, "GPUAccelerator: Initializing GPU acceleration");

    try
    {
      // Detect best backend if not specified
      GPUBackend backendToUse = config.preferredBackend;
      if (backendToUse == GPUBackend::NONE)
      {
        backendToUse = DetectBestBackendInternal();
        CLog::Log(LOGINFO, "GPUAccelerator: Auto-detected backend: {}",
                  GPUBackendToString(backendToUse));
      }

      // Check if backend is available
      if (!IsBackendAvailableInternal(backendToUse))
      {
        CLog::Log(LOGWARNING,
                  "GPUAccelerator: Requested backend {} not available, trying fallback",
                  GPUBackendToString(backendToUse));

        if (config.fallbackToCPU)
        {
          m_status = GPUStatus::FALLBACK_TO_CPU;
          m_activeBackend = GPUBackend::NONE;
          m_statusMessage = "GPU not available, using CPU fallback";
          CLog::Log(LOGINFO, "GPUAccelerator: {}", m_statusMessage);
          m_initialized = true;
          return true;
        }
        else
        {
          m_status = GPUStatus::NOT_AVAILABLE;
          m_statusMessage = "GPU backend not available and fallback disabled";
          CLog::Log(LOGERROR, "GPUAccelerator: {}", m_statusMessage);
          return false;
        }
      }

      // Enumerate devices for the selected backend
      auto devices = EnumerateDevicesForBackend(backendToUse);
      if (devices.empty())
      {
        CLog::Log(LOGWARNING, "GPUAccelerator: No devices found for backend {}",
                  GPUBackendToString(backendToUse));

        if (config.fallbackToCPU)
        {
          m_status = GPUStatus::FALLBACK_TO_CPU;
          m_activeBackend = GPUBackend::NONE;
          m_statusMessage = "No GPU devices found, using CPU fallback";
          CLog::Log(LOGINFO, "GPUAccelerator: {}", m_statusMessage);
          m_initialized = true;
          return true;
        }
        else
        {
          m_status = GPUStatus::NOT_AVAILABLE;
          m_statusMessage = "No GPU devices found";
          return false;
        }
      }

      // Select device (use config.deviceId or first available)
      m_activeDevice = (config.deviceId >= 0 && static_cast<size_t>(config.deviceId) < devices.size())
                           ? devices[config.deviceId]
                           : devices[0];

      m_activeBackend = backendToUse;
      m_status = GPUStatus::READY;
      m_initialized = true;

      std::ostringstream oss;
      oss << "GPU Ready: " << m_activeDevice.name << " (" << GPUBackendToString(m_activeBackend)
          << ") with " << m_activeDevice.totalMemoryMB << " MB VRAM";
      m_statusMessage = oss.str();

      CLog::Log(LOGINFO, "GPUAccelerator: {}", m_statusMessage);
      CLog::Log(LOGINFO, "GPUAccelerator: Optimal batch size: {}", GetOptimalBatchSize());

      return true;
    }
    catch (const std::exception& e)
    {
      m_status = GPUStatus::ERROR;
      m_statusMessage = std::string("GPU initialization error: ") + e.what();
      CLog::Log(LOGERROR, "GPUAccelerator: {}", m_statusMessage);

      if (config.fallbackToCPU)
      {
        m_status = GPUStatus::FALLBACK_TO_CPU;
        m_activeBackend = GPUBackend::NONE;
        m_statusMessage = "GPU error, using CPU fallback";
        CLog::Log(LOGINFO, "GPUAccelerator: {}", m_statusMessage);
        m_initialized = true;
        return true;
      }

      return false;
    }
#endif
  }

  void Shutdown()
  {
    if (!m_initialized)
      return;

    CLog::Log(LOGINFO, "GPUAccelerator: Shutting down");
    m_status = GPUStatus::DISABLED;
    m_activeBackend = GPUBackend::NONE;
    m_initialized = false;
  }

  bool IsAvailable() const { return m_initialized && m_status == GPUStatus::READY; }

  bool ConfigureSessionOptions(Ort::SessionOptions& sessionOptions) const
  {
#ifdef HAS_ONNXRUNTIME
    if (!m_initialized)
    {
      CLog::Log(LOGWARNING, "GPUAccelerator: Not initialized, cannot configure session");
      return false;
    }

    // If using CPU fallback, don't add GPU provider
    if (m_activeBackend == GPUBackend::NONE || m_status == GPUStatus::FALLBACK_TO_CPU)
    {
      CLog::Log(LOGDEBUG, "GPUAccelerator: Using CPU execution (no GPU provider)");
      return true; // Not an error, just using CPU
    }

    try
    {
      CLog::Log(LOGINFO, "GPUAccelerator: Configuring {} execution provider",
                GPUBackendToString(m_activeBackend));

      switch (m_activeBackend)
      {
        case GPUBackend::CUDA:
        {
          OrtCUDAProviderOptions cudaOptions;
          cudaOptions.device_id = m_activeDevice.deviceId;
          cudaOptions.cudnn_conv_algo_search = OrtCudnnConvAlgoSearchExhaustive;
          cudaOptions.gpu_mem_limit = m_config.memoryLimitMB > 0
                                          ? m_config.memoryLimitMB * 1024ULL * 1024ULL
                                          : 0;
          cudaOptions.arena_extend_strategy = 1; // kSameAsRequested
          cudaOptions.do_copy_in_default_stream = !m_config.enableAsyncTransfer;

          sessionOptions.AppendExecutionProvider_CUDA(cudaOptions);
          CLog::Log(LOGINFO, "GPUAccelerator: CUDA provider configured (device {}, mem limit {}MB)",
                    cudaOptions.device_id, m_config.memoryLimitMB);
          break;
        }

        case GPUBackend::DIRECTML:
        {
#if defined(TARGET_WINDOWS)
          OrtDmlProviderOptions dmlOptions;
          dmlOptions.device_id = m_activeDevice.deviceId;
          sessionOptions.AppendExecutionProvider_DML(dmlOptions);
          CLog::Log(LOGINFO, "GPUAccelerator: DirectML provider configured (device {})",
                    dmlOptions.device_id);
#else
          CLog::Log(LOGERROR, "GPUAccelerator: DirectML only available on Windows");
          return false;
#endif
          break;
        }

        case GPUBackend::OPENCL:
        {
          // Note: OpenCL provider may have limited availability in ONNX Runtime
          // Fallback to CPU if not available
          CLog::Log(LOGWARNING,
                    "GPUAccelerator: OpenCL provider support varies by ONNX Runtime build");
          // Could try to append if available in the ONNX Runtime build
          break;
        }

        case GPUBackend::COREML:
        {
#if defined(TARGET_DARWIN)
          uint32_t coremlFlags = 0;
          // COREML_FLAG_USE_CPU_ONLY = 0x001
          // COREML_FLAG_ENABLE_ON_SUBGRAPH = 0x002
          // COREML_FLAG_ONLY_ENABLE_DEVICE_WITH_ANE = 0x004
          coremlFlags |= 0x002; // Enable on subgraph
          sessionOptions.AppendExecutionProvider_CoreML(coremlFlags);
          CLog::Log(LOGINFO, "GPUAccelerator: CoreML provider configured");
#else
          CLog::Log(LOGERROR, "GPUAccelerator: CoreML only available on macOS/iOS");
          return false;
#endif
          break;
        }

        case GPUBackend::ROCM:
        {
          OrtROCMProviderOptions rocmOptions;
          rocmOptions.device_id = m_activeDevice.deviceId;
          rocmOptions.gpu_mem_limit = m_config.memoryLimitMB > 0
                                          ? m_config.memoryLimitMB * 1024ULL * 1024ULL
                                          : 0;
          sessionOptions.AppendExecutionProvider_ROCM(rocmOptions);
          CLog::Log(LOGINFO, "GPUAccelerator: ROCm provider configured (device {}, mem limit {}MB)",
                    rocmOptions.device_id, m_config.memoryLimitMB);
          break;
        }

        case GPUBackend::TENSORRT:
        {
          OrtTensorRTProviderOptions trtOptions;
          trtOptions.device_id = m_activeDevice.deviceId;
          trtOptions.trt_max_workspace_size = m_config.memoryLimitMB > 0
                                                  ? m_config.memoryLimitMB * 1024ULL * 1024ULL
                                                  : 1024ULL * 1024ULL * 1024ULL; // 1GB default
          sessionOptions.AppendExecutionProvider_TensorRT(trtOptions);
          CLog::Log(LOGINFO,
                    "GPUAccelerator: TensorRT provider configured (device {}, workspace {}MB)",
                    trtOptions.device_id, m_config.memoryLimitMB > 0 ? m_config.memoryLimitMB : 1024);
          break;
        }

        default:
          CLog::Log(LOGWARNING, "GPUAccelerator: Unknown backend type");
          return false;
      }

      return true;
    }
    catch (const Ort::Exception& e)
    {
      CLog::Log(LOGERROR, "GPUAccelerator: Failed to configure execution provider: {}", e.what());
      return false;
    }
    catch (const std::exception& e)
    {
      CLog::Log(LOGERROR, "GPUAccelerator: Exception configuring session: {}", e.what());
      return false;
    }
#else
    return false;
#endif
  }

  int GetOptimalBatchSize() const
  {
    if (!IsAvailable() || m_activeBackend == GPUBackend::NONE)
    {
      // CPU mode: smaller batches to avoid memory pressure
      return 4;
    }

    // GPU mode: larger batches for better utilization
    // Base batch size on available memory
    size_t availableMB = m_activeDevice.availableMemoryMB;

    if (availableMB >= 8192) // 8GB+
      return 64;
    else if (availableMB >= 4096) // 4GB+
      return 32;
    else if (availableMB >= 2048) // 2GB+
      return 16;
    else
      return 8; // Minimal GPU memory

    // Respect user config if set
    if (m_config.batchSize > 0)
      return std::min(m_config.batchSize, 64);

    return 32; // Default for GPU
  }

  size_t EstimateMemoryUsage(int batchSize, int maxTokens) const
  {
    // Rough estimation for all-MiniLM-L6-v2 model:
    // - Input tensors: 3 tensors * batchSize * maxTokens * 8 bytes (int64_t)
    // - Output tensor: batchSize * maxTokens * 384 * 4 bytes (float)
    // - Model weights: ~80MB (loaded once)
    // - Working memory: ~2x tensor size (rough estimate)

    size_t inputSize = 3ULL * batchSize * maxTokens * sizeof(int64_t);
    size_t outputSize = batchSize * maxTokens * 384 * sizeof(float);
    size_t tensorSize = inputSize + outputSize;
    size_t workingMemory = tensorSize * 2; // Conservative estimate
    size_t modelSize = 80 * 1024 * 1024; // ~80MB

    size_t totalBytes = modelSize + workingMemory;
    return totalBytes / (1024 * 1024); // Convert to MB
  }

  bool CanFitBatch(int batchSize, int maxTokens) const
  {
    if (!IsAvailable())
      return true; // CPU mode, no GPU memory constraints

    size_t estimatedMB = EstimateMemoryUsage(batchSize, maxTokens);
    size_t availableMB = m_activeDevice.availableMemoryMB;

    // Leave 20% headroom
    size_t usableMB = static_cast<size_t>(availableMB * 0.8);

    bool fits = estimatedMB <= usableMB;

    if (!fits)
    {
      CLog::Log(LOGDEBUG,
                "GPUAccelerator: Batch too large - estimated {}MB, available {}MB ({}MB usable)",
                estimatedMB, availableMB, usableMB);
    }

    return fits;
  }

  GPUMetrics GetMetrics() const
  {
    GPUMetrics metrics;

    if (!IsAvailable())
      return metrics;

    // Platform-specific metric collection
#if defined(TARGET_WINDOWS) && defined(HAS_ONNXRUNTIME)
    // Windows: Could use NVML for NVIDIA, or Windows Performance Counters
    // For now, basic metrics only
    metrics.memoryTotalMB = static_cast<float>(m_activeDevice.totalMemoryMB);
    metrics.metricsAvailable = false; // Full metrics not implemented yet
#elif defined(TARGET_LINUX)
    // Linux: Could read from /sys/class/drm or use NVML for NVIDIA
    metrics.memoryTotalMB = static_cast<float>(m_activeDevice.totalMemoryMB);
    metrics.metricsAvailable = false; // Full metrics not implemented yet
#elif defined(TARGET_DARWIN)
    // macOS: Limited GPU metrics available
    metrics.memoryTotalMB = static_cast<float>(m_activeDevice.totalMemoryMB);
    metrics.metricsAvailable = false; // Full metrics not implemented yet
#endif

    return metrics;
  }

  std::vector<GPUDeviceInfo> EnumerateDevicesForBackend(GPUBackend backend) const
  {
    std::vector<GPUDeviceInfo> devices;

    switch (backend)
    {
      case GPUBackend::CUDA:
        devices = EnumerateCUDADevices();
        break;
      case GPUBackend::DIRECTML:
        devices = EnumerateDirectMLDevices();
        break;
      case GPUBackend::OPENCL:
        devices = EnumerateOpenCLDevices();
        break;
      case GPUBackend::COREML:
        devices = EnumerateCoreMLDevices();
        break;
      case GPUBackend::ROCM:
        devices = EnumerateROCmDevices();
        break;
      case GPUBackend::TENSORRT:
        devices = EnumerateCUDADevices(); // TensorRT uses CUDA devices
        break;
      default:
        break;
    }

    return devices;
  }

  std::vector<GPUDeviceInfo> EnumerateCUDADevices() const
  {
    std::vector<GPUDeviceInfo> devices;

#if defined(HAS_ONNXRUNTIME) && (defined(TARGET_WINDOWS) || defined(TARGET_LINUX))
    // Try to detect CUDA devices via NVIDIA driver or CUDA runtime
    // For a production implementation, would use CUDA API or NVML
    // Simplified stub for now - assumes CUDA available if requested

    GPUDeviceInfo device;
    device.deviceId = 0;
    device.backend = GPUBackend::CUDA;
    device.name = "NVIDIA GPU (CUDA)";
    device.vendor = "NVIDIA";
    device.totalMemoryMB = 4096; // Placeholder - would query actual VRAM
    device.availableMemoryMB = 3072;
    device.supportsCompute = true;
    device.computeCapability = 70; // Compute 7.0 (Volta+)
    device.isDefault = true;

    devices.push_back(device);
#endif

    return devices;
  }

  std::vector<GPUDeviceInfo> EnumerateDirectMLDevices() const
  {
    std::vector<GPUDeviceInfo> devices;

#if defined(TARGET_WINDOWS) && defined(HAS_ONNXRUNTIME)
    // DirectML works with any D3D12-capable GPU
    GPUDeviceInfo device;
    device.deviceId = 0;
    device.backend = GPUBackend::DIRECTML;
    device.name = "DirectML GPU";
    device.vendor = "DirectML";
    device.totalMemoryMB = 4096; // Placeholder
    device.availableMemoryMB = 3072;
    device.supportsCompute = true;
    device.isDefault = true;

    devices.push_back(device);
#endif

    return devices;
  }

  std::vector<GPUDeviceInfo> EnumerateOpenCLDevices() const
  {
    std::vector<GPUDeviceInfo> devices;

    // OpenCL device enumeration would require OpenCL SDK
    // Stub implementation for now

    return devices;
  }

  std::vector<GPUDeviceInfo> EnumerateCoreMLDevices() const
  {
    std::vector<GPUDeviceInfo> devices;

#if defined(TARGET_DARWIN) && defined(HAS_ONNXRUNTIME)
    // CoreML available on all Macs with Metal support
    GPUDeviceInfo device;
    device.deviceId = 0;
    device.backend = GPUBackend::COREML;
    device.vendor = "Apple";
    device.supportsCompute = true;
    device.isDefault = true;

#if defined(TARGET_DARWIN_IOS)
    device.name = "Apple Neural Engine";
    device.totalMemoryMB = 2048; // Shared memory on iOS
    device.availableMemoryMB = 1536;
#else
    // Check if Apple Silicon or Intel Mac
    // Simple detection - would use actual sysctl in production
    device.name = "Apple GPU";
    device.totalMemoryMB = 8192; // Placeholder
    device.availableMemoryMB = 6144;
#endif

    devices.push_back(device);
#endif

    return devices;
  }

  std::vector<GPUDeviceInfo> EnumerateROCmDevices() const
  {
    std::vector<GPUDeviceInfo> devices;

#if defined(TARGET_LINUX) && defined(HAS_ONNXRUNTIME)
    // ROCm device enumeration - would use ROCm SMI in production
    GPUDeviceInfo device;
    device.deviceId = 0;
    device.backend = GPUBackend::ROCM;
    device.name = "AMD GPU (ROCm)";
    device.vendor = "AMD";
    device.totalMemoryMB = 8192; // Placeholder
    device.availableMemoryMB = 6144;
    device.supportsCompute = true;
    device.isDefault = true;

    devices.push_back(device);
#endif

    return devices;
  }

  static GPUBackend DetectBestBackendInternal()
  {
#if defined(TARGET_WINDOWS)
    // Windows: Try CUDA first, then DirectML
    if (IsBackendAvailableInternal(GPUBackend::CUDA))
      return GPUBackend::CUDA;
    if (IsBackendAvailableInternal(GPUBackend::DIRECTML))
      return GPUBackend::DIRECTML;
#elif defined(TARGET_LINUX)
    // Linux: Try CUDA, then ROCm, then OpenCL
    if (IsBackendAvailableInternal(GPUBackend::CUDA))
      return GPUBackend::CUDA;
    if (IsBackendAvailableInternal(GPUBackend::ROCM))
      return GPUBackend::ROCM;
    if (IsBackendAvailableInternal(GPUBackend::OPENCL))
      return GPUBackend::OPENCL;
#elif defined(TARGET_DARWIN)
    // macOS/iOS: CoreML
    if (IsBackendAvailableInternal(GPUBackend::COREML))
      return GPUBackend::COREML;
#endif

    return GPUBackend::NONE;
  }

  static bool IsBackendAvailableInternal(GPUBackend backend)
  {
#ifndef HAS_ONNXRUNTIME
    return false;
#endif

    switch (backend)
    {
      case GPUBackend::CUDA:
#if defined(TARGET_WINDOWS) || defined(TARGET_LINUX)
        // Check for CUDA libraries - simplified check
        // In production, would check for libcuda.so / nvcuda.dll
        return true; // Assume available if compiled with support
#else
        return false;
#endif

      case GPUBackend::DIRECTML:
#if defined(TARGET_WINDOWS)
        // DirectML available on Windows 10 1903+ with D3D12
        return true;
#else
        return false;
#endif

      case GPUBackend::OPENCL:
        // OpenCL could be available on multiple platforms
        // Would need to check for OpenCL runtime
        return false; // Conservative - requires explicit setup

      case GPUBackend::COREML:
#if defined(TARGET_DARWIN)
        return true;
#else
        return false;
#endif

      case GPUBackend::ROCM:
#if defined(TARGET_LINUX)
        // Check for ROCm installation
        return false; // Conservative - requires explicit setup
#else
        return false;
#endif

      case GPUBackend::TENSORRT:
#if defined(TARGET_WINDOWS) || defined(TARGET_LINUX)
        // TensorRT requires CUDA + TensorRT libraries
        return false; // Conservative - requires explicit setup
#else
        return false;
#endif

      default:
        return false;
    }
  }
};

// CGPUAccelerator public interface implementation

CGPUAccelerator::CGPUAccelerator() : m_impl(std::make_unique<Impl>())
{
}

CGPUAccelerator::~CGPUAccelerator() = default;

bool CGPUAccelerator::Initialize(const GPUConfig& config)
{
  return m_impl->Initialize(config);
}

void CGPUAccelerator::Shutdown()
{
  m_impl->Shutdown();
}

bool CGPUAccelerator::IsAvailable() const
{
  return m_impl->IsAvailable();
}

GPUStatus CGPUAccelerator::GetStatus() const
{
  return m_impl->m_status;
}

GPUBackend CGPUAccelerator::GetBackend() const
{
  return m_impl->m_activeBackend;
}

std::vector<GPUDeviceInfo> CGPUAccelerator::EnumerateDevices() const
{
  std::vector<GPUDeviceInfo> allDevices;

  // Enumerate across all backends
  for (int i = static_cast<int>(GPUBackend::CUDA); i <= static_cast<int>(GPUBackend::TENSORRT); ++i)
  {
    GPUBackend backend = static_cast<GPUBackend>(i);
    if (IsBackendAvailable(backend))
    {
      auto devices = m_impl->EnumerateDevicesForBackend(backend);
      allDevices.insert(allDevices.end(), devices.begin(), devices.end());
    }
  }

  return allDevices;
}

GPUDeviceInfo CGPUAccelerator::GetActiveDevice() const
{
  return m_impl->m_activeDevice;
}

GPUMetrics CGPUAccelerator::GetMetrics() const
{
  return m_impl->GetMetrics();
}

bool CGPUAccelerator::ConfigureSessionOptions(Ort::SessionOptions& sessionOptions) const
{
  return m_impl->ConfigureSessionOptions(sessionOptions);
}

int CGPUAccelerator::GetOptimalBatchSize() const
{
  return m_impl->GetOptimalBatchSize();
}

size_t CGPUAccelerator::EstimateMemoryUsage(int batchSize, int maxTokens) const
{
  return m_impl->EstimateMemoryUsage(batchSize, maxTokens);
}

bool CGPUAccelerator::CanFitBatch(int batchSize, int maxTokens) const
{
  return m_impl->CanFitBatch(batchSize, maxTokens);
}

GPUConfig CGPUAccelerator::GetConfig() const
{
  return m_impl->m_config;
}

bool CGPUAccelerator::UpdateConfig(const GPUConfig& config)
{
  // Some settings can be updated without reinit
  bool needsReinit = (config.enabled != m_impl->m_config.enabled) ||
                     (config.preferredBackend != m_impl->m_config.preferredBackend) ||
                     (config.deviceId != m_impl->m_config.deviceId);

  if (needsReinit)
  {
    CLog::Log(LOGINFO, "GPUAccelerator: Configuration change requires reinitialization");
    return false; // Caller should reinitialize
  }

  // Update settings that don't require reinit
  m_impl->m_config = config;
  return true;
}

std::string CGPUAccelerator::GetStatusMessage() const
{
  return m_impl->m_statusMessage;
}

GPUBackend CGPUAccelerator::DetectBestBackend()
{
  return Impl::DetectBestBackendInternal();
}

bool CGPUAccelerator::IsBackendAvailable(GPUBackend backend)
{
  return Impl::IsBackendAvailableInternal(backend);
}

} // namespace SEMANTIC
} // namespace KODI
