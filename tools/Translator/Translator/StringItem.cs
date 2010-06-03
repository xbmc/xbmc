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

			// Replace \n and \r with the visible newline char ¶ to make it editable
			if (Text.IndexOf("\r\n")>-1)
				text=text.Replace("\r\n", "¶");
			else if (Text.IndexOf("\n")>-1)
				text=text.Replace("\n", "¶");
			else if (Text.IndexOf("\r")>-1)
				text=text.Replace("\r", "¶");
		}

		/// <summary>
		/// Save a single StringItem to a XmlElement
		/// </summary>
		public void SaveToXml(ref XmlElement element)
		{
			element.SetAttribute("id", Id.ToString());
			element.InnerText=Text;

			// Replace the visible newline char ¶ with \n for saving
			if (element.InnerText.IndexOf("¶")>-1)
				element.InnerText=element.InnerText.Replace("¶", "\n");
		}

		#endregion
	}
}
