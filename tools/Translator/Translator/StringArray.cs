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
using System.Collections;

namespace TeamXBMC.TranslatorCore
{
	/// <summary>
	/// Represents a xml language file of XBMC
	/// </summary>
	/// 
	public class StringArray : ICollection 
	{
		private ArrayList strings=new ArrayList();
		private Hashtable stringsMap=new Hashtable();
		private double version=0.0;

		#region Constructors

		public StringArray()
		{
		}

		#endregion

		#region Language file procressing

		/// <summary>
		/// Loads a language file by filename
		/// </summary>
		public void Load(string filename)
		{
			XmlTextReader reader=null;

			try
			{
				reader=new XmlTextReader(filename);

				XmlDocument doc=new XmlDocument();
				doc.Load(reader);

				foreach (XmlNode node in doc)
				{
					if (node.NodeType==XmlNodeType.Comment && node.InnerText.IndexOf("$Revision")>-1)
					{
						string comment=node.InnerText;
						int startPos=comment.IndexOf("$Revision")+10;
						int endPos=comment.IndexOf("$", startPos);

						if (endPos>startPos)
						{
							string str=comment.Substring(startPos, endPos-startPos);
							version=Convert.ToDouble(str, System.Globalization.CultureInfo.InvariantCulture);
						}
						break;
					}
				}

				XmlNode root=doc.DocumentElement.SelectSingleNode("/strings");
				if (root==null)
					throw new TranslatorException(filename+" is not a language file.");

				XmlNodeList list=doc.DocumentElement.SelectNodes("/strings/string");

				if (list.Count>0)
				{
					foreach (XmlNode node in list)
					{
						StringItem item=new StringItem();
						item.LoadFromXml((XmlElement)node);

						Add(item);
					}

					strings.Sort();
				}
			}
			catch(Exception e)
			{
				throw new TranslatorException("Error loading "+filename, e);
			}
			finally
			{
				if (reader!=null)
					reader.Close();
			}
		}

		/// <summary>
		/// Saves a language file by filename
		/// </summary>
		public bool Save(string filename)
		{
			return Save(filename, null);
		}

		/// <summary>
		/// Saves a language file by filename, with additional comments
		/// </summary>
		public bool Save(string filename, string[] comments)
		{
			XmlTextWriter writer=null;

			try
			{
				XmlDocument doc=new XmlDocument();

				doc.AppendChild(doc.CreateXmlDeclaration("1.0", "utf-8", "yes"));

				if (comments!=null)
				{
					foreach(string comment in comments)
					{
						doc.AppendChild(doc.CreateComment(comment));
					}
				}

				XmlElement root=doc.CreateElement("strings");
				doc.AppendChild(root);

				foreach (StringItem item in this)
				{
					XmlElement element=doc.CreateElement("string");
					root.AppendChild(element);
					item.SaveToXml(ref element);
				}

				writer=new XmlTextWriter(filename, System.Text.Encoding.UTF8);

				writer.Formatting=Formatting.Indented;

				doc.Save(writer);
			}
			catch(Exception e)
			{
				throw new TranslatorException("Error saving "+filename, e);
			}
			finally
			{
				if (writer!=null)
					writer.Close();
			}

			return true;
		}

		#endregion

		#region Array methods
    
		/// <summary>
		/// Add a StringItem to the StringArray
		/// </summary>
		public void Add(StringItem item)
		{
			try
			{
				stringsMap.Add(item.Id, item);
			}
			catch (ArgumentException e)
			{
				throw new TranslatorException("The string with the id " + item.Id.ToString() + " appears more then once in this file.", e);
			}

			strings.Add(item);

		}

		/// <summary>
		/// Remove all StringItems from the StringArray
		/// </summary>
		public void Clear()
		{
			strings.Clear();
			stringsMap.Clear();
		}

		/// <summary>
		/// Sort the StringArray by StringItem id
		/// </summary>
		public void Sort()
		{
			strings.Sort();
		}

		/// <summary>
		/// Returns a string with a certain id
		/// </summary>
		public bool GetStringById(long id, ref StringItem item)
		{
			if (!stringsMap.Contains(id))
			{
				item=null;
				return false;
			}

			item=(StringItem)stringsMap[id];

			return true;
		}

		/// <summary>
		/// Returns a StringArray with all string not in
		/// arr.
		/// </summary>
		public StringArray GetStringsNotIn(StringArray arr)
		{
			StringArray ret=new StringArray();

			foreach (StringItem item in strings)
			{
				if (item.Id!=6) // "XBMC SVN"
				{
					StringItem stringItem=null;
					arr.GetStringById(item.Id, ref stringItem);
					if (stringItem==null)
						ret.Add(item);
				}
			}

			return ret;
		}

		/// <summary>
		/// Returns the ids of all changed strings
		/// of arr.
		/// </summary>
		public long[] GetStringsChangedIn(StringArray arr)
		{
			ArrayList ret=new ArrayList();

			foreach (StringItem item in strings)
			{
				if (item.Id!=6) // "XBMC SVN"
				{
					StringItem stringItem=null;
					arr.GetStringById(item.Id, ref stringItem);
					if (stringItem!=null && stringItem.Text!=item.Text)
						ret.Add(item.Id);
				}
			}

			return (long[])ret.ToArray(typeof(long));
		}

		#endregion

		#region Properties

		/// <summary>
		/// Returns the version of the strings file
		/// </summary>
		public double Version
		{
			get { return version; }
		}

		#endregion

		#region ICollection Members

		public void CopyTo(Array array, int index)
		{
			strings.CopyTo(array, index);
		}

		public int Count
		{
			get{return strings.Count;}
		}

		public object SyncRoot
		{
			get{return this;}
		}

		public bool IsSynchronized
		{
			get{return false;}
		}

		#endregion

		#region IEnumerable Members

		public IEnumerator GetEnumerator()
		{
			return strings.GetEnumerator();
		}

		#endregion
	}
}
