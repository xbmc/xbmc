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
	/// Holds language specific settings like date/time format or codepages.
	/// </summary>
	public class LanguageInfo
	{

		public class Region
		{
			private string dateLong="DDDD, D MMMM YYYY";
			private string dateShort="DD/MM/YYYY";
			private string time="H:mm:ss";
			private string symbolAM="";
			private string symbolPM="";
			private string speedUnit="kmh";
			private string tempUnit="C";
			private string name="(default)";

			public Region()
			{

			}

			public string DateLong
			{
				get { return dateLong; }
				set { dateLong=value; }
			}

			public string DateShort
			{
				get { return dateShort; }
				set { dateShort=value; }
			}

			public string Time
			{
				get { return time; }
				set { time=value; }
			}

			public string SymbolAM
			{
				get { return symbolAM; }
				set { symbolAM=value; }
			}

			public string SymbolPM
			{
				get { return symbolPM; }
				set { symbolPM=value; }
			}

			public string SpeedUnit
			{
				get { return speedUnit; }
				set { speedUnit=value; }
			}

			public string TempUnit
			{
				get { return tempUnit; }
				set { tempUnit=value; }
			}

			public string Name
			{
				get { return name; }
				set { name=value; }
			}

			public void Load(XmlElement element)
			{
				if (element.GetAttribute("name")!="")
					name=element.GetAttribute("name");

				XmlNode node=null;

				node=element.SelectSingleNode("dateshort");
				if (node!=null)
					dateShort=node.InnerText;

				node=element.SelectSingleNode("datelong");
				if (node!=null)
					dateLong=node.InnerText;

				XmlElement timeElement=(XmlElement)element.SelectSingleNode("time");
				if (timeElement!=null)
				{
					symbolAM=timeElement.GetAttribute("symbolAM");
					symbolPM=timeElement.GetAttribute("symbolPM");
					time=timeElement.InnerText;
				}

				node=element.SelectSingleNode("speedunit");
				if (node!=null)
					speedUnit=node.InnerText;

				node=element.SelectSingleNode("tempunit");
				if (node!=null)
					tempUnit=node.InnerText;
			}

			public void Save(ref XmlElement element)
			{
				if (name!="(default)")
					element.SetAttribute("name", name);
				XmlElement dateShortElement=element.OwnerDocument.CreateElement("dateshort");
				dateShortElement.InnerText=dateShort;
				element.AppendChild(dateShortElement);
				
				XmlElement dateLongElement=element.OwnerDocument.CreateElement("datelong");
				dateLongElement.InnerText=dateLong;
				element.AppendChild(dateLongElement);

				XmlElement timeElement=element.OwnerDocument.CreateElement("time");
				timeElement.InnerText=time;
				timeElement.SetAttribute("symbolAM", symbolAM);
				timeElement.SetAttribute("symbolPM", symbolPM);
				element.AppendChild(timeElement);

				XmlElement tempUnitElement=element.OwnerDocument.CreateElement("tempunit");
				tempUnitElement.InnerText=tempUnit;
				element.AppendChild(tempUnitElement);

				XmlElement speedUnitElement=element.OwnerDocument.CreateElement("speedunit");
				speedUnitElement.InnerText=speedUnit;
				element.AppendChild(speedUnitElement);

			}
		};

		private string dvdMenu="en";
		private string dvdAudio="en";
		private string dvdSubtitle="en";
		private string charsetGui="CP1251";
		private string charsetSubtitle="CP1251";
		private bool forceUnicodeFont=false;
		private ArrayList regions=new ArrayList();

		public LanguageInfo()
		{
			// Add a region with default values
			regions.Add(new Region()); 
		}

		/// <summary>
		/// Load a language info from filename
		/// </summary>
		public void Load(string filename)
		{
			XmlTextReader reader=null;

			try
			{
				reader=new XmlTextReader(filename);

				XmlDocument doc=new XmlDocument();
				doc.Load(reader);

				XmlNode node=null;

				node=doc.DocumentElement.SelectSingleNode("/language/charsets/gui");
				if (node!=null)
				{
					XmlElement gui=(XmlElement)node;
					charsetGui=gui.InnerText;
					forceUnicodeFont=(gui.GetAttribute("unicodefont")=="true");
				}
				
				node=doc.DocumentElement.SelectSingleNode("/language/charsets/subtitle");
				if (node!=null)
				{
					charsetSubtitle=node.InnerText;
				}


				node=doc.DocumentElement.SelectSingleNode("/language/dvd/menu");
				if (node!=null)
				{
					dvdMenu=node.InnerText;
				}

				node=doc.DocumentElement.SelectSingleNode("/language/dvd/audio");
				if (node!=null)
				{
					dvdAudio=node.InnerText;
				}

				node=doc.DocumentElement.SelectSingleNode("/language/dvd/subtitle");
				if (node!=null)
				{
					dvdSubtitle=node.InnerText;
				}

				XmlNodeList regionNodes=doc.DocumentElement.SelectNodes("/language/regions/region");

				if (regionNodes.Count>0)
				{
					regions.Clear(); // remove default region

					foreach (XmlNode regionNode in regionNodes)
					{
						Region region=new Region();
						region.Load((XmlElement)regionNode);
						regions.Add(region);
					}
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
		/// Save the language info to filename
		/// </summary>
		public void Save(string filename)
		{
			XmlTextWriter writer=null;

			try
			{
				XmlDocument doc=new XmlDocument();

				doc.AppendChild(doc.CreateXmlDeclaration("1.0", "utf-8", "yes"));

				XmlElement root=doc.CreateElement("language");
				doc.AppendChild(root);

				XmlNode charsetsNode=doc.CreateElement("charsets");
				root.AppendChild(charsetsNode);

				XmlElement guiElement=doc.CreateElement("gui");
				guiElement.InnerText=charsetGui;
				guiElement.SetAttribute("unicodefont", forceUnicodeFont ? "true" : "false");
				charsetsNode.AppendChild(guiElement);

				XmlNode subtitleNode=doc.CreateElement("subtitle");
				subtitleNode.InnerText=charsetSubtitle;
				charsetsNode.AppendChild(subtitleNode);

				XmlNode dvdNode=doc.CreateElement("dvd");
				root.AppendChild(dvdNode);

				XmlNode dvdMenuNode=doc.CreateElement("menu");
				dvdMenuNode.InnerText=dvdMenu;
				dvdNode.AppendChild(dvdMenuNode);

				XmlNode dvdAudioNode=doc.CreateElement("audio");
				dvdAudioNode.InnerText=dvdAudio;
				dvdNode.AppendChild(dvdAudioNode);

				XmlNode dvdSubtitleNode=doc.CreateElement("subtitle");
				dvdSubtitleNode.InnerText=dvdSubtitle;
				dvdNode.AppendChild(dvdSubtitleNode);

				XmlNode regionsNode=doc.CreateElement("regions");
				root.AppendChild(regionsNode);

				foreach (Region region in regions)
				{
					XmlElement regionNode=doc.CreateElement("region");
					regionsNode.AppendChild(regionNode);
					region.Save(ref regionNode);
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
		}

		/// <summary>
		/// Gets/Sets the dvd menu language as ISO-639:1988 language code
		/// </summary>
		public string DvdMenu
		{
			get { return dvdMenu; }
			set { dvdMenu=value; }
		}

		/// <summary>
		/// Gets/Sets the dvd audio language as ISO-639:1988 language code
		/// </summary>
		public string DvdAudio
		{
			get { return dvdAudio; }
			set { dvdAudio=value; }
		}

		/// <summary>
		/// Gets/Sets the dvd subtitle language as ISO-639:1988 language code
		/// </summary>
		public string DvdSubtitle
		{
			get { return dvdSubtitle; }
			set { dvdSubtitle=value; }
		}

		/// <summary>
		/// Gets/Sets the charset used for the user interface of xbmc
		/// </summary>
		public string CharsetGui
		{
			get { return charsetGui; }
			set { charsetGui=value; }
		}

		/// <summary>
		/// Gets/Sets the default charset used for the subtitles in xbmc
		/// </summary>
		public string CharsetSubtitle
		{
			get { return charsetSubtitle; }
			set { charsetSubtitle=value; }
		}

		/// <summary>
		/// Gets/Sets if the language need a unicode font to display properly
		/// </summary>
		public bool ForceUnicodeFont
		{
			get { return forceUnicodeFont; }
			set { forceUnicodeFont=value; }
		}

		/// <summary>
		/// Returns the available region, a region contains
		/// things like date/time format or speed and temp units
		/// </summary>
		public Region[] Regions
		{
			get { return (Region[])regions.ToArray(typeof(Region)); }
		}

		/// <summary>
		/// Removes a region from the language info by name
		/// </summary>
		public void RemoveRegion(string regionName)
		{
			Region toDelete=null;
			foreach (Region region in regions)
			{
				if (region.Name==regionName)
				{
					// Region to remove is found
					toDelete=region;
					break;
				}
			}

			// remove it
			if (toDelete!=null)
				regions.Remove(toDelete);
		}

		/// <summary>
		/// Adds a region to the language info.
		/// The name of the new region is returned.
		/// </summary>
		public string AddRegion()
		{
			int i=1;
			string newName="";
			bool found=false;
			while (!found)
			{
				// Try next name
				newName="Region"+i.ToString();
				i++;
				// Does the new region name already exist
				bool exists=false;
				foreach (Region region in regions)
				{
					if (region.Name==newName)
					{
						exists=true;
						break;
					}
				}

				if (!exists)
				{ // none existing name found 
					// add a region with this name
					found=true;
					Region region=new Region();
					region.Name=newName;
					regions.Add(region);
					break;
				}
			}

			return newName;
		}
	}
}
