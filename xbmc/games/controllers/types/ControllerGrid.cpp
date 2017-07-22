/*
 *      Copyright (C) 2017 Team Kodi
 *      http://kodi.tv
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this Program; see the file COPYING.  If not, see
 *  <http://www.gnu.org/licenses/>.
 *
 */

#include "ControllerGrid.h"
#include "games/controllers/Controller.h"
#include "games/controllers/ControllerTranslator.h"
#include "utils/log.h"

#include <algorithm>
#include <utility>

using namespace KODI;
using namespace GAME;

CControllerGrid::CControllerGrid()
{
}

CControllerGrid::CControllerGrid(const CControllerGrid &other) :
  m_grid(other.m_grid),
  m_height(other.m_height)
{
}

CControllerGrid::~CControllerGrid() = default;

void CControllerGrid::SetControllerTree(const CControllerTree &controllerTree)
{
  // Clear the result
  m_grid.clear();

  m_height = AddPorts(controllerTree.Ports(), m_grid);
  SetHeight(m_height, m_grid);
}

ControllerVector CControllerGrid::GetControllers(unsigned int playerIndex) const
{
  ControllerVector controllers;

  if (playerIndex < m_grid.size())
  {
    for (const auto &controllerVertex : m_grid[playerIndex].vertices)
    {
      if (controllerVertex.controller)
        controllers.emplace_back(controllerVertex.controller);
    }
  }

  return controllers;
}

unsigned int CControllerGrid::AddPorts(const ControllerPortVec &ports, ControllerGrid &grid)
{
  unsigned int height = 0;

  auto itKeyboard = std::find_if(ports.begin(), ports.end(),
    [](const CControllerPortNode &port)
    {
      return port.PortType() == PORT_TYPE::KEYBOARD;
    });

  auto itMouse = std::find_if(ports.begin(), ports.end(),
    [](const CControllerPortNode &port)
    {
      return port.PortType() == PORT_TYPE::MOUSE;
    });

  auto itController = std::find_if(ports.begin(), ports.end(),
    [](const CControllerPortNode &port)
    {
      return port.PortType() == PORT_TYPE::CONTROLLER;
    });

  // Keyboard and mouse are not allowed to have ports because they might
  // overlap with controllers
  if (itKeyboard != ports.end() && itKeyboard->ActiveController().Hub().HasPorts())
  {
    CLog::Log(LOGERROR, "Found keyboard with controller ports, skipping");
    itKeyboard = ports.end();
  }
  if (itMouse != ports.end() && itMouse->ActiveController().Hub().HasPorts())
  {
    CLog::Log(LOGERROR, "Found mouse with controller ports, skipping");
    itMouse = ports.end();
  }

  if (itController != ports.end())
  {
    // Add controller ports
    bool bFirstPlayer = true;
    for (const CControllerPortNode &port : ports)
    {
      ControllerColumn column;

      if (port.PortType() == PORT_TYPE::CONTROLLER)
      {
        // Add controller
        height = std::max(height, AddController(port, column.vertices.size(), column.vertices, grid));

        if (bFirstPlayer == true)
        {
          bFirstPlayer = false;

          // Keyboard and mouse are added below the first controller
          if (itKeyboard != ports.end())
            height = std::max(height, AddController(*itKeyboard, column.vertices.size(), column.vertices, grid));
          if (itMouse != ports.end())
            height = std::max(height, AddController(*itMouse, column.vertices.size(), column.vertices, grid));
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
      height = std::max(height, AddController(*itKeyboard, column.vertices.size(), column.vertices, grid));
    if (itMouse != ports.end())
      height = std::max(height, AddController(*itMouse, column.vertices.size(), column.vertices, grid));

    if (!column.vertices.empty())
      grid.emplace_back(std::move(column));
  }

  return height;
}

unsigned int CControllerGrid::AddController(const CControllerPortNode &port, unsigned int height,
                                            std::vector<ControllerVertex> &column, ControllerGrid &grid)
{
  // Add spacers
  while (column.size() < height)
    AddInvisible(column);

  const CControllerNode &activeController = port.ActiveController();

  // Add vertex
  ControllerVertex vertex;
  vertex.bVisible = true;
  vertex.bConnected = port.Connected();
  vertex.portType = port.PortType();
  vertex.controller = activeController.Controller();
  vertex.address = activeController.Address();
  for (const CControllerNode &node : port.CompatibleControllers())
    vertex.compatible.emplace_back(node.Controller());
  column.emplace_back(std::move(vertex));

  height++;

  // Process ports
  const ControllerPortVec &ports = activeController.Hub().Ports();
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
      const CControllerPortNode &firstController = ports.at(0);
      height = std::max(height, AddController(firstController, row, column, grid));

      // Add the remaining controllers on the same row
      height = std::max(height, AddHub(ports, row, true, grid));

      break;
    }
    }
  }

  return height;
}

unsigned int CControllerGrid::AddHub(const ControllerPortVec &ports, unsigned int height, bool bSkipFirst,
                                     ControllerGrid &grid)
{
  const unsigned int row = height;

  unsigned int port = 0;
  for (const auto &controllerPort : ports)
  {
    // If controller has no player, it has already added the hub's first controller
    if (bSkipFirst && port == 0)
      continue;

    // Add a column for this controller
    grid.emplace_back();
    ControllerColumn &column = grid.back();

    height = std::max(height, AddController(controllerPort, row, column.vertices, grid));

    port++;
  }

  return height;
}

void CControllerGrid::AddInvisible(std::vector<ControllerVertex> &column)
{
  ControllerVertex vertex;
  vertex.bVisible = false;
  column.emplace_back(std::move(vertex));
}

void CControllerGrid::SetHeight(unsigned int height, ControllerGrid &grid)
{
  for (auto &column : grid)
  {
    while (column.vertices.size() < height)
      AddInvisible(column.vertices);
  }
}

CControllerGrid::GRID_DIRECTION CControllerGrid::GetDirection(const CControllerNode &node)
{
  // Hub controllers are added horizontally, one per row.
  //
  // If the current controller offers a player spot, the row starts to the
  // right at the same hight as the controller.
  //
  // Otherwise, to row starts below the current controller in the same
  // column.
  if (node.ProvidesInput())
    return GRID_DIRECTION::RIGHT;
  else
    return GRID_DIRECTION::DOWN;
}
