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

namespace TeamXBMC.TranslatorCore
{
	/// <summary>
	/// State of a TranslatorItem
	/// </summary>
	public enum TranslationState
	{
		Unknown=0,
		Translated,
		Untranslated,
		Changed
	};

	/// <summary>
	/// Single item of a languge to be translated.
	/// Used in the TranslatorArray.
	/// </summary>
	public class TranslatorItem : IComparable
	{
		private StringItem translated=null;
		private StringItem original=null;
		private TranslationState state=TranslationState.Unknown;

		#region Constructors

		public TranslatorItem(StringItem translated, StringItem original, TranslationState state)
		{
			this.translated=translated;
			this.original=original;
			this.state=state;
		}

		#endregion

		#region Properties

		/// <summary>
		/// Returns a StringItem with the translated text
		/// </summary>
		public StringItem StringTranslated
		{
			get { return translated; }
		}

		/// <summary>
		/// Returns a StringItem with the original text
		/// </summary>
		public StringItem StringOriginal
		{
			get { return original; }
		}

		/// <summary>
		/// Gets/Sets the state of the item
		/// </summary>
		public TranslationState State
		{
			get { return state; }
			set { state=value; }
		}

		#endregion

		#region IComparable Members

		public int CompareTo(object obj)
		{
			TranslatorItem right=(TranslatorItem)obj;
			return Convert.ToInt32(StringTranslated.Id)-Convert.ToInt32(right.StringTranslated.Id);
		}

		#endregion
	}
}
