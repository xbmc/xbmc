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
