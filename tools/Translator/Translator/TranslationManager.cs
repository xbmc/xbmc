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
using System.IO;
using System.Collections;

namespace TeamXBMC.TranslatorCore
{
	/// <summary>
	/// Object to communicate with the gui.
	/// </summary>
	public sealed class TranslationManager
	{
		private TranslatorArray strings = new TranslatorArray();
		private static TranslationManager instance=null;

		private TranslationManager()
		{

		}

		/// <summary>
		/// Loads the current language
		/// </summary>
		public void Initialize()
		{
			strings.Clear();
			strings.Load();
		}

		#region Language file processing

		/// <summary>
		/// Create a new language
		/// </summary>
		public void CreateLanguage(string name)
		{
			if (File.Exists(Settings.Instance.LanguageFolder+@"\"+name+@"\strings.xml"))
				throw new TranslatorException("The language \""+name+"\" already exists.");

			try
			{
				Directory.CreateDirectory(Settings.Instance.LanguageFolder+@"\"+name);
			}
			catch (Exception e)
			{
				throw new TranslatorException("Unable to create the directory "+Settings.Instance.LanguageFolder+@"\"+name, e);
			}


			// Save an empty strings.xml file
			StringArray stringsNew=new StringArray();
			stringsNew.Save(Settings.Instance.LanguageFolder+@"\"+name+@"\strings.xml");

			LanguageInfo langinfo=new LanguageInfo();
			langinfo.Save(Settings.Instance.LanguageFolder+@"\"+name+@"\langinfo.xml");
		}

		/// <summary>
		/// Save the current language
		/// </summary>
		public void SaveTranslated()
		{
			strings.Save();
		}

		#endregion

		#region Properties

		/// <summary>
		/// Gets the instance of the TranslationManager
		/// </summary>
		public static TranslationManager Instance
		{
			get
			{
				if (instance==null)
				{
					instance=new TranslationManager();
				}

				return instance;
			}
		}

		/// <summary>
		/// Get the strings of the current language with the state translated
		/// </summary>
		public TranslatorArray Translated
		{
			get 
			{
				strings.Enumerator=TranslatorArrayEnumerator.Translated;
				return strings;
			}
		}

		/// <summary>
		/// Get the strings of the current language with the state untranslated
		/// </summary>
		public TranslatorArray Untranslated
		{
			get
			{
				strings.Enumerator=TranslatorArrayEnumerator.Untranslated;
				return strings;
			}
		}

		/// <summary>
		/// Get the strings of the current language with the state changed
		/// </summary>
		public TranslatorArray Changed
		{
			get
			{
				strings.Enumerator=TranslatorArrayEnumerator.Changed;
				return strings;
			}
		}

		/// <summary>
		/// Get all strings of the current language
		/// </summary>
		public TranslatorArray All
		{
			get
			{
				strings.Enumerator=TranslatorArrayEnumerator.All;
				return strings;
			}
		}

		/// <summary>
		/// Get the name of the current language
		/// </summary>
		public string LanguageTranslated
		{
			get { return Settings.Instance.Language; }
		}

		/// <summary>
		/// Get the name of the master language
		/// </summary>
		public string LanguageOriginal
		{
			get { return Settings.Instance.LanguageOriginal; }
		}

		/// <summary>
		/// Get all available languages
		/// </summary>
		public string[] Languages
		{
			get
			{
				string root=Settings.Instance.LanguageFolder;
				string[] dirs=Directory.GetDirectories(root);
				ArrayList languages=new ArrayList();

				foreach (string dir in dirs)
				{
					if (File.Exists(dir+@"\strings.xml"))
					{
						// Extract language name from path
						string language=dir.Substring(root.Length+1, dir.Length-root.Length-1);
						if (!language.Equals(Settings.Instance.LanguageOriginal) )
							languages.Add(language);
					}
				}

				return (string[])languages.ToArray(typeof(string));
			}
		}

		/// <summary>
		/// Returns true if the currently edited language file is has been changed by the user
		/// </summary>
		public bool IsModified
		{
			get { return strings.IsModified; }
		}

		#endregion
	}
}
