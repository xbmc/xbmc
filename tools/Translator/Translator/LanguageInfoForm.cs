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
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Globalization;
using System.IO;
using TeamXBMC.TranslatorCore;

namespace TeamXBMC.Translator
{
	/// <summary>
	/// Summary description for LangInfoForm.
	/// </summary>
	public class LanguageInfoForm : System.Windows.Forms.Form
	{
		private LanguageInfo languageInfo=new LanguageInfo();
		private System.Windows.Forms.Button buttonOK;
		private System.Windows.Forms.TabPage tabPage1;
		private System.Windows.Forms.TabPage tabPage2;
		private System.Windows.Forms.TabPage tabPage3;
		private System.Windows.Forms.Button buttonRemove;
		private System.Windows.Forms.Button buttonAdd;
		private System.Windows.Forms.TabControl tabControl1;
		private System.Windows.Forms.Button buttonCancel;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.GroupBox groupBox3;
		private System.Windows.Forms.GroupBox groupBox4;
		private System.Windows.Forms.GroupBox groupBox5;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.Label label4;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.Label label6;
		private System.Windows.Forms.ComboBox comboBoxSubtitle;
		private System.Windows.Forms.ComboBox comboBoxGUI;
		private System.Windows.Forms.ComboBox comboBoxDvdSubtitle;
		private System.Windows.Forms.ComboBox comboBoxDvdAudio;
		private System.Windows.Forms.ComboBox comboBoxDvdMenu;
		private System.Windows.Forms.Label label7;
		private System.Windows.Forms.Button buttonCharsetSystemDefault;
		private System.Windows.Forms.Label label8;
		private System.Windows.Forms.Button buttonDvdDefault;
		private System.Windows.Forms.Button buttonProperties;
		private System.Windows.Forms.GroupBox groupBox6;
		private System.Windows.Forms.Label label9;
		private System.Windows.Forms.Label label10;
		private System.Windows.Forms.CheckBox checkBoxUnicodeFont;
		private System.Windows.Forms.ColumnHeader columnHeaderRegion;
		private System.Windows.Forms.ListView listViewRegions;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public LanguageInfoForm()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			//
			// TODO: Add any constructor code after InitializeComponent call
			//
		}

		/// <summary>
		/// Clean up any resources being used.
		/// </summary>
		protected override void Dispose( bool disposing )
		{
			if( disposing )
			{
				if(components != null)
				{
					components.Dispose();
				}
			}
			base.Dispose( disposing );
		}

