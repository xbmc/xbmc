/*
 *      Copyright © 2006-2010 Team XBMC
 *      http://www.xbmc.org
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
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

using System;
using System.Windows.Forms;
using TeamXBMC.TranslatorCore;

namespace TeamXBMC.Translator
{
	/// <summary>
	/// Summary description for ListViewItemString.
	/// </summary>
	public sealed class ListViewItemString : ListViewItem
	{
		public ListViewItemString(TranslatorItem item)
		{
			Text=item.StringTranslated.Id.ToString();
			SubItems.Add(item.StringTranslated.Text);
			SubItems.Add(item.StringOriginal.Text);
			item.StringTranslated.stringUpdated+=new StringItem.StringUpdatedDelegate(StringUpdated);
		}

		/// <summary>
		/// Is called when the string a ListViewItemString represents is changed
		/// </summary>
		private void StringUpdated(StringItem item)
		{
			SubItems[1].Text=item.Text;
		}
	}
}
