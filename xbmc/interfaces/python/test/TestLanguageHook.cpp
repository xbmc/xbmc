/*
 *  Copyright (C) 2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "interfaces/python/LanguageHook.h"

#include <atomic>
#include <chrono>
#include <cstdint>
#include <thread>
#include <vector>

#include <gtest/gtest.h>

using namespace XBMCAddon;
using namespace std::chrono_literals;

namespace
{
constexpr int HOOK_COUNT = 64;

// The hook only uses the interpreter pointer as a map key.
PyInterpreterState* FakeInterpreter(int index)
{
  return reinterpret_cast<PyInterpreterState*>(static_cast<std::uintptr_t>(0x1000 + 0x10 * index));
}
} // namespace

TEST(TestPythonLanguageHook, IsAddonClassInstanceRegisteredWhileHooksChange)
{
  std::vector<AddonClass::Ref<Python::PythonLanguageHook>> hooks;
  for (int i = 0; i < HOOK_COUNT; ++i)
    hooks.emplace_back(new Python::PythonLanguageHook(FakeInterpreter(i)));

  std::atomic_bool stop{false};
  std::thread writer(
      [&]
      {
        while (!stop)
        {
          for (auto& hook : hooks)
            hook->RegisterMe();
          for (auto& hook : hooks)
            hook->UnregisterMe();
        }
      });

  auto* const unregistered = reinterpret_cast<AddonClass*>(std::uintptr_t{0x10});
  const auto end = std::chrono::steady_clock::now() + 2s;
  while (std::chrono::steady_clock::now() < end)
    EXPECT_FALSE(Python::PythonLanguageHook::IsAddonClassInstanceRegistered(unregistered));

  stop = true;
  writer.join();
  for (auto& hook : hooks)
    hook->UnregisterMe();
}
