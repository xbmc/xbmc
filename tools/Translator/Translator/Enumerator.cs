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
