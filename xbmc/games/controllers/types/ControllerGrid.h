/*
 *  Copyright (C) 2017-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "ControllerTree.h"
#include "games/controllers/ControllerTypes.h"

#include <string>
#include <vector>

namespace KODI
{
namespace GAME
{
/*!
 * \ingroup games
 *
 * \brief Vertex in the grid of controllers
 */
struct ControllerVertex
{
  bool bVisible = true;
  bool bConnected = false;
  ControllerPtr controller; // Mandatory if connected
  PORT_TYPE portType = PORT_TYPE::UNKNOWN; // Optional
  std::string address; // Optional
  ControllerVector compatible; // Compatible controllers
};

/*!
 * \ingroup games
 *
 * \brief Column of controllers in the grid
 */
struct ControllerColumn
{
  std::vector<ControllerVertex> vertices;
};

/*!
 * \ingroup games
 *
 * \brief Collection of controllers in a grid layout
 */
using ControllerGrid = std::vector<ControllerColumn>;

/*!
 * \ingroup games
 *
 * \brief Class to encapsulate grid operations
 */
class CControllerGrid
{
public:
  CControllerGrid() = default;
  CControllerGrid(const CControllerGrid& other) = default;
  ~CControllerGrid();

  /*!
   * \brief Create a grid from a controller tree
   */
  void SetControllerTree(const CControllerTree& controllerTree);

  /*!
   * \brief Get the width of the controller grid
   */
  unsigned int GetWidth() const { return static_cast<unsigned int>(m_grid.size()); }

  /*!
   * \brief Get the height (deepest controller) of the controller grid
   *
   * The height is cached when the controller grid is created to avoid
   * iterating the grid
   */
  unsigned int GetHeight() const { return m_height; }

  /*!
   * \brief Access the controller grid
   */
  const ControllerGrid& GetGrid() const { return m_grid; }

  /*!
   * \brief Get the controllers in use by the specified player
   *
   * \param playerIndex The column in the grid to get controllers from
   */
  ControllerVector GetControllers(unsigned int playerIndex) const;

private:
  /*!
   * \brief Directions of vertex traversal
   */
  enum class GRID_DIRECTION
  {
    RIGHT,
    DOWN,
  };

  /*!
   * \brief Add ports to the grid
   *
   * \param ports The ports on a console or controller
   * \param[out] grid The controller grid being created
   *
   * \return The height of the grid determined by the maximum column height
   */
  static unsigned int AddPorts(const PortVec& ports, ControllerGrid& grid);

  /*!
   * \brief Draw a controller to the column at the specified height
   *
   * \param port The controller's port node
   * \param height The height to draw the controller at
   * \param column[in/out] The column to draw to
   * \param grid[in/out] The grid to add additional columns to
   *
   * \return The height of the grid
   */
  static unsigned int AddController(const CPortNode& port,
                                    unsigned int height,
                                    std::vector<ControllerVertex>& column,
                                    ControllerGrid& grid);

  /*!
   * \brief Draw a series of controllers to the grid at the specified height
   *
   * \param ports The ports of the controllers to draw
   * \param height The height to start drawing the controllers at
   * \param bSkipFirst True if the first controller has already been drawn to
   *                   a column, false to start drawing at the first controller
   * \param grid[in/out] The grid to add columns to
   *
   * \return The height of the grid
   */
  static unsigned int AddHub(const PortVec& ports,
                             unsigned int height,
                             bool bSkipFirst,
                             ControllerGrid& grid);

  /*!
   * \brief Draw an invisible vertex to the column
   *
   * \param[in/out] column The column in a controller grid
   */
  static void AddInvisible(std::vector<ControllerVertex>& column);

  /*!
   * \brief Fill all columns with invisible vertices until the specified height
   *
   * \param height The height to make all columns
   * \param[in/out] grid The grid to update
   */
  static void SetHeight(unsigned int height, ControllerGrid& grid);

  /*!
   * \brief Get the direction of traversal for the next vertex
   *
   * \param node The node in the controller tree being visited
   *
   * \return The direction of the next vertex, or GRID_DIRECTION::UNKNOWN if
   *         unknown
   */
  static GRID_DIRECTION GetDirection(const CControllerNode& node);

  ControllerGrid m_grid;
  unsigned int m_height = 0;
};
} // namespace GAME
} // namespace KODI
