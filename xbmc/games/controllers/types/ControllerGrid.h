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
   * \brief Column of controllers in the grid
   */
  struct ControllerColumn
  {
    std::vector<ControllerVertex> vertices;
  };

  /*!
   * \brief Collection of controllers in a grid layout
   */
  using ControllerGrid = std::vector<ControllerColumn>;

  /*!
   * \brief Class to encapsulate grid operations
   */
  class CControllerGrid
  {
  public:
    CControllerGrid();
    CControllerGrid(const CControllerGrid &other);
    ~CControllerGrid();

    /*!
     * \brief Create a grid from a controller tree
     */
    void SetControllerTree(const CControllerTree &controllerTree);

    /*!
     * \brief Get the width of the controller grid
     */
    unsigned int Width() const { return m_grid.size(); }

    /*!
     * \brief Get the height (deepest controller) of the controller grid
     *
     * The height is cached when the controller grid is created to avoid
     * iterating the grid
     */
    unsigned int Height() const { return m_height; }

    /*!
     * \brief Access the controller grid
     */
    const ControllerGrid &Grid() const { return m_grid; }

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
    static unsigned int AddPorts(const ControllerPortVec &ports, ControllerGrid &grid);

    /*!
     * \brief Draw a controller to the column at the specified height
     *
     * \param port The controller's port node
     * \param height The hight to draw the controller at
     * \param column[in/out] The column to draw to
     * \param grid[in/out] The grid to add additional columns to
     *
     * \return The height of the grid
     */
    static unsigned int AddController(const CControllerPortNode &port, unsigned int height,
                                      std::vector<ControllerVertex> &column, ControllerGrid &grid);

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
    static unsigned int AddHub(const ControllerPortVec &ports, unsigned int height, bool bSkipFirst,
                               ControllerGrid &grid);

    /*!
     * \brief Draw an invisible vertex to the column
     *
     * \param[in/out] column The column in a controller grid
     */
    static void AddInvisible(std::vector<ControllerVertex> &column);

    /*!
     * \brief Fill all columns with invisible vertices until the specified height
     *
     * \param height The height to make all columns
     * \param[in/out] grid The grid to update
     */
    static void SetHeight(unsigned int height, ControllerGrid &grid);

    /*!
     * \brief Get the direction of traversal for the next vertex
     *
     * \param node The node in the controller tree being visited
     *
     * \return The direction of the next vertex, or GRID_DIRECTION::UNKNOWN if
     *         unknown
     */
    static GRID_DIRECTION GetDirection(const CControllerNode &node);

    ControllerGrid m_grid;
    unsigned int m_height = 0;
  };
}
}
