/*
 *  Copyright (C) 2005-2026 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

// Initialize window
constexpr const int GUI_MSG_WINDOW_INIT = 1;

// Deinitialize window
constexpr const int GUI_MSG_WINDOW_DEINIT = 2;

// Reset window to initial state
constexpr const int GUI_MSG_WINDOW_RESET = 27;

// Set focus to control param1=up/down/left/right
constexpr const int GUI_MSG_SETFOCUS = 3;

// Control lost focus
constexpr const int GUI_MSG_LOSTFOCUS = 4;

// Control has been clicked
constexpr const int GUI_MSG_CLICKED = 5;

// Set control visible
constexpr const int GUI_MSG_VISIBLE = 6;

// Set control hidden
constexpr const int GUI_MSG_HIDDEN = 7;

// Enable control
constexpr const int GUI_MSG_ENABLED = 8;

// Disable control
constexpr const int GUI_MSG_DISABLED = 9;

// Control = selected
constexpr const int GUI_MSG_SET_SELECTED = 10;

// Control = not selected
constexpr const int GUI_MSG_SET_DESELECTED = 11;

// Add label control (for controls supporting more then 1 label)
constexpr const int GUI_MSG_LABEL_ADD = 12;

// Set the label of a control
constexpr const int GUI_MSG_LABEL_SET = 13;

// Set the label2 of a control
constexpr const int GUI_MSG_LABEL2_SET = 17;

// Clear all labels of a control
constexpr const int GUI_MSG_LABEL_RESET = 14;

// Ask control to return the selected item
constexpr const int GUI_MSG_ITEM_SELECTED = 15;

// Ask control to select a specific item
constexpr const int GUI_MSG_ITEM_SELECT = 16;

// Show or hide the range indicator in a spin control
constexpr const int GUI_MSG_SHOWRANGE = 18;

// Should go to fullscreen window (vis or video)
constexpr const int GUI_MSG_FULLSCREEN = 19;

// User has clicked on a button with <execute> tag
constexpr const int GUI_MSG_EXECUTE = 20;

// Message will be send to all active and inactive(!) windows, all active
// modal and modeless dialogs. dwParam1 must contain an additional message the
// windows should react on.
constexpr const int GUI_MSG_NOTIFY_ALL = 21;

// Message is sent to all windows to refresh all thumbs
constexpr const int GUI_MSG_REFRESH_THUMBS = 22;

// Message is sent to the window from the base control class when it's been
// asked to move. dwParam1 contains direction.
constexpr const int GUI_MSG_MOVE = 23;

// Bind label control (for controls supporting more then 1 label)
constexpr const int GUI_MSG_LABEL_BIND = 24;

// A control has become focused
constexpr const int GUI_MSG_FOCUSED = 26;

// A page control has changed the page number
constexpr const int GUI_MSG_PAGE_CHANGE = 28;

// Message sent to all listing controls telling them to refresh their item
// layouts
constexpr const int GUI_MSG_REFRESH_LIST = 29;

// Page up
constexpr const int GUI_MSG_PAGE_UP = 30;

// Page down
constexpr const int GUI_MSG_PAGE_DOWN = 31;

// Instruct the control to MoveUp or MoveDown by offset amount
constexpr const int GUI_MSG_MOVE_OFFSET = 32;

// Instruct a control to set its type appropriately
constexpr const int GUI_MSG_SET_TYPE = 33;

/*!
 * \brief Message indicating the window has been resized
 *
 * Any controls that keep stored sizing information based on aspect ratio or
 * window size should recalculate sizing information.
 */
constexpr const int GUI_MSG_WINDOW_RESIZE = 34;

/*!
 * \brief Message indicating loss of renderer, prior to reset
 *
 * Any controls that keep shared resources should free them on receipt of this
 * message, as the renderer is about to be reset.
 */
constexpr const int GUI_MSG_RENDERER_LOST = 35;

/*!
 * \brief Message indicating regain of renderer, after reset
 *
 * Any controls that keep shared resources may reallocate them now that the
 * renderer is back.
 */
constexpr const int GUI_MSG_RENDERER_RESET = 36;

/*!
 * \brief A control wishes to have (or release) exclusive access to mouse actions
 */
constexpr const int GUI_MSG_EXCLUSIVE_MOUSE = 37;

/*!
 * \brief A request for supported gestures is made
 */
constexpr const int GUI_MSG_GESTURE_NOTIFY = 38;

/*!
 * \brief A request to add a control
 */
constexpr const int GUI_MSG_ADD_CONTROL = 39;

/*!
 * \brief A request to remove a control
 */
constexpr const int GUI_MSG_REMOVE_CONTROL = 40;

/*!
 * \brief A request to unfocus all currently focused controls
 */
constexpr const int GUI_MSG_UNFOCUS_ALL = 41;

/*!
 * \brief Set the text content of an edit control or other text-based control
 */
constexpr const int GUI_MSG_SET_TEXT = 42;

/*!
 * \brief Initialize a window before displaying it
 */
constexpr const int GUI_MSG_WINDOW_LOAD = 43;

/*!
 * \brief Notify that an edit control's input validity state has changed
 */
constexpr const int GUI_MSG_VALIDITY_CHANGED = 44;

/*!
 * \brief Check whether a button is selected
 */
constexpr const int GUI_MSG_IS_SELECTED = 45;

/*!
 * \brief Bind a set of labels to a spin (or similar) control
 */
constexpr const int GUI_MSG_SET_LABELS = 46;

/*!
 * \brief Set the filename for an image control
 */
constexpr const int GUI_MSG_SET_FILENAME = 47;

/*!
 * \brief Get the filename of an image control
 */
constexpr const int GUI_MSG_GET_FILENAME = 48;

/*!
 * \brief The user interface is ready for usage
 */
constexpr const int GUI_MSG_UI_READY = 49;

/*!
 * \brief Called every 500ms to allow time dependent updates
 */
constexpr const int GUI_MSG_REFRESH_TIMER = 50;

/*!
 * \brief Called if state has changed which could lead to GUI changes
 */
constexpr const int GUI_MSG_STATE_CHANGED = 51;

/*!
 * \brief Called when a subtitle download has finished
 */
constexpr const int GUI_MSG_SUBTITLE_DOWNLOADED = 52;

/*!
 * \brief Reset a multiimage to its initial state
 */
constexpr const int GUI_MSG_RESET_MULTI_IMAGE = 53;

/*!
 * \brief Base offset for application-defined user messages (1000)
 */
constexpr const int GUI_MSG_USER = 1000;

/*!
 * \brief Complete to get codingtable page
 */
constexpr const int GUI_MSG_CODINGTABLE_LOOKUP_COMPLETED = 65000;
