/*
 *  Copyright (C) 2014-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include "games/controllers/ControllerTypes.h"
#include "input/InputTypes.h"
#include "input/joysticks/JoystickTypes.h"

#include <string>
#include <vector>

class CEvent;

namespace KODI
{
namespace GAME
{
class CPhysicalFeature;

/*!
 * \ingroup games
 *
 * \brief A list populated by installed controllers for the controller
 *        configuration window
 *
 * The configuration window presents a list of controllers. Also on the screen
 * is a list of features (\ref IFeatureList) belonging to that controller.
 *
 * The configuration utility reacts to several events:
 *
 *   1) When a controller is focused, the feature list is populated with the
 *      controller's features.
 *
 *   2) When a feature is selected, the user is prompted for controller input.
 *      This initiates a "wizard" (\ref IConfigurationWizard) that walks the
 *      user through the subsequent features.
 *
 *   3) When the wizard's active feature loses focus, the wizard is cancelled
 *      and the prompt for input ends.
 */
class IControllerList
{
public:
  virtual ~IControllerList() = default;

  /*!
   * \brief  Initialize the resource
   *
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
   *
   * \param controllerId The controller to focus, or empty to leave focus unchanged
   *
   * \return True if the list was changed
   */
  virtual bool Refresh(const std::string& controllerId) = 0;

  /*
   * \brief  The specified controller has been focused
   *
   * \param  controllerIndex The index of the controller being focused
   */
  virtual void OnFocus(unsigned int controllerIndex) = 0;

  /*!
   * \brief  The specified controller has been selected
   *
   * \param  controllerIndex The index of the controller being selected
   */
  virtual void OnSelect(unsigned int controllerIndex) = 0;

  /*!
   * \brief Get the index of the focused controller
   *
   * \return The index of the focused controller, or -1 if no controller has been focused yet
   */
  virtual int GetFocusedController() const = 0;

  /*!
   * \brief Reset the focused controller
   */
  virtual void ResetController(void) = 0;
};

/*!
 * \ingroup games
 *
 * \brief A list populated by the controller's features
 *
 * The feature list is populated by features (\ref IFeatureButton) belonging
 * to the current controller selected in the controller list
 * (\ref IControllerList).
 */
class IFeatureList
{
public:
  virtual ~IFeatureList() = default;

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
   * \brief Check if the feature type has any buttons in the GUI
   * \param The type of the feature being added to the GUI
   * \return True if the type is support, false otherwise
   */
  virtual bool HasButton(JOYSTICK::FEATURE_TYPE type) const = 0;

  /*!
   * \brief Load the features for the specified controller
   * \param controller The controller to load
   */
  virtual void Load(const ControllerPtr& controller) = 0;

  /*!
   * \brief  Focus has been set to the specified GUI button
   * \param  buttonIndex The index of the button being focused
   */
  virtual void OnFocus(unsigned int buttonIndex) = 0;

  /*!
   * \brief  The specified GUI button has been selected
   * \param  buttonIndex The index of the button being selected
   */
  virtual void OnSelect(unsigned int buttonIndex) = 0;
};

/*!
 * \ingroup games
 *
 * \brief A GUI button in a feature list (\ref IFeatureList)
 */
class IFeatureButton
{
public:
  virtual ~IFeatureButton() = default;

  /*!
   * \brief Get the feature represented by this button
   */
  virtual const CPhysicalFeature& Feature(void) const = 0;

  /*!
   * \brief Allow the wizard to include this feature in a list of buttons
   *        to map
   */
  virtual bool AllowWizard() const { return true; }

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
   * \brief Get the direction of the next analog stick or relative pointer
   *        prompt
   * \return The next direction to be prompted, or UNKNOWN if this isn't a
   *         cardinal feature or the prompt is finished
   */
  virtual INPUT::CARDINAL_DIRECTION GetCardinalDirection(void) const = 0;

  /*!
   * \brief Get the direction of the next wheel prompt
   * \return The next direction to be prompted, or UNKNOWN if this isn't a
   *         wheel or the prompt is finished
   */
  virtual JOYSTICK::WHEEL_DIRECTION GetWheelDirection(void) const = 0;

  /*!
   * \brief Get the direction of the next throttle prompt
   * \return The next direction to be prompted, or UNKNOWN if this isn't a
   *         throttle or the prompt is finished
   */
  virtual JOYSTICK::THROTTLE_DIRECTION GetThrottleDirection(void) const = 0;

  /*!
   * \brief True if the button is waiting for a key press
   */
  virtual bool NeedsKey() const { return false; }

  /*!
   * \brief Set the pressed key that the user will be prompted to map
   *
   * \param key The key that was pressed
   */
  virtual void SetKey(const CPhysicalFeature& key) {}

  /*!
   * \brief Reset button after prompting for input has finished
   */
  virtual void Reset(void) = 0;
};

/*!
 * \ingroup games
 *
 * \brief A wizard to direct user input
 *
 * The wizard is run for the current controller selected in the controller list
 * (\ref IControllerList). It prompts the user for input for each feature
 * (\ref IFeatureButton) in the feature list (\ref IFeatureList).
 */
class IConfigurationWizard
{
public:
  virtual ~IConfigurationWizard() = default;

  /*!
   * \brief Start the wizard for the specified buttons
   * \param controllerId The controller ID being mapped
   * \param buttons The buttons to map
   */
  virtual void Run(const std::string& strControllerId,
                   const std::vector<IFeatureButton*>& buttons) = 0;

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

  /*!
   * \brief Register a key by its keycode
   * \param key A key with a valid keycode
   *
   * This should be called before Run(). It allows the user to choose a key
   * to map instead of scrolling through a long list.
   */
  virtual void RegisterKey(const CPhysicalFeature& key) = 0;

  /*!
   * \brief Unregister all registered keys
   */
  virtual void UnregisterKeys() = 0;
};
} // namespace GAME
} // namespace KODI
