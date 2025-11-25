/*
 *  Copyright (C) 2025 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <string>

namespace KODI
{
namespace SEMANTIC
{

/*!
 * \brief GPU backend type enumeration
 *
 * Defines the available GPU execution providers for ONNX Runtime.
 * Each provider targets specific hardware vendors and platforms.
 */
enum class GPUBackend
{
  NONE,      //!< CPU only (no GPU acceleration)
  CUDA,      //!< NVIDIA CUDA (NVIDIA GPUs on Windows/Linux)
  DIRECTML,  //!< DirectML (Windows - all GPU vendors)
  OPENCL,    //!< OpenCL (Cross-platform - AMD/Intel/NVIDIA)
  COREML,    //!< CoreML (Apple Silicon and Intel Macs)
  ROCM,      //!< AMD ROCm (AMD GPUs on Linux)
  TENSORRT   //!< NVIDIA TensorRT (optimized NVIDIA inference)
};

/*!
 * \brief GPU device information
 *
 * Contains information about an available GPU device including
 * vendor, model, memory, and capabilities.
 */
struct GPUDeviceInfo
{
  int deviceId{0};                  //!< Device index (0-based)
  GPUBackend backend{GPUBackend::NONE}; //!< Backend type
  std::string name;                 //!< Device name/model
  std::string vendor;               //!< Vendor (NVIDIA, AMD, Intel, Apple)
  size_t totalMemoryMB{0};          //!< Total VRAM in megabytes
  size_t availableMemoryMB{0};      //!< Available VRAM in megabytes
  bool supportsCompute{false};      //!< Supports compute operations
  int computeCapability{0};         //!< Compute capability (CUDA) or version
  bool isDefault{false};            //!< Is the default/primary device
};

/*!
 * \brief GPU performance metrics
 *
 * Runtime statistics about GPU utilization and performance.
 * Not all metrics may be available on all platforms.
 */
struct GPUMetrics
{
  float utilizationPercent{0.0f};   //!< GPU utilization (0-100%)
  float memoryUsedMB{0.0f};         //!< Current memory usage in MB
  float memoryTotalMB{0.0f};        //!< Total memory in MB
  float temperatureCelsius{0.0f};   //!< GPU temperature (if available)
  float powerWatts{0.0f};           //!< Power consumption (if available)
  int clockSpeedMHz{0};             //!< Current clock speed (if available)
  bool metricsAvailable{false};     //!< Whether metrics are supported
};

/*!
 * \brief GPU acceleration configuration
 *
 * Settings for controlling GPU acceleration behavior.
 */
struct GPUConfig
{
  bool enabled{false};              //!< Enable GPU acceleration
  GPUBackend preferredBackend{GPUBackend::CUDA}; //!< Preferred backend
  int deviceId{0};                  //!< GPU device to use (0 = auto/default)
  size_t memoryLimitMB{0};          //!< Memory limit (0 = no limit)
  int batchSize{32};                //!< Batch size for GPU inference
  bool enablePinnedMemory{true};    //!< Use pinned memory for transfers
  bool enableAsyncTransfer{true};   //!< Async data transfer
  bool fallbackToCPU{true};         //!< Fallback to CPU if GPU fails
};

/*!
 * \brief GPU acceleration status
 *
 * Current state of GPU acceleration.
 */
enum class GPUStatus
{
  DISABLED,           //!< GPU acceleration disabled by user
  NOT_AVAILABLE,      //!< No compatible GPU found
  INITIALIZING,       //!< GPU is being initialized
  READY,              //!< GPU ready for inference
  ERROR,              //!< GPU error occurred
  FALLBACK_TO_CPU     //!< Using CPU fallback
};

/*!
 * \brief Convert GPUBackend to string
 */
inline const char* GPUBackendToString(GPUBackend backend)
{
  switch (backend)
  {
    case GPUBackend::NONE:
      return "CPU";
    case GPUBackend::CUDA:
      return "CUDA";
    case GPUBackend::DIRECTML:
      return "DirectML";
    case GPUBackend::OPENCL:
      return "OpenCL";
    case GPUBackend::COREML:
      return "CoreML";
    case GPUBackend::ROCM:
      return "ROCm";
    case GPUBackend::TENSORRT:
      return "TensorRT";
    default:
      return "Unknown";
  }
}

/*!
 * \brief Convert GPUStatus to string
 */
inline const char* GPUStatusToString(GPUStatus status)
{
  switch (status)
  {
    case GPUStatus::DISABLED:
      return "Disabled";
    case GPUStatus::NOT_AVAILABLE:
      return "Not Available";
    case GPUStatus::INITIALIZING:
      return "Initializing";
    case GPUStatus::READY:
      return "Ready";
    case GPUStatus::ERROR:
      return "Error";
    case GPUStatus::FALLBACK_TO_CPU:
      return "CPU Fallback";
    default:
      return "Unknown";
  }
}

} // namespace SEMANTIC
} // namespace KODI