		#region Windows Form Designer generated code
		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		private void InitializeComponent()
		{
			this.buttonOK = new System.Windows.Forms.Button();
			this.tabPage1 = new System.Windows.Forms.TabPage();
			this.groupBox6 = new System.Windows.Forms.GroupBox();
			this.checkBoxUnicodeFont = new System.Windows.Forms.CheckBox();
			this.label10 = new System.Windows.Forms.Label();
			this.label7 = new System.Windows.Forms.Label();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.comboBoxSubtitle = new System.Windows.Forms.ComboBox();
			this.label3 = new System.Windows.Forms.Label();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.comboBoxGUI = new System.Windows.Forms.ComboBox();
			this.label2 = new System.Windows.Forms.Label();
			this.label1 = new System.Windows.Forms.Label();
			this.buttonCharsetSystemDefault = new System.Windows.Forms.Button();
			this.tabPage2 = new System.Windows.Forms.TabPage();
			this.label8 = new System.Windows.Forms.Label();
			this.buttonDvdDefault = new System.Windows.Forms.Button();
			this.groupBox5 = new System.Windows.Forms.GroupBox();
			this.label6 = new System.Windows.Forms.Label();
			this.comboBoxDvdSubtitle = new System.Windows.Forms.ComboBox();
			this.groupBox4 = new System.Windows.Forms.GroupBox();
			this.label5 = new System.Windows.Forms.Label();
			this.comboBoxDvdAudio = new System.Windows.Forms.ComboBox();
			this.groupBox3 = new System.Windows.Forms.GroupBox();
			this.label4 = new System.Windows.Forms.Label();
			this.comboBoxDvdMenu = new System.Windows.Forms.ComboBox();
			this.tabPage3 = new System.Windows.Forms.TabPage();
			this.listViewRegions = new System.Windows.Forms.ListView();
			this.columnHeaderRegion = new System.Windows.Forms.ColumnHeader();
			this.label9 = new System.Windows.Forms.Label();
			this.buttonProperties = new System.Windows.Forms.Button();
			this.buttonRemove = new System.Windows.Forms.Button();
			this.buttonAdd = new System.Windows.Forms.Button();
			this.tabControl1 = new System.Windows.Forms.TabControl();
			this.buttonCancel = new System.Windows.Forms.Button();
			this.tabPage1.SuspendLayout();
			this.groupBox6.SuspendLayout();
			this.groupBox2.SuspendLayout();
			this.groupBox1.SuspendLayout();
			this.tabPage2.SuspendLayout();
			this.groupBox5.SuspendLayout();
			this.groupBox4.SuspendLayout();
			this.groupBox3.SuspendLayout();
			this.tabPage3.SuspendLayout();
			this.tabControl1.SuspendLayout();
			this.SuspendLayout();
			// 
			// buttonOK
			// 
			this.buttonOK.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.buttonOK.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonOK.Location = new System.Drawing.Point(152, 328);
			this.buttonOK.Name = "buttonOK";
			this.buttonOK.TabIndex = 1;
			this.buttonOK.Text = "OK";
			this.buttonOK.Click += new System.EventHandler(this.buttonOK_Click);
			// 
			// tabPage1
			// 
			this.tabPage1.BackColor = System.Drawing.SystemColors.ControlLightLight;
			this.tabPage1.Controls.Add(this.groupBox6);
			this.tabPage1.Controls.Add(this.label7);
			this.tabPage1.Controls.Add(this.groupBox2);
			this.tabPage1.Controls.Add(this.groupBox1);
			this.tabPage1.Controls.Add(this.label1);
			this.tabPage1.Controls.Add(this.buttonCharsetSystemDefault);
			this.tabPage1.ForeColor = System.Drawing.SystemColors.WindowText;
			this.tabPage1.Location = new System.Drawing.Point(4, 22);
			this.tabPage1.Name = "tabPage1";
			this.tabPage1.Size = new System.Drawing.Size(296, 286);
			this.tabPage1.TabIndex = 0;
			this.tabPage1.Text = "Charsets";
			// 
			// groupBox6
			// 
			this.groupBox6.Controls.Add(this.checkBoxUnicodeFont);
			this.groupBox6.Controls.Add(this.label10);
			this.groupBox6.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.groupBox6.Location = new System.Drawing.Point(8, 168);
			this.groupBox6.Name = "groupBox6";
			this.groupBox6.Size = new System.Drawing.Size(280, 72);
			this.groupBox6.TabIndex = 8;
			this.groupBox6.TabStop = false;
			this.groupBox6.Text = "Font";
			// 
			// checkBoxUnicodeFont
			// 
			this.checkBoxUnicodeFont.BackColor = System.Drawing.SystemColors.ControlLightLight;
			this.checkBoxUnicodeFont.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.checkBoxUnicodeFont.Location = new System.Drawing.Point(120, 48);
			this.checkBoxUnicodeFont.Name = "checkBoxUnicodeFont";
			this.checkBoxUnicodeFont.Size = new System.Drawing.Size(120, 16);
			this.checkBoxUnicodeFont.TabIndex = 10;
			this.checkBoxUnicodeFont.Text = "Force unicode font";
			// 
			// label10
			// 
			this.label10.Location = new System.Drawing.Point(8, 16);
			this.label10.Name = "label10";
			this.label10.Size = new System.Drawing.Size(264, 40);
			this.label10.TabIndex = 9;
			this.label10.Text = "Click if the language needs a unicode font to display properly, eg. the language " +
				"has none latin characters.";
			// 
			// label7
			// 
			this.label7.Location = new System.Drawing.Point(8, 248);
			this.label7.Name = "label7";
			this.label7.Size = new System.Drawing.Size(200, 32);
			this.label7.TabIndex = 11;
			this.label7.Text = "Use this button to set the charsets from the current local windows settings";
			// 
			// groupBox2
			// 
			this.groupBox2.Controls.Add(this.comboBoxSubtitle);
			this.groupBox2.Controls.Add(this.label3);
			this.groupBox2.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.groupBox2.Location = new System.Drawing.Point(8, 96);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(280, 64);
			this.groupBox2.TabIndex = 5;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "Subtitle";
			// 
			// comboBoxSubtitle
			// 
			this.comboBoxSubtitle.ItemHeight = 13;
			this.comboBoxSubtitle.Location = new System.Drawing.Point(88, 32);
			this.comboBoxSubtitle.Name = "comboBoxSubtitle";
			this.comboBoxSubtitle.Size = new System.Drawing.Size(121, 21);
			this.comboBoxSubtitle.Sorted = true;
			this.comboBoxSubtitle.TabIndex = 7;
			// 
			// label3
			// 
			this.label3.Location = new System.Drawing.Point(16, 16);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(224, 23);
			this.label3.TabIndex = 6;
			this.label3.Text = "Select the charset for subtitles";
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.Add(this.comboBoxGUI);
			this.groupBox1.Controls.Add(this.label2);
			this.groupBox1.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.groupBox1.Location = new System.Drawing.Point(8, 24);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(280, 64);
			this.groupBox1.TabIndex = 2;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "User Interface";
			// 
			// comboBoxGUI
			// 
			this.comboBoxGUI.ItemHeight = 13;
			this.comboBoxGUI.Location = new System.Drawing.Point(88, 32);
			this.comboBoxGUI.Name = "comboBoxGUI";
			this.comboBoxGUI.Size = new System.Drawing.Size(120, 21);
			this.comboBoxGUI.Sorted = true;
			this.comboBoxGUI.TabIndex = 4;
			// 
			// label2
			// 
			this.label2.Location = new System.Drawing.Point(16, 16);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(224, 23);
			this.label2.TabIndex = 3;
			this.label2.Text = "Select the charset of the user interface";
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(8, 8);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(264, 23);
			this.label1.TabIndex = 1;
			this.label1.Text = "Select the standard charsets for this language";
			// 
			// buttonCharsetSystemDefault
			// 
			this.buttonCharsetSystemDefault.BackColor = System.Drawing.SystemColors.Control;
			this.buttonCharsetSystemDefault.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonCharsetSystemDefault.Location = new System.Drawing.Point(208, 248);
			this.buttonCharsetSystemDefault.Name = "buttonCharsetSystemDefault";
			this.buttonCharsetSystemDefault.TabIndex = 12;
			this.buttonCharsetSystemDefault.Text = "Default";
			this.buttonCharsetSystemDefault.Click += new System.EventHandler(this.buttonCharsetSystemDefault_Click);
			// 
			// tabPage2
			// 
			this.tabPage2.BackColor = System.Drawing.SystemColors.ControlLightLight;
			this.tabPage2.Controls.Add(this.label8);
			this.tabPage2.Controls.Add(this.buttonDvdDefault);
			this.tabPage2.Controls.Add(this.groupBox5);
			this.tabPage2.Controls.Add(this.groupBox4);
			this.tabPage2.Controls.Add(this.groupBox3);
			this.tabPage2.ForeColor = System.Drawing.SystemColors.WindowText;
			this.tabPage2.Location = new System.Drawing.Point(4, 22);
			this.tabPage2.Name = "tabPage2";
			this.tabPage2.Size = new System.Drawing.Size(296, 286);
			this.tabPage2.TabIndex = 1;
			this.tabPage2.Text = "DVD Language";
			// 
			// label8
			// 
			this.label8.Location = new System.Drawing.Point(8, 248);
			this.label8.Name = "label8";
			this.label8.Size = new System.Drawing.Size(200, 32);
			this.label8.TabIndex = 10;
			this.label8.Text = "Use this button to set the language from the current local windows settings";
			// 
			// buttonDvdDefault
			// 
			this.buttonDvdDefault.BackColor = System.Drawing.SystemColors.Control;
			this.buttonDvdDefault.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonDvdDefault.Location = new System.Drawing.Point(208, 248);
			this.buttonDvdDefault.Name = "buttonDvdDefault";
			this.buttonDvdDefault.TabIndex = 11;
			this.buttonDvdDefault.Text = "Default";
			this.buttonDvdDefault.Click += new System.EventHandler(this.buttonDvdDefault_Click);
			// 
			// groupBox5
			// 
			this.groupBox5.Controls.Add(this.label6);
			this.groupBox5.Controls.Add(this.comboBoxDvdSubtitle);
			this.groupBox5.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.groupBox5.Location = new System.Drawing.Point(8, 168);
			this.groupBox5.Name = "groupBox5";
			this.groupBox5.Size = new System.Drawing.Size(280, 72);
			this.groupBox5.TabIndex = 7;
			this.groupBox5.TabStop = false;
			this.groupBox5.Text = "Subtitle";
			// 
			// label6
			// 
			this.label6.Location = new System.Drawing.Point(16, 32);
			this.label6.Name = "label6";
			this.label6.Size = new System.Drawing.Size(96, 23);
			this.label6.TabIndex = 8;
			this.label6.Text = "Language:";
			// 
			// comboBoxDvdSubtitle
			// 
			this.comboBoxDvdSubtitle.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.comboBoxDvdSubtitle.ItemHeight = 13;
			this.comboBoxDvdSubtitle.Location = new System.Drawing.Point(136, 32);
			this.comboBoxDvdSubtitle.Name = "comboBoxDvdSubtitle";
			this.comboBoxDvdSubtitle.Size = new System.Drawing.Size(121, 21);
			this.comboBoxDvdSubtitle.Sorted = true;
			this.comboBoxDvdSubtitle.TabIndex = 9;
			// 
			// groupBox4
			// 
			this.groupBox4.Controls.Add(this.label5);
			this.groupBox4.Controls.Add(this.comboBoxDvdAudio);
			this.groupBox4.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.groupBox4.ForeColor = System.Drawing.SystemColors.ControlText;
			this.groupBox4.Location = new System.Drawing.Point(8, 88);
			this.groupBox4.Name = "groupBox4";
			this.groupBox4.Size = new System.Drawing.Size(280, 72);
			this.groupBox4.TabIndex = 4;
			this.groupBox4.TabStop = false;
			this.groupBox4.Text = "Audio";
			// 
			// label5
			// 
			this.label5.Location = new System.Drawing.Point(16, 32);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(96, 23);
			this.label5.TabIndex = 5;
			this.label5.Text = "Language:";
			// 
			// comboBoxDvdAudio
			// 
			this.comboBoxDvdAudio.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.comboBoxDvdAudio.ItemHeight = 13;
			this.comboBoxDvdAudio.Location = new System.Drawing.Point(136, 32);
			this.comboBoxDvdAudio.Name = "comboBoxDvdAudio";
			this.comboBoxDvdAudio.Size = new System.Drawing.Size(121, 21);
			this.comboBoxDvdAudio.Sorted = true;
			this.comboBoxDvdAudio.TabIndex = 6;
			// 
			// groupBox3
			// 
			this.groupBox3.Controls.Add(this.label4);
			this.groupBox3.Controls.Add(this.comboBoxDvdMenu);
			this.groupBox3.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.groupBox3.ForeColor = System.Drawing.SystemColors.ControlText;
			this.groupBox3.Location = new System.Drawing.Point(8, 8);
			this.groupBox3.Name = "groupBox3";
			this.groupBox3.Size = new System.Drawing.Size(280, 72);
			this.groupBox3.TabIndex = 1;
			this.groupBox3.TabStop = false;
			this.groupBox3.Text = "Menu";
			// 
			// label4
			// 
			this.label4.Location = new System.Drawing.Point(16, 32);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(96, 23);
			this.label4.TabIndex = 2;
			this.label4.Text = "Language:";
			// 
			// comboBoxDvdMenu
			// 
			this.comboBoxDvdMenu.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.comboBoxDvdMenu.ItemHeight = 13;
			this.comboBoxDvdMenu.Location = new System.Drawing.Point(136, 32);
			this.comboBoxDvdMenu.Name = "comboBoxDvdMenu";
			this.comboBoxDvdMenu.Size = new System.Drawing.Size(121, 21);
			this.comboBoxDvdMenu.Sorted = true;
			this.comboBoxDvdMenu.TabIndex = 3;
			// 
			// tabPage3
			// 
			this.tabPage3.BackColor = System.Drawing.SystemColors.ControlLightLight;
			this.tabPage3.Controls.Add(this.listViewRegions);
			this.tabPage3.Controls.Add(this.label9);
			this.tabPage3.Controls.Add(this.buttonProperties);
			this.tabPage3.Controls.Add(this.buttonRemove);
			this.tabPage3.Controls.Add(this.buttonAdd);
			this.tabPage3.ForeColor = System.Drawing.SystemColors.WindowText;
			this.tabPage3.Location = new System.Drawing.Point(4, 22);
			this.tabPage3.Name = "tabPage3";
			this.tabPage3.Size = new System.Drawing.Size(296, 286);
			this.tabPage3.TabIndex = 2;
			this.tabPage3.Text = "Regions";
			// 
			// listViewRegions
			// 
			this.listViewRegions.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
																																											this.columnHeaderRegion});
			this.listViewRegions.FullRowSelect = true;
			this.listViewRegions.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
			this.listViewRegions.HideSelection = false;
			this.listViewRegions.LabelEdit = true;
			this.listViewRegions.Location = new System.Drawing.Point(24, 64);
			this.listViewRegions.MultiSelect = false;
			this.listViewRegions.Name = "listViewRegions";
			this.listViewRegions.Size = new System.Drawing.Size(160, 184);
			this.listViewRegions.Sorting = System.Windows.Forms.SortOrder.Ascending;
			this.listViewRegions.TabIndex = 1;
			this.listViewRegions.View = System.Windows.Forms.View.Details;
			this.listViewRegions.DoubleClick += new System.EventHandler(this.listViewRegions_DoubleClick);
			this.listViewRegions.AfterLabelEdit += new System.Windows.Forms.LabelEditEventHandler(this.listViewRegions_AfterLabelEdit);
			// 
			// columnHeaderRegion
			// 
			this.columnHeaderRegion.Text = "Region";
			this.columnHeaderRegion.Width = 139;
			// 
			// label9
			// 
			this.label9.Location = new System.Drawing.Point(24, 8);
			this.label9.Name = "label9";
			this.label9.Size = new System.Drawing.Size(248, 48);
			this.label9.TabIndex = 0;
			this.label9.Text = "If the language can be used in more then one location, but date and time format d" +
				"iffers, an addition region can be specified.";
			// 
			// buttonProperties
			// 
			this.buttonProperties.BackColor = System.Drawing.SystemColors.Control;
			this.buttonProperties.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonProperties.Location = new System.Drawing.Point(192, 128);
			this.buttonProperties.Name = "buttonProperties";
			this.buttonProperties.TabIndex = 4;
			this.buttonProperties.Text = "Properties";
			this.buttonProperties.Click += new System.EventHandler(this.buttonProperties_Click);
			// 
			// buttonRemove
			// 
			this.buttonRemove.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonRemove.Location = new System.Drawing.Point(192, 96);
			this.buttonRemove.Name = "buttonRemove";
			this.buttonRemove.TabIndex = 3;
			this.buttonRemove.Text = "Remove";
			this.buttonRemove.Click += new System.EventHandler(this.buttonRemove_Click);
			// 
			// buttonAdd
			// 
			this.buttonAdd.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonAdd.Location = new System.Drawing.Point(192, 64);
			this.buttonAdd.Name = "buttonAdd";
			this.buttonAdd.TabIndex = 2;
			this.buttonAdd.Text = "Add...";
			this.buttonAdd.Click += new System.EventHandler(this.buttonAdd_Click);
			// 
			// tabControl1
			// 
			this.tabControl1.Controls.Add(this.tabPage1);
			this.tabControl1.Controls.Add(this.tabPage2);
			this.tabControl1.Controls.Add(this.tabPage3);
			this.tabControl1.ItemSize = new System.Drawing.Size(53, 18);
			this.tabControl1.Location = new System.Drawing.Point(8, 8);
			this.tabControl1.Name = "tabControl1";
			this.tabControl1.SelectedIndex = 0;
			this.tabControl1.Size = new System.Drawing.Size(304, 312);
			this.tabControl1.TabIndex = 0;
			// 
			// buttonCancel
			// 
			this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.buttonCancel.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonCancel.Location = new System.Drawing.Point(240, 328);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.TabIndex = 2;
			this.buttonCancel.Text = "Cancel";
			// 
			// LanguageInfoForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(322, 360);
			this.Controls.Add(this.tabControl1);
			this.Controls.Add(this.buttonCancel);
			this.Controls.Add(this.buttonOK);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "LanguageInfoForm";
			this.ShowInTaskbar = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Language Settings";
			this.Load += new System.EventHandler(this.LangInfoForm_Load);
			this.tabPage1.ResumeLayout(false);
			this.groupBox6.ResumeLayout(false);
			this.groupBox2.ResumeLayout(false);
			this.groupBox1.ResumeLayout(false);
			this.tabPage2.ResumeLayout(false);
			this.groupBox5.ResumeLayout(false);
			this.groupBox4.ResumeLayout(false);
			this.groupBox3.ResumeLayout(false);
			this.tabPage3.ResumeLayout(false);
			this.tabControl1.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		#region Form initialization
		
		/// <summary>
		/// Fills the values of a langinfo file into the tabpages
		/// </summary>
		private void LangInfoForm_Load(object sender, System.EventArgs e)
		{
			// Fill comboBoxes with all available codepages and iso dvd languages
			CultureInfo[] cultures=CultureInfo.GetCultures(CultureTypes.NeutralCultures);
			foreach (CultureInfo info in cultures)
			{
				if (!comboBoxDvdMenu.Items.Contains(info.TwoLetterISOLanguageName))
				{
					comboBoxDvdMenu.Items.Add(info.TwoLetterISOLanguageName);
					comboBoxDvdAudio.Items.Add(info.TwoLetterISOLanguageName);
					comboBoxDvdSubtitle.Items.Add(info.TwoLetterISOLanguageName);
				}
				if (info.TextInfo.ANSICodePage>0 && !comboBoxGUI.Items.Contains("CP"+info.TextInfo.ANSICodePage))
				{
					comboBoxGUI.Items.Add("CP"+info.TextInfo.ANSICodePage);
					comboBoxSubtitle.Items.Add("CP"+info.TextInfo.ANSICodePage);
				}
			}

			// Load teh langinfo file if it exists
			if (File.Exists(Settings.Instance.FilenameLanguageInfo))
				languageInfo.Load(Settings.Instance.FilenameLanguageInfo);

			// Update comboBoxes with the values from langinfo
			comboBoxGUI.Text=languageInfo.CharsetGui;
			checkBoxUnicodeFont.Checked=languageInfo.ForceUnicodeFont;
			comboBoxSubtitle.Text=languageInfo.CharsetSubtitle;

			comboBoxDvdMenu.Text=languageInfo.DvdMenu;
			comboBoxDvdAudio.Text=languageInfo.DvdAudio;
			comboBoxDvdSubtitle.Text=languageInfo.DvdSubtitle;

			// Fill listbox with region form langinfo
			foreach (LanguageInfo.Region region in languageInfo.Regions)
			{
				listViewRegions.Items.Add(region.Name);
			}

			if (listViewRegions.Items.Count>0)
				listViewRegions.Items[0].Selected=true;
		}

		#endregion

		#region Control events

		private void buttonOK_Click(object sender, System.EventArgs e)
		{
			languageInfo.CharsetGui=comboBoxGUI.Text;
			languageInfo.ForceUnicodeFont=checkBoxUnicodeFont.Checked;
			languageInfo.CharsetSubtitle=comboBoxSubtitle.Text;

			languageInfo.DvdMenu=comboBoxDvdMenu.Text;
			languageInfo.DvdAudio=comboBoxDvdAudio.Text;
			languageInfo.DvdSubtitle=comboBoxDvdSubtitle.Text;

			languageInfo.Save(Settings.Instance.FilenameLanguageInfo);
		}

		/// <summary>
		/// Sets the Windows default codepage for gui and subtitles
		/// </summary>
		private void buttonCharsetSystemDefault_Click(object sender, System.EventArgs e)
		{
			CultureInfo culture=CultureInfo.CurrentCulture;
			comboBoxSubtitle.SelectedItem="CP"+culture.TextInfo.ANSICodePage;
			comboBoxGUI.SelectedItem="CP"+culture.TextInfo.ANSICodePage;
		}

		/// <summary>
		/// Sets the Windows default ISO-639:1988 language code for dvd language
		/// </summary>
		private void buttonDvdDefault_Click(object sender, System.EventArgs e)
		{
			CultureInfo culture=CultureInfo.CurrentCulture;
			comboBoxDvdMenu.SelectedItem=culture.TwoLetterISOLanguageName;
			comboBoxDvdAudio.SelectedItem=culture.TwoLetterISOLanguageName;
			comboBoxDvdSubtitle.SelectedItem=culture.TwoLetterISOLanguageName;
		}

		/// <summary>
		/// Remove a region to the listView
		/// </summary>
		private void buttonAdd_Click(object sender, System.EventArgs e)
		{
			if (listViewRegions.SelectedItems.Count>0)
			{
				string newRegion=languageInfo.AddRegion();
				listViewRegions.Items.Add(newRegion);

				// Find the new item
				foreach (ListViewItem item in listViewRegions.Items)
				{
					if (item.Text==newRegion)
					{ // and select it
						item.Selected=true;
						break;
					}
				}
			}
		}

		/// <summary>
		/// Remove a region from the listView
		/// </summary>
		private void buttonRemove_Click(object sender, System.EventArgs e)
		{
			if (listViewRegions.SelectedItems.Count>0)
			{
				string regionName=listViewRegions.SelectedItems[0].Text;
				languageInfo.RemoveRegion(regionName);

				int lastPos=listViewRegions.SelectedIndices[0];
				listViewRegions.Items.RemoveAt(lastPos);

				if (listViewRegions.Items.Count>0)
				{
					// new selected item out of range?
					if (lastPos>listViewRegions.Items.Count-1)
						lastPos--;

					listViewRegions.Items[lastPos].Selected=true;
				}
			}

			if (listViewRegions.Items.Count==0)
			{	// no regions left, add a default region
				string newRegion=languageInfo.AddRegion();
				listViewRegions.Items.Add(newRegion);
				listViewRegions.Items[0].Selected=true;
			}
		}

		/// <summary>
		/// Shows a form to edit the selected region
		/// </summary>
		private void buttonProperties_Click(object sender, System.EventArgs e)
		{
			foreach (LanguageInfo.Region region in languageInfo.Regions)
			{
				if (region.Name==listViewRegions.SelectedItems[0].Text)
				{
					RegionForm form=new RegionForm();
					form.RegionInfo=region;
					form.ShowDialog();
					return;
				}
			}
		}

		/// <summary>
		/// Shows a form to edit the selected region
		/// </summary>
		private void listViewRegions_DoubleClick(object sender, System.EventArgs e)
		{
			foreach (LanguageInfo.Region region in languageInfo.Regions)
			{
				if (region.Name==listViewRegions.SelectedItems[0].Text)
				{
					RegionForm form=new RegionForm();
					form.RegionInfo=region;
					form.ShowDialog();
					return;
				}
			}
		}

		/// <summary>
		/// Changes the name of a region
		/// </summary>
		private void listViewRegions_AfterLabelEdit(object sender, LabelEditEventArgs e)
		{
			if (e.Label==null) // user pressed esc key
			{
				e.CancelEdit=true;
				return;
			}

			// Does the new name exist
			foreach (LanguageInfo.Region region in languageInfo.Regions)
			{
				if (region.Name==e.Label)
				{
					e.CancelEdit=true;
					return;
				}
			}

			// Find the item the user renames and set the new region name
			foreach (LanguageInfo.Region region in languageInfo.Regions)
			{
				if (region.Name==listViewRegions.Items[e.Item].Text)
				{
					region.Name=e.Label;
					return;
				}
			}

			// Region not found 
			e.CancelEdit=true;
		}

		#endregion
	}
}
