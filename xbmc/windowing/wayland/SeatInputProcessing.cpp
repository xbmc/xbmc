/*
 *  Copyright (C) 2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "SeatInputProcessing.h"

#include <cassert>
#include <memory>

using namespace KODI::WINDOWING::WAYLAND;

CSeatInputProcessing::CSeatInputProcessing(wayland::surface_t const& inputSurface, IInputHandler& handler)
: m_inputSurface{inputSurface}, m_handler{handler}
{
}

void CSeatInputProcessing::AddSeat(CSeat* seat)
{
  assert(m_seats.find(seat->GetGlobalName()) == m_seats.end());
  auto& seatState = m_seats.emplace(seat->GetGlobalName(), seat).first->second;

  seatState.keyboardProcessor = std::make_unique<CInputProcessorKeyboard>(*this);
  seat->AddRawInputHandlerKeyboard(seatState.keyboardProcessor.get());
  seatState.pointerProcessor = std::make_unique<CInputProcessorPointer>(m_inputSurface, *this);
  seat->AddRawInputHandlerPointer(seatState.pointerProcessor.get());
  seatState.touchProcessor = std::make_unique<CInputProcessorTouch>(m_inputSurface);
  seat->AddRawInputHandlerTouch(seatState.touchProcessor.get());
}

void CSeatInputProcessing::RemoveSeat(CSeat* seat)
{
  auto seatStateI = m_seats.find(seat->GetGlobalName());
  if (seatStateI != m_seats.end())
  {
    seat->RemoveRawInputHandlerKeyboard(seatStateI->second.keyboardProcessor.get());
    seat->RemoveRawInputHandlerPointer(seatStateI->second.pointerProcessor.get());
    seat->RemoveRawInputHandlerTouch(seatStateI->second.touchProcessor.get());
    m_seats.erase(seatStateI);
  }
}

void CSeatInputProcessing::OnPointerEnter(std::uint32_t seatGlobalName, std::uint32_t serial)
{
  m_handler.OnSetCursor(seatGlobalName, serial);
  m_handler.OnEnter(InputType::POINTER);
}

void CSeatInputProcessing::OnPointerLeave()
{
  m_handler.OnLeave(InputType::POINTER);
}

void CSeatInputProcessing::OnPointerEvent(XBMC_Event& event)
{
  m_handler.OnEvent(InputType::POINTER, event);
}

void CSeatInputProcessing::OnKeyboardEnter()
{
  m_handler.OnEnter(InputType::KEYBOARD);
}

void CSeatInputProcessing::OnKeyboardLeave()
{
  m_handler.OnLeave(InputType::KEYBOARD);
}

void CSeatInputProcessing::OnKeyboardEvent(XBMC_Event& event)
{
  m_handler.OnEvent(InputType::KEYBOARD, event);
}

void CSeatInputProcessing::SetCoordinateScale(std::int32_t scale)
{
  for (auto& seatPair : m_seats)
  {
    seatPair.second.touchProcessor->SetCoordinateScale(scale);
    seatPair.second.pointerProcessor->SetCoordinateScale(scale);
  }
}
