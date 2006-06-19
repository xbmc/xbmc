using System;
using System.Xml;

namespace TeamXBMC.TranslatorCore
{
	/// <summary>
	/// Represent a single string of a language file
	/// </summary>
	public class StringItem : IComparable
	{
		/// <summary>
		/// The delegate is called to notify that a StringItem has changed
		/// </summary>
		public delegate void StringUpdatedDelegate(StringItem item);

		private long id=0;
		private string text="";
		public event StringUpdatedDelegate stringUpdated=null;

		#region Constructors
		
		public StringItem()
		{

		}

		public StringItem(StringItem right)
		{
			id=right.id;
			Text=right.Text;
		}

		#endregion

		#region Properties

		/// <summary>
		/// Get the id of the StingItem
		/// </summary>
		public long Id
		{
			get { return id; }
		}

		/// <summary>
		/// Get the value of the StingItem
		/// </summary>
		public string Text
		{
			get { return text; }
			set
			{
				text=value;
				if (stringUpdated!=null)
				{
					stringUpdated(this);
				}
			}
		}

		#endregion

		#region IComparable Members

		public int CompareTo(object obj)
		{
			StringItem right=(StringItem)obj;
			return Convert.ToInt32(Id)-Convert.ToInt32(right.Id);
		}

		#endregion

		#region Xml Io

		/// <summary>
		/// Load a single StringItem from a XmlElement
		/// </summary>
		public void LoadFromXml(XmlElement element)
		{
			if (element.Attributes.Count==1)
			{ // new language file layout with id as attribute
				id=Convert.ToInt32(element.GetAttribute("id"));
				text=element.InnerText;
			}
			else
			{ // old language file layout with nodes for ids and values
				id=Convert.ToInt32(element.SelectSingleNode("id").InnerText);
				text=element.SelectSingleNode("value").InnerText;
			}
		}

		/// <summary>
		/// Save a single StringItem to a XmlElement
		/// </summary>
		public void SaveToXml(ref XmlElement element)
		{
			element.SetAttribute("id", Id.ToString());
			element.InnerText=Text;
		}

		#endregion
	}
}
