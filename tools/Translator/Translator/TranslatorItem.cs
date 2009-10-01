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
