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
			public string dateLong="DDDD, D MMMM YYYY";
			public string dateShort="DD/MM/YYYY";
			public string time="H:mm:ss";
			public string symbolAM="";
			public string symbolPM="";
			public string speedUnit="kmh";
			public string tempUnit="C";
			public string name="(default)";

			public Region()
			{

			}

			public void Load(XmlElement element)
			{
				if (element.GetAttribute("name")!="")
					name=element.GetAttribute("name");
				dateShort=element.SelectSingleNode("dateshort").InnerText;
				dateLong=element.SelectSingleNode("datelong").InnerText;
				XmlElement timeElement=(XmlElement)element.SelectSingleNode("time");
				symbolAM=timeElement.GetAttribute("symbolAM");
				symbolPM=timeElement.GetAttribute("symbolPM");
				time=timeElement.InnerText;
				speedUnit=element.SelectSingleNode("speedunit").InnerText;
				tempUnit=element.SelectSingleNode("tempunit").InnerText;
			}

			public void Save(ref XmlDocument doc, ref XmlElement element)
			{
				if (name!="(default)")
					element.SetAttribute("name", name);
				XmlElement dateShortElement=doc.CreateElement("dateshort");
				dateShortElement.InnerText=dateShort;
				element.AppendChild(dateShortElement);
				
				XmlElement dateLongElement=doc.CreateElement("datelong");
				dateLongElement.InnerText=dateLong;
				element.AppendChild(dateLongElement);

				XmlElement timeElement=doc.CreateElement("time");
				timeElement.InnerText=time;
				timeElement.SetAttribute("symbolAM", symbolAM);
				timeElement.SetAttribute("symbolPM", symbolPM);
				element.AppendChild(timeElement);

				XmlElement tempUnitElement=doc.CreateElement("tempunit");
				tempUnitElement.InnerText=tempUnit;
				element.AppendChild(tempUnitElement);

				XmlElement speedUnitElement=doc.CreateElement("speedunit");
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
			//
			// TODO: Add constructor logic here
			//
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

				if (regionNodes!=null)
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
					region.Save(ref doc, ref regionNode);
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
	}
}
