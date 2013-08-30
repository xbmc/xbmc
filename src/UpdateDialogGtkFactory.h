#pragma once

class UpdateDialog;

/** Factory for loading the GTK version of the update dialog
 * dynamically at runtime if the GTK libraries are available.
 */
class UpdateDialogGtkFactory
{
	public:
		static UpdateDialog* createDialog();
};

