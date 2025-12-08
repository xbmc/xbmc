/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "ControllerGrid.h"

#include "games/controllers/Controller.h"
#include "utils/log.h"

#include <algorithm>
#include <utility>

using namespace KODI;
using namespace GAME;

CControllerGrid::~CControllerGrid() = default;

void CControllerGrid::SetControllerTree(const CControllerTree& controllerTree)
{
  // Clear the result
  m_grid.clear();

  m_height = AddPorts(controllerTree.GetPorts(), m_grid);
  SetHeight(m_height, m_grid);
}

ControllerVector CControllerGrid::GetControllers(unsigned int playerIndex) const
{
  ControllerVector controllers;

  if (playerIndex < m_grid.size())
  {
    for (const auto& controllerVertex : m_grid[playerIndex].vertices)
    {
      if (controllerVertex.controller)
        controllers.emplace_back(controllerVertex.controller);
    }
  }

  return controllers;
}

unsigned int CControllerGrid::AddPorts(const PortVec& ports, ControllerGrid& grid)
{
  unsigned int height = 0;

  auto itKeyboard =
      std::find_if(ports.begin(), ports.end(),
                   [](const CPortNode& port) { return port.GetPortType() == PORT_TYPE::KEYBOARD; });

  auto itMouse =
      std::find_if(ports.begin(), ports.end(),
                   [](const CPortNode& port) { return port.GetPortType() == PORT_TYPE::MOUSE; });

  auto itController = std::find_if(ports.begin(), ports.end(),
                                   [](const CPortNode& port)
                                   { return port.GetPortType() == PORT_TYPE::CONTROLLER; });

  // Keyboard and mouse are not allowed to have ports because they might
  // overlap with controllers
  if (itKeyboard != ports.end() && itKeyboard->GetActiveController().GetHub().HasPorts())
  {
    CLog::Log(LOGERROR, "Found keyboard with controller ports, skipping");
    itKeyboard = ports.end();
  }
  if (itMouse != ports.end() && itMouse->GetActiveController().GetHub().HasPorts())
  {
    CLog::Log(LOGERROR, "Found mouse with controller ports, skipping");
    itMouse = ports.end();
  }

  if (itController != ports.end())
  {
    // Add controller ports
    bool bFirstPlayer = true;
    for (const CPortNode& port : ports)
    {
      ControllerColumn column;

      if (port.GetPortType() == PORT_TYPE::CONTROLLER)
      {
        // Add controller
        height =
            std::max(height, AddController(port, static_cast<unsigned int>(column.vertices.size()),
                                           column.vertices, grid));

        if (bFirstPlayer)
        {
          bFirstPlayer = false;

          // Keyboard and mouse are added below the first controller
          if (itKeyboard != ports.end())
            height =
                std::max(height, AddController(*itKeyboard,
                                               static_cast<unsigned int>(column.vertices.size()),
                                               column.vertices, grid));
          if (itMouse != ports.end())
            height = std::max(
                height, AddController(*itMouse, static_cast<unsigned int>(column.vertices.size()),
                                      column.vertices, grid));
        }
      }

      if (!column.vertices.empty())
        grid.emplace_back(std::move(column));
    }
  }
  else
  {
    // No controllers, add keyboard and mouse
    ControllerColumn column;

    if (itKeyboard != ports.end())
      height = std::max(height, AddController(*itKeyboard,
                                              static_cast<unsigned int>(column.vertices.size()),
                                              column.vertices, grid));
    if (itMouse != ports.end())
      height = std::max(height,
                        AddController(*itMouse, static_cast<unsigned int>(column.vertices.size()),
                                      column.vertices, grid));

    if (!column.vertices.empty())
      grid.emplace_back(std::move(column));
  }

  return height;
}

unsigned int CControllerGrid::AddController(const CPortNode& port,
                                            unsigned int height,
                                            std::vector<ControllerVertex>& column,
                                            ControllerGrid& grid)
{
  // Add spacers
  while (column.size() < height)
    AddInvisible(column);

  const CControllerNode& activeController = port.GetActiveController();

  // Add vertex
  ControllerVertex vertex;
  vertex.bVisible = true;
  vertex.bConnected = port.IsConnected();
  vertex.portType = port.GetPortType();
  vertex.controller = activeController.GetController();
  vertex.address = activeController.GetControllerAddress();
  for (const CControllerNode& node : port.GetCompatibleControllers())
    vertex.compatible.emplace_back(node.GetController());
  column.emplace_back(std::move(vertex));

  height++;

  // Process ports
  const PortVec& ports = activeController.GetHub().GetPorts();
  if (!ports.empty())
  {
    switch (GetDirection(activeController))
    {
      case GRID_DIRECTION::RIGHT:
      {
        height = std::max(height, AddHub(ports, height - 1, false, grid));
        break;
      }
      case GRID_DIRECTION::DOWN:
      {
        const unsigned int row = height;

        // Add the first controller to the column
        const CPortNode firstController = ports.at(0);
        height = std::max(height, AddController(firstController, row, column, grid));

        // Add the remaining controllers on the same row
        height = std::max(height, AddHub(ports, row, true, grid));

        break;
      }
    }
  }

  return height;
}

unsigned int CControllerGrid::AddHub(const PortVec& ports,
                                     unsigned int height,
                                     bool bSkipFirst,
                                     ControllerGrid& grid)
{
  const unsigned int row = height;

  unsigned int port = 0;
  for (const auto& controllerPort : ports)
  {
    // If controller has no player, it has already added the hub's first controller
    if (bSkipFirst && port == 0)
      continue;

    // Add a column for this controller
    grid.emplace_back();
    ControllerColumn& column = grid.back();

    height = std::max(height, AddController(controllerPort, row, column.vertices, grid));

    port++;
  }

  return height;
}

void CControllerGrid::AddInvisible(std::vector<ControllerVertex>& column)
{
  ControllerVertex vertex;
  vertex.bVisible = false;
  column.emplace_back(std::move(vertex));
}

void CControllerGrid::SetHeight(unsigned int height, ControllerGrid& grid)
{
  for (auto& column : grid)
  {
    while (static_cast<unsigned int>(column.vertices.size()) < height)
      AddInvisible(column.vertices);
  }
}

CControllerGrid::GRID_DIRECTION CControllerGrid::GetDirection(const CControllerNode& node)
{
  // Hub controllers are added horizontally, one per row.
  //
  // If the current controller offers a player spot, the row starts to the
  // right at the same height as the controller.
  //
  // Otherwise, to row starts below the current controller in the same
  // column.
  if (node.ProvidesInput())
    return GRID_DIRECTION::RIGHT;
  else
    return GRID_DIRECTION::DOWN;
}
