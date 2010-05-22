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
using System.Collections;

namespace TeamXBMC.TranslatorCore
{
	/// <summary>
	/// Available enumerators for the TranslatorArray class.
	/// </summary>
	public enum TranslatorArrayEnumerator
	{
		All=0,
		Translated,
		Untranslated,
		Changed
	};

	/// <summary>
	/// Enumerates a TranslatorArray class by items with the state translated.
	/// </summary>
	public class EnumeratorTranslated : IEnumerator
	{
		private IEnumerator enumerator=null;

		public EnumeratorTranslated(IEnumerator enumerator)
		{
			this.enumerator=enumerator;
		}

		#region IEnumerator Members

		public void Reset()
		{
			enumerator.Reset();
			MoveNext();
		}

		public object Current
		{
			get
			{
				return enumerator.Current;
			}
		}

		public bool MoveNext()
		{
			if (!enumerator.MoveNext())
				return false;

			TranslatorItem item=(TranslatorItem)enumerator.Current;
			while (item.State!=TranslationState.Translated)
			{
				if (!enumerator.MoveNext())
					return false;
				item=(TranslatorItem)enumerator.Current;
			}

			return true;
		}

		#endregion
	};

	/// <summary>
	/// Enumerates a TranslatorArray class by items with the state untranslated.
	/// </summary>
	public class EnumeratorUntranslated : IEnumerator
	{
		private IEnumerator enumerator=null;

		public EnumeratorUntranslated(IEnumerator enumerator)
		{
			this.enumerator=enumerator;
		}

		#region IEnumerator Members

		public void Reset()
		{
			enumerator.Reset();
			MoveNext();
		}

		public object Current
		{
			get
			{
				return enumerator.Current;
			}
		}

		public bool MoveNext()
		{
			if (!enumerator.MoveNext())
				return false;

			TranslatorItem item=(TranslatorItem)enumerator.Current;
			while (item.State!=TranslationState.Untranslated)
			{
				if (!enumerator.MoveNext())
					return false;

				item=(TranslatorItem)enumerator.Current;
			}

			return true;
		}

		#endregion
	};

	/// <summary>
	/// Enumerates a TranslatorArray class by items with the state changed.
	/// </summary>
	public class EnumeratorChanged : IEnumerator
	{
		private IEnumerator enumerator=null;

		public EnumeratorChanged(IEnumerator enumerator)
		{
			this.enumerator=enumerator;
		}

		#region IEnumerator Members

		public void Reset()
		{
			enumerator.Reset();
			MoveNext();
		}

		public object Current
		{
			get
			{
				return enumerator.Current;
			}
		}

		public bool MoveNext()
		{
			if (!enumerator.MoveNext())
				return false;

			TranslatorItem item=(TranslatorItem)enumerator.Current;
			while (item.State!=TranslationState.Changed)
			{
				if (!enumerator.MoveNext())
					return false;

				item=(TranslatorItem)enumerator.Current;
			}

			return true;
		}

		#endregion
	};
}
