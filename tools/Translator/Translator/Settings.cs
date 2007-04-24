using System;
using System.IO;
using Microsoft.Win32;

namespace TeamXBMC.TranslatorCore
{
	/// <summary>
	/// Application wide settings
	/// </summary>
	public sealed class Settings
	{
		private string languageFolder;
		private string language;
		private const string languageOriginal="English";
		private string name="";
		private string email="";
		private static Settings instance=null;

		#region Constructor

		private Settings()
		{

		}

		#endregion

		#region Registry

		/// <summary>
		/// Loads settings from the registry
		/// </summary>
		private void ReadSettings()
		{
			RegistryKey keyUser=Registry.CurrentUser;
			RegistryKey keyTranslator=keyUser.CreateSubKey(@"SOFTWARE\Team XBMC\Translator");
			languageFolder=(string)keyTranslator.GetValue("LanguageFolder");
			language=(string)keyTranslator.GetValue("Language");
			name=(string)keyTranslator.GetValue("Name");
			email=(string)keyTranslator.GetValue("Email");
			
			if (!Directory.Exists(languageFolder) ||
					!File.Exists(FilenameOriginal))
			{
				languageFolder="";
				language="";
			}

		}

		/// <summary>
		/// Saves settings to the registry
		/// </summary>
		private void WriteSettings()
		{
			RegistryKey keyUser=Registry.CurrentUser;
			RegistryKey keyTranslator=keyUser.CreateSubKey(@"SOFTWARE\Team XBMC\Translator");
			if (languageFolder!=null) keyTranslator.SetValue("LanguageFolder", languageFolder);
			if (language!=null) keyTranslator.SetValue("Language", language);
			if (name!=null) keyTranslator.SetValue("Name", name);
			if (email!=null) keyTranslator.SetValue("Email", email);
		}
		#endregion

		#region Properties

		/// <summary>
		/// Returns the instance of the settings object
		/// </summary>
		public static Settings Instance
		{
			get
			{
				if (instance==null)
				{
					instance=new Settings();
					instance.ReadSettings();
				}

				return instance;
			}
		}

		/// <summary>
		/// Gets/Sets the language folder
		/// </summary>
		public string LanguageFolder
		{
			get { return languageFolder; }
			set 
			{
				languageFolder=value;
				WriteSettings();
			}
		}

		/// <summary>
		/// Gets/Sets the language currently edited
		/// </summary>
		public string Language
		{
			get { return language; }
			set 
			{
				language=value;
				WriteSettings();
			}
		}

		/// <summary>
		/// Gets the language uses a master
		/// </summary>
		public string LanguageOriginal
		{
			get { return languageOriginal; }
		}

		/// <summary>
		/// Gets the filename of the active language
		/// </summary>
		public string FilenameTranslated
		{
			get { return LanguageFolder+@"\"+Language+@"\strings.xml"; }
		}

		/// <summary>
		/// Gets the filename of the master language
		/// </summary>
		public string FilenameOriginal
		{
			get { return LanguageFolder+@"\"+LanguageOriginal+@"\strings.xml"; }
		}

		/// <summary>
		/// Gets the filename of the langinfo of the active language
		/// </summary>
		public string FilenameLanguageInfo
		{
			get { return LanguageFolder+@"\"+Language+@"\langinfo.xml"; }
		}

		/// <summary>
		/// Gets/Sets the name of the translator
		/// </summary>
		public string TranslatorName
		{
			get {  return name==null ? "" : name; }
			set
			{ 
				name=value;
				WriteSettings();
			}
		}

		/// <summary>
		/// Gets/Sets the email adress of the translator
		/// </summary>
		public string TranslatorEmail
		{
			get { return email==null ? "" : email; }
			set
			{
				email=value;
				WriteSettings();
			}
		}

		#endregion
	}
}
