/*
 *      Copyright (C) 2014-2016 Team Kodi
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

#include "games/controllers/ControllerTypes.h"
#include "input/joysticks/JoystickTypes.h"

#include <string>
#include <vector>

class CEvent;

/*!
 * \brief Controller configuration window
 *
 * The configuration window presents a list of controllers. Also on the screen
 * is a list of features belonging to that controller.
 *
 * The configuration utility reacts to several events:
 *
 *   1) When a controller is focused, the feature list is populated with the
 *      controller's features.
 *
 *   2) When a feature is selected, the user is prompted for controller input.
 *      This initiates a "wizard" that walks the user through the subsequent
 *      features.
 *
 *   3) When the wizard's active feature loses focus, the wizard is cancelled
 *      and the prompt for input ends.
 */
namespace GAME
{
  class CControllerFeature;

  /*!
   * \brief A list populated by installed controllers
   */
  class IControllerList
  {
  public:
    virtual ~IControllerList(void) { }

    /*!
     * \brief  Initialize the resource
     * \return true if the resource is initialized and can be used
     *         false if the resource failed to initialize and must not be used
     */
    virtual bool Initialize(void) = 0;

    /*!
     * \brief  Deinitialize the resource
     */
    virtual void Deinitialize(void) = 0;

    /*!
     * \brief Refresh the contents of the list
     * \return True if the list was changed
     */
    virtual bool Refresh(void) = 0;

    /*
     * \brief  The specified controller has been focused
     * \param  controllerIndex The index of the controller being focused
     */
    virtual void OnFocus(unsigned int controllerIndex) = 0;

    /*!
     * \brief  The specified controller has been selected
     * \param  controllerIndex The index of the controller being selected
     */
    virtual void OnSelect(unsigned int controllerIndex) = 0;

    /*!
     * \brief Reset the focused controller
     */
    virtual void ResetController(void) = 0;
  };

  /*!
   * \brief A list populated by the controller's features
   */
  class IFeatureList
  {
  public:
    virtual ~IFeatureList(void) { }

    /*!
     * \brief  Initialize the resource
     * \return true if the resource is initialized and can be used
     *         false if the resource failed to initialize and must not be used
     */
    virtual bool Initialize(void) = 0;

    /*!
     * \brief  Deinitialize the resource
     * \remark This must be called if Initialize() returned true
     */
    virtual void Deinitialize(void) = 0;

    /*!
     * \brief Load the features for the specified controller
     * \param controller The controller to load
     */
    virtual void Load(const ControllerPtr& controller) = 0;

    /*!
     * \brief  Focus has been set to the specified feature
     * \param  featureIndex The index of the feature being focused
     */
    virtual void OnFocus(unsigned int index) = 0;

    /*!
     * \brief  The specified feature has been selected
     * \param  featureIndex The index of the feature being selected
     */
    virtual void OnSelect(unsigned int index) = 0;
  };

  /*!
   * \brief A button in a feature list
   */
  class IFeatureButton
  {
  public:
    virtual ~IFeatureButton(void) { }

    /*!
     * \brief Get the feature represented by this button
     */
    virtual const CControllerFeature& Feature(void) const = 0;

    /*!
     * \brief Prompt the user for a single input element
     * \param waitEvent The event to block on while prompting for input
     * \return true if input was received (event fired), false if the prompt timed out
     *
     * After the button has finished prompting the user for all the input
     * elements it requires, this will return false until Reset() is called.
     */
    virtual bool PromptForInput(CEvent& waitEvent) = 0;

    /*!
     * \brief Check if the button supports further calls to PromptForInput()
     * \return true if the button requires no more input elements from the user
     */
    virtual bool IsFinished(void) const = 0;

    /*!
     * \brief Get the direction of the next analog stick prompt
     * \return The next direction to be prompted, or UNKNOWN if this isn't an
     *         analog stick or the prompt is finished
     */
    virtual JOYSTICK::CARDINAL_DIRECTION GetDirection(void) const = 0;

    /*!
     * \brief Reset button after prompting for input has finished
     */
    virtual void Reset(void) = 0;
  };

  /*!
   * \brief A wizard to direct user input
   */
  class IConfigurationWizard
  {
  public:
    virtual ~IConfigurationWizard(void) { }

    /*!
     * \brief Start the wizard at the specified feature
     * \param featureIndex The index of the feature to start at
     */
    virtual void Run(const std::string& strControllerId, const std::vector<IFeatureButton*>& buttons) = 0;

    /*!
     * \brief Callback for feature losing focus
     * \param button The feature button losing focus
     */
    virtual void OnUnfocus(IFeatureButton* button) = 0;

    /*!
     * \brief Abort a running wizard
     * \param bWait True if the call should block until the wizard is fully aborted
     * \return true if aborted, or false if the wizard wasn't running
     */
    virtual bool Abort(bool bWait = true) = 0;
  };
}
