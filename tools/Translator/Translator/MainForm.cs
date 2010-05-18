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
using System.ComponentModel;
using System.Windows.Forms;
using System.IO;
using TeamXBMC.TranslatorCore;

namespace TeamXBMC.Translator
{
	/// <summary>
	/// Main form of the application.
	/// </summary>
	public sealed class MainForm : System.Windows.Forms.Form
	{
		private System.Windows.Forms.ListView listView1;
		private System.Windows.Forms.TabControl tabControl1;
		private System.Windows.Forms.TabPage tabPage1;
		private System.Windows.Forms.TabPage tabPage2;
		private System.Windows.Forms.TabPage tabPage3;
		private System.Windows.Forms.MainMenu mainMenu1;
		private System.Windows.Forms.MenuItem menuItemExit;
		private System.Windows.Forms.MenuItem menuItemFile;
		private System.Windows.Forms.MenuItem menuItem1;
		private System.Windows.Forms.MenuItem menuItemAbout;
		private System.Windows.Forms.MenuItem menuItem3;
		private System.Windows.Forms.FolderBrowserDialog folderBrowserDialog1;
		private System.Windows.Forms.MenuItem menuItemSave;
		private System.Windows.Forms.MenuItem menuItem4;
		private System.Windows.Forms.MenuItem menuItem2;
		private System.Windows.Forms.ListView listView2;
		private System.Windows.Forms.ColumnHeader columnHeader1;
		private System.Windows.Forms.ColumnHeader columnHeader2;
		private System.Windows.Forms.ColumnHeader columnHeader3;
		private System.Windows.Forms.ListView listView3;
		private System.Windows.Forms.ColumnHeader columnHeader4;
		private System.Windows.Forms.ColumnHeader columnHeader5;
		private System.Windows.Forms.ColumnHeader columnHeader6;
		private System.Windows.Forms.ColumnHeader String;
		private System.Windows.Forms.ColumnHeader Original;
		private System.Windows.Forms.ColumnHeader StingId;
		private System.Windows.Forms.MenuItem menuItemOpen;
		private System.Windows.Forms.MenuItem menuItemLanguageFolder;
		private System.Windows.Forms.MenuItem menuItemEdit;
		private System.Windows.Forms.MenuItem menuItemFind;
		private System.ComponentModel.IContainer components=null;
		private System.Windows.Forms.MenuItem menuItemFindNext;
		private System.Windows.Forms.MenuItem menuItemLanguageInfo;
		private System.Windows.Forms.MenuItem menuItem6;
		private System.Windows.Forms.OpenFileDialog openFileDialog1;
		private System.Windows.Forms.MenuItem menuItemConvert;
		private System.Windows.Forms.SaveFileDialog saveFileDialog1;
		private System.Windows.Forms.MenuItem menuItemUserName;
		private System.Windows.Forms.MenuItem menuItemNew;
		private System.Windows.Forms.MenuItem menuItem7;
		private System.Windows.Forms.MenuItem menuItemOptions;
		private System.Windows.Forms.MenuItem menuItemTools;
		private System.Windows.Forms.MenuItem menuItemValidate;
		private System.Windows.Forms.MenuItem menuItemHelp;
		private System.Windows.Forms.MenuItem menuItem8;
		private FindForm findForm=new FindForm();

		public MainForm()
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
				if (components != null) 
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
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(MainForm));
			this.listView1 = new System.Windows.Forms.ListView();
			this.StingId = new System.Windows.Forms.ColumnHeader();
			this.String = new System.Windows.Forms.ColumnHeader();
			this.Original = new System.Windows.Forms.ColumnHeader();
			this.tabControl1 = new System.Windows.Forms.TabControl();
			this.tabPage1 = new System.Windows.Forms.TabPage();
			this.tabPage2 = new System.Windows.Forms.TabPage();
			this.listView2 = new System.Windows.Forms.ListView();
			this.columnHeader1 = new System.Windows.Forms.ColumnHeader();
			this.columnHeader2 = new System.Windows.Forms.ColumnHeader();
			this.columnHeader3 = new System.Windows.Forms.ColumnHeader();
			this.tabPage3 = new System.Windows.Forms.TabPage();
			this.listView3 = new System.Windows.Forms.ListView();
			this.columnHeader4 = new System.Windows.Forms.ColumnHeader();
			this.columnHeader5 = new System.Windows.Forms.ColumnHeader();
			this.columnHeader6 = new System.Windows.Forms.ColumnHeader();
			this.mainMenu1 = new System.Windows.Forms.MainMenu();
			this.menuItemFile = new System.Windows.Forms.MenuItem();
			this.menuItemNew = new System.Windows.Forms.MenuItem();
			this.menuItem7 = new System.Windows.Forms.MenuItem();
			this.menuItemOpen = new System.Windows.Forms.MenuItem();
			this.menuItem2 = new System.Windows.Forms.MenuItem();
			this.menuItemSave = new System.Windows.Forms.MenuItem();
			this.menuItem4 = new System.Windows.Forms.MenuItem();
			this.menuItemLanguageFolder = new System.Windows.Forms.MenuItem();
			this.menuItem3 = new System.Windows.Forms.MenuItem();
			this.menuItemExit = new System.Windows.Forms.MenuItem();
			this.menuItemEdit = new System.Windows.Forms.MenuItem();
			this.menuItemFind = new System.Windows.Forms.MenuItem();
			this.menuItemFindNext = new System.Windows.Forms.MenuItem();
			this.menuItem6 = new System.Windows.Forms.MenuItem();
			this.menuItemLanguageInfo = new System.Windows.Forms.MenuItem();
			this.menuItemOptions = new System.Windows.Forms.MenuItem();
			this.menuItemUserName = new System.Windows.Forms.MenuItem();
			this.menuItemTools = new System.Windows.Forms.MenuItem();
			this.menuItemValidate = new System.Windows.Forms.MenuItem();
			this.menuItemConvert = new System.Windows.Forms.MenuItem();
			this.menuItem1 = new System.Windows.Forms.MenuItem();
			this.menuItemHelp = new System.Windows.Forms.MenuItem();
			this.menuItem8 = new System.Windows.Forms.MenuItem();
			this.menuItemAbout = new System.Windows.Forms.MenuItem();
			this.folderBrowserDialog1 = new System.Windows.Forms.FolderBrowserDialog();
			this.openFileDialog1 = new System.Windows.Forms.OpenFileDialog();
			this.saveFileDialog1 = new System.Windows.Forms.SaveFileDialog();
			this.tabControl1.SuspendLayout();
			this.tabPage1.SuspendLayout();
			this.tabPage2.SuspendLayout();
			this.tabPage3.SuspendLayout();
			this.SuspendLayout();
			// 
			// listView1
			// 
			this.listView1.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
																																								this.StingId,
																																								this.String,
																																								this.Original});
			this.listView1.FullRowSelect = true;
			this.listView1.GridLines = true;
			this.listView1.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
			this.listView1.HideSelection = false;
			this.listView1.LabelWrap = false;
			this.listView1.Location = new System.Drawing.Point(0, 0);
			this.listView1.MultiSelect = false;
			this.listView1.Name = "listView1";
			this.listView1.Size = new System.Drawing.Size(696, 360);
			this.listView1.TabIndex = 0;
			this.listView1.View = System.Windows.Forms.View.Details;
			this.listView1.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.listView_KeyPress);
			this.listView1.DoubleClick += new System.EventHandler(this.listView_DoubleClick);
			// 
			// StingId
			// 
			this.StingId.Text = "String Id";
			this.StingId.Width = 59;
			// 
			// String
			// 
			this.String.Text = "String";
			this.String.Width = 308;
			// 
			// Original
			// 
			this.Original.Text = "Original";
			this.Original.Width = 308;
			// 
			// tabControl1
			// 
			this.tabControl1.Appearance = System.Windows.Forms.TabAppearance.FlatButtons;
			this.tabControl1.Controls.Add(this.tabPage1);
			this.tabControl1.Controls.Add(this.tabPage2);
			this.tabControl1.Controls.Add(this.tabPage3);
			this.tabControl1.ItemSize = new System.Drawing.Size(55, 21);
			this.tabControl1.Location = new System.Drawing.Point(8, 8);
			this.tabControl1.Multiline = true;
			this.tabControl1.Name = "tabControl1";
			this.tabControl1.RightToLeft = System.Windows.Forms.RightToLeft.No;
			this.tabControl1.SelectedIndex = 0;
			this.tabControl1.Size = new System.Drawing.Size(704, 392);
			this.tabControl1.TabIndex = 0;
			// 
			// tabPage1
			// 
			this.tabPage1.BackColor = System.Drawing.SystemColors.Control;
			this.tabPage1.Controls.Add(this.listView1);
			this.tabPage1.Location = new System.Drawing.Point(4, 25);
			this.tabPage1.Name = "tabPage1";
			this.tabPage1.Size = new System.Drawing.Size(696, 363);
			this.tabPage1.TabIndex = 0;
			this.tabPage1.Text = "All Strings";
			// 
			// tabPage2
			// 
			this.tabPage2.BackColor = System.Drawing.SystemColors.Control;
			this.tabPage2.Controls.Add(this.listView2);
			this.tabPage2.Location = new System.Drawing.Point(4, 25);
			this.tabPage2.Name = "tabPage2";
			this.tabPage2.Size = new System.Drawing.Size(696, 363);
			this.tabPage2.TabIndex = 1;
			this.tabPage2.Text = "Untranslated";
			// 
			// listView2
			// 
			this.listView2.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
																																								this.columnHeader1,
																																								this.columnHeader2,
																																								this.columnHeader3});
			this.listView2.FullRowSelect = true;
			this.listView2.GridLines = true;
			this.listView2.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
			this.listView2.HideSelection = false;
			this.listView2.LabelWrap = false;
			this.listView2.Location = new System.Drawing.Point(0, 0);
			this.listView2.MultiSelect = false;
			this.listView2.Name = "listView2";
			this.listView2.Size = new System.Drawing.Size(696, 360);
			this.listView2.TabIndex = 1;
			this.listView2.View = System.Windows.Forms.View.Details;
			this.listView2.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.listView_KeyPress);
			this.listView2.DoubleClick += new System.EventHandler(this.listView_DoubleClick);
			// 
			// columnHeader1
			// 
			this.columnHeader1.Text = "String Id";
			this.columnHeader1.Width = 59;
			// 
			// columnHeader2
			// 
			this.columnHeader2.Text = "String";
			this.columnHeader2.Width = 308;
			// 
			// columnHeader3
			// 
			this.columnHeader3.Text = "Original";
			this.columnHeader3.Width = 308;
			// 
			// tabPage3
			// 
			this.tabPage3.BackColor = System.Drawing.SystemColors.Control;
			this.tabPage3.Controls.Add(this.listView3);
			this.tabPage3.Location = new System.Drawing.Point(4, 25);
			this.tabPage3.Name = "tabPage3";
			this.tabPage3.Size = new System.Drawing.Size(696, 363);
			this.tabPage3.TabIndex = 2;
			this.tabPage3.Text = "Changed";
			// 
			// listView3
			// 
			this.listView3.Columns.AddRange(new System.Windows.Forms.ColumnHeader[] {
																																								this.columnHeader4,
																																								this.columnHeader5,
																																								this.columnHeader6});
			this.listView3.FullRowSelect = true;
			this.listView3.GridLines = true;
			this.listView3.HeaderStyle = System.Windows.Forms.ColumnHeaderStyle.Nonclickable;
			this.listView3.HideSelection = false;
			this.listView3.LabelWrap = false;
			this.listView3.Location = new System.Drawing.Point(0, 0);
			this.listView3.MultiSelect = false;
			this.listView3.Name = "listView3";
			this.listView3.Size = new System.Drawing.Size(696, 360);
			this.listView3.TabIndex = 1;
			this.listView3.View = System.Windows.Forms.View.Details;
			this.listView3.KeyPress += new System.Windows.Forms.KeyPressEventHandler(this.listView_KeyPress);
			this.listView3.DoubleClick += new System.EventHandler(this.listView_DoubleClick);
			// 
			// columnHeader4
			// 
			this.columnHeader4.Text = "String Id";
			this.columnHeader4.Width = 59;
			// 
			// columnHeader5
			// 
			this.columnHeader5.Text = "String";
			this.columnHeader5.Width = 308;
			// 
			// columnHeader6
			// 
			this.columnHeader6.Text = "Original";
			this.columnHeader6.Width = 308;
			// 
			// mainMenu1
			// 
			this.mainMenu1.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																																							this.menuItemFile,
																																							this.menuItemEdit,
																																							this.menuItemOptions,
																																							this.menuItemTools,
																																							this.menuItem1});
			// 
			// menuItemFile
			// 
			this.menuItemFile.Index = 0;
			this.menuItemFile.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																																								 this.menuItemNew,
																																								 this.menuItem7,
																																								 this.menuItemOpen,
																																								 this.menuItem2,
																																								 this.menuItemSave,
																																								 this.menuItem4,
																																								 this.menuItemLanguageFolder,
																																								 this.menuItem3,
																																								 this.menuItemExit});
			this.menuItemFile.Text = "&File";
			// 
			// menuItemNew
			// 
			this.menuItemNew.Index = 0;
			this.menuItemNew.Shortcut = System.Windows.Forms.Shortcut.CtrlN;
			this.menuItemNew.Text = "&New Language...";
			this.menuItemNew.Click += new System.EventHandler(this.menuItemNew_Click);
			// 
			// menuItem7
			// 
			this.menuItem7.Index = 1;
			this.menuItem7.Text = "-";
			// 
			// menuItemOpen
			// 
			this.menuItemOpen.Index = 2;
			this.menuItemOpen.Shortcut = System.Windows.Forms.Shortcut.CtrlO;
			this.menuItemOpen.Text = "&Open Language...";
			this.menuItemOpen.Click += new System.EventHandler(this.menuItemOpen_Click);
			// 
			// menuItem2
			// 
			this.menuItem2.Index = 3;
			this.menuItem2.Text = "-";
			// 
			// menuItemSave
			// 
			this.menuItemSave.Index = 4;
			this.menuItemSave.Shortcut = System.Windows.Forms.Shortcut.CtrlS;
			this.menuItemSave.Text = "&Save";
			this.menuItemSave.Click += new System.EventHandler(this.menuItemSave_Click);
			// 
			// menuItem4
			// 
			this.menuItem4.Index = 5;
			this.menuItem4.Text = "-";
			// 
			// menuItemLanguageFolder
			// 
			this.menuItemLanguageFolder.Index = 6;
			this.menuItemLanguageFolder.Text = "Set &Language Folder...";
			this.menuItemLanguageFolder.Click += new System.EventHandler(this.menuItemLanguageFolder_Click);
			// 
			// menuItem3
			// 
			this.menuItem3.Index = 7;
			this.menuItem3.Text = "-";
			// 
			// menuItemExit
			// 
			this.menuItemExit.Index = 8;
			this.menuItemExit.Shortcut = System.Windows.Forms.Shortcut.AltF4;
			this.menuItemExit.Text = "&Exit";
			this.menuItemExit.Click += new System.EventHandler(this.menuItemExit_Click);
			// 
			// menuItemEdit
			// 
			this.menuItemEdit.Index = 1;
			this.menuItemEdit.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																																								 this.menuItemFind,
																																								 this.menuItemFindNext,
																																								 this.menuItem6,
																																								 this.menuItemLanguageInfo});
			this.menuItemEdit.Text = "&Edit";
			// 
			// menuItemFind
			// 
			this.menuItemFind.Index = 0;
			this.menuItemFind.Shortcut = System.Windows.Forms.Shortcut.CtrlF;
			this.menuItemFind.Text = "&Find...";
			this.menuItemFind.Click += new System.EventHandler(this.menuItemFind_Click);
			// 
			// menuItemFindNext
			// 
			this.menuItemFindNext.Enabled = false;
			this.menuItemFindNext.Index = 1;
			this.menuItemFindNext.Shortcut = System.Windows.Forms.Shortcut.F3;
			this.menuItemFindNext.Text = "Find &Next";
			this.menuItemFindNext.Click += new System.EventHandler(this.menuItemFindNext_Click);
			// 
			// menuItem6
			// 
			this.menuItem6.Index = 2;
			this.menuItem6.Text = "-";
			// 
			// menuItemLanguageInfo
			// 
			this.menuItemLanguageInfo.Index = 3;
			this.menuItemLanguageInfo.Text = "Language &Settings...";
			this.menuItemLanguageInfo.Click += new System.EventHandler(this.menuItemLanguageInfo_Click);
			// 
			// menuItemOptions
			// 
			this.menuItemOptions.Index = 2;
			this.menuItemOptions.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																																										this.menuItemUserName});
			this.menuItemOptions.Text = "&Options";
			// 
			// menuItemUserName
			// 
			this.menuItemUserName.Index = 0;
			this.menuItemUserName.Text = "&Translator Name...";
			this.menuItemUserName.Click += new System.EventHandler(this.menuItemUserName_Click);
			// 
			// menuItemTools
			// 
			this.menuItemTools.Index = 3;
			this.menuItemTools.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																																									this.menuItemValidate,
																																									this.menuItemConvert});
			this.menuItemTools.Text = "&Tools";
			// 
			// menuItemValidate
			// 
			this.menuItemValidate.Index = 0;
			this.menuItemValidate.Text = "&Validate Language File...";
			this.menuItemValidate.Click += new System.EventHandler(this.menuItemValidateFile_Click);
			// 
			// menuItemConvert
			// 
			this.menuItemConvert.Index = 1;
			this.menuItemConvert.Text = "&Convert Language File...";
			this.menuItemConvert.Click += new System.EventHandler(this.menuItemConvert_Click);
			// 
			// menuItem1
			// 
			this.menuItem1.Index = 4;
			this.menuItem1.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																																							this.menuItemHelp,
																																							this.menuItem8,
																																							this.menuItemAbout});
			this.menuItem1.Text = "&?";
			// 
			// menuItemHelp
			// 
			this.menuItemHelp.Index = 0;
			this.menuItemHelp.Text = "&Help...";
			this.menuItemHelp.Click += new System.EventHandler(this.menuItemHelp_Click);
			// 
			// menuItem8
			// 
			this.menuItem8.Index = 1;
			this.menuItem8.Text = "-";
			// 
			// menuItemAbout
			// 
			this.menuItemAbout.Index = 2;
			this.menuItemAbout.Text = "&About...";
			this.menuItemAbout.Click += new System.EventHandler(this.menuItemAbout_Click);
			// 
			// openFileDialog1
			// 
			this.openFileDialog1.DefaultExt = "xml";
			this.openFileDialog1.FileName = "strings.xml";
			this.openFileDialog1.Filter = "Language File|strings.xml|All Files|*.*";
			this.openFileDialog1.Title = "Choose the language file";
			// 
			// saveFileDialog1
			// 
			this.saveFileDialog1.FileName = "strings.xml";
			this.saveFileDialog1.Filter = "Language File|strings.xml|All Files|*.*";
			this.saveFileDialog1.Title = "Save converted file as";
			// 
			// MainForm
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(722, 411);
			this.Controls.Add(this.tabControl1);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.Icon = ((System.Drawing.Icon)(resources.GetObject("$this.Icon")));
			this.MaximizeBox = false;
			this.Menu = this.mainMenu1;
			this.Name = "MainForm";
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
			this.Text = "Team XBMC Translator";
			this.Load += new System.EventHandler(this.MainForm_Load);
			this.Closing +=new CancelEventHandler(MainForm_Closing);
			this.tabControl1.ResumeLayout(false);
			this.tabPage1.ResumeLayout(false);
			this.tabPage2.ResumeLayout(false);
			this.tabPage3.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		#region Application initialization
		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.EnableVisualStyles();
			Application.DoEvents();
			Application.Run(new MainForm());
		}
		#endregion

		#region Form initialization

		/// <summary>
		/// Loaded the last edited language, if any
		/// </summary>
		private void MainForm_Load(object sender, System.EventArgs e)
		{
			Initialize();
		}

		/// <summary>
		/// Check if we have to save the language file
		/// </summary>
		private void MainForm_Closing(object sender, CancelEventArgs e)
		{
			DialogResult result=ShouldSaveLanguageFile();
			if (result==DialogResult.Yes)
				SaveLanguageFile();
			else if (result==DialogResult.Cancel)
				e.Cancel=true;
		}

		/// <summary>
		/// Initializes the user interface
		/// </summary>
		private bool Initialize()
		{
			try
			{
				// disable menu items that are unsave
				// in an undefined state
				EnableMenuItems(false);

				// Reset the form
				Reset();

				// Tell the TranslationManager to load the currently selected 
				// language
				TranslationManager.Instance.Initialize();
			}
			catch (TranslatorException e)
			{
				Settings.Instance.Language=""; // Failed to load language, remove the current one
				ShowMessageBox(BuildErrorMessageText(e), MessageBoxIcon.Error);
				return false;
			}

			// Fill listView with strings
			UpdateListViews();

			if (Settings.Instance.Language=="" && Settings.Instance.LanguageFolder=="")
			{ // No language folder and language available, disable menuItemNew
				menuItemNew.Enabled=false;
			}
			else
			{ // Everything is fine enable all menuItems
				EnableMenuItems(true);
			}

			return true;
		}

		/// <summary>
		/// Resets the user interface
		/// </summary>
		private void Reset()
		{
			ClearListView(listView1);
			ClearListView(listView2);
			ClearListView(listView3);

			EnableMenuItems(false);
		}
		
		/// <summary>
		/// Disables/enalbes the menu items
		/// </summary>
		void EnableMenuItems(bool enabled)
		{
			menuItemNew.Enabled=true;
			menuItemSave.Enabled=enabled;

			menuItemLanguageInfo.Enabled=enabled;
			menuItemFind.Enabled=enabled;
			menuItemFindNext.Enabled=false;
		}

		#endregion

		#region Error presentation

		/// <summary>
		/// Shows a predefined error message box
		/// </summary>
		private void ShowMessageBox(string message, MessageBoxIcon icon)
		{
			MessageBox.Show(message, Application.ProductName, MessageBoxButtons.OK, icon);
		}

		/// <summary>
		/// Construct an error message text from a TranslatorException
		/// </summary>
		private string BuildErrorMessageText(TranslatorException exception)
		{
			string message="The following error occured:\n";
			message+=exception.Message;
			if (exception.InnerException!=null)
			{
				message+="\nAdditional Info:\n";
				message+=exception.InnerException.Message;
			}

			return message;
		}

		#endregion

		#region List View control

		/// <summary>
		/// Update all listviews with its strings
		/// </summary>
		private void UpdateListViews()
		{
			UpdateListView(listView1, TranslationManager.Instance.All);
			UpdateListView(listView2, TranslationManager.Instance.Untranslated);
			UpdateListView(listView3, TranslationManager.Instance.Changed);
		}
		
		/// <summary>
		/// Update a listview with its strings
		/// </summary>
		private void UpdateListView(ListView listView, TranslatorArray strings)
		{
			listView.Columns[1].Text=TranslationManager.Instance.LanguageTranslated;
			listView.Columns[2].Text=TranslationManager.Instance.LanguageOriginal;

			foreach (TranslatorItem item in strings)
			{
				listView.Items.Add(new ListViewItemString(item));
			}
		}

		/// <summary>
		/// Removes items from a listview
		/// </summary>
		private void ClearListView(ListView listView)
		{
			listView.Items.Clear();
		}

		/// <summary>
		/// Shows the EditStringForm when a listview is double clicked
		/// </summary>
		void listView_DoubleClick(object sender, System.EventArgs e)
		{
			ShowEditForm((ListView)sender);
		}

		/// <summary>
		/// Shows the EditStringForm when enter is pressed in a listview
		/// </summary>
		void listView_KeyPress(object sender, KeyPressEventArgs e)
		{
			if (e.KeyChar==(char)13)
			{
				ShowEditForm((ListView)sender);
			}
		}

		/// <summary>
		/// Shows the EditStringForm for a certain listview
		/// </summary>
		void ShowEditForm(ListView listView)
		{
			if (listView.SelectedItems.Count>0)
			{
				ListViewItem item=listView.SelectedItems[0];
				long stringId=Convert.ToInt32(item.SubItems[0].Text);

				EditStringForm form=new EditStringForm();
				form.Translated=item.SubItems[1].Text;
				form.Original=item.SubItems[2].Text;
				if (form.ShowDialog(this)==DialogResult.Cancel)
					return;

				TranslatorItem translatorItem=null;
				TranslationManager.Instance.All.GetItemById(stringId, ref translatorItem);

				if (translatorItem==null)
					return;

				if (form.Translated=="")
					translatorItem.State=TranslationState.Untranslated;
				else
					translatorItem.State=TranslationState.Translated;

				translatorItem.StringTranslated.Text=form.Translated;
			}
		}

		#endregion

		#region Menu Item Handler File

		/// <summary>
		/// Creates a new language
		/// </summary>
		private void menuItemNew_Click(object sender, System.EventArgs e)
		{
			if (Settings.Instance.LanguageFolder=="" && !ShowFolderBrowser())
				return;

			DialogResult result=ShouldSaveLanguageFile();
			if (result==DialogResult.Yes)
				SaveLanguageFile();
			else if (result==DialogResult.Cancel)
				return;

			NewLanguageForm form=new NewLanguageForm();
			if (form.ShowDialog()==DialogResult.Cancel)
				return;

			try
			{
				TranslationManager.Instance.CreateLanguage(form.LanguageName);
			}
			catch (TranslatorException ex)
			{
				ShowMessageBox(BuildErrorMessageText(ex), MessageBoxIcon.Error);
				return;
			}

			Settings.Instance.Language=form.LanguageName;
			Initialize();
		}

		/// <summary>
		/// Opens a language
		/// </summary>
		private void menuItemOpen_Click(object sender, System.EventArgs e)
		{
			if (Settings.Instance.LanguageFolder=="" && !ShowFolderBrowser())
				return;

			DialogResult result=ShouldSaveLanguageFile();
			if (result==DialogResult.Yes)
				SaveLanguageFile();
			else if (result==DialogResult.Cancel)
				return;

			ChooseLanguageForm form=new ChooseLanguageForm();
			if (form.ShowDialog()==DialogResult.Cancel)
				return;

			Initialize();
		}

		/// <summary>
		/// Opens a folder browser to choose the language folder.
		/// It also tests if the folder selected contains the english folder,
		/// if not it will prompt again. Returns false if the user cancels out.
		/// </summary>
		private bool ShowFolderBrowser()
		{
			folderBrowserDialog1.Description="Please choose the root folder where your language files are located. This folder needs at least the english language folder.";
			folderBrowserDialog1.SelectedPath=Settings.Instance.LanguageFolder;
			folderBrowserDialog1.ShowNewFolderButton=false;

			if (folderBrowserDialog1.ShowDialog()!=DialogResult.Cancel)
			{
				Settings.Instance.LanguageFolder=folderBrowserDialog1.SelectedPath;
				if (!File.Exists(Settings.Instance.FilenameOriginal))
				{
					Settings.Instance.LanguageFolder="";
					ShowMessageBox("The english language folder was not found in this directory.\nPlease choose another folder.", MessageBoxIcon.Error);
					return ShowFolderBrowser();
				}

				return true;
			}

			return false;
		}

		/// <summary>
		/// Saves the language file
		/// </summary>
		private void menuItemSave_Click(object sender, System.EventArgs e)
		{
			SaveLanguageFile();
		}

		/// <summary>
		/// Asks the user if he wants to save the language file is it's modified
		/// </summary>
		private DialogResult ShouldSaveLanguageFile()
		{
			if (TranslationManager.Instance.IsModified)
			{
				string message="The language file has been modified.\n\nSave changes?";
				return MessageBox.Show(message, Application.ProductName, MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
			}

			return DialogResult.No;
		}

		/// <summary>
		/// Saves the active language file
		/// </summary>
		private void SaveLanguageFile()
		{
			try
			{
				TranslationManager.Instance.SaveTranslated();
			}
			catch (TranslatorException ex)
			{
				ShowMessageBox(BuildErrorMessageText(ex), MessageBoxIcon.Error);
			}
		}

		/// <summary>
		/// Sets the language folder
		/// </summary>
		private void menuItemLanguageFolder_Click(object sender, System.EventArgs e)
		{
			DialogResult result=ShouldSaveLanguageFile();
			if (result==DialogResult.Yes)
				SaveLanguageFile();
			else if (result==DialogResult.Cancel)
				return;

			if (ShowFolderBrowser())
				Initialize();
		}

		/// <summary>
		/// Exits the application
		/// </summary>
		private void menuItemExit_Click(object sender, System.EventArgs e)
		{
			DialogResult result=ShouldSaveLanguageFile();
			if (result==DialogResult.Yes)
				SaveLanguageFile();
			else if (result==DialogResult.Cancel)
				return;

			Application.Exit();
		}

		#endregion

		#region Menu Item Handler Edit

		/// <summary>
		/// Edits the langinfo of the current language
		/// </summary>
		private void menuItemLanguageInfo_Click(object sender, System.EventArgs e)
		{
			try
			{
				LanguageInfoForm form=new LanguageInfoForm();
				form.ShowDialog();
			}
			catch(TranslatorException ex)
			{
				ShowMessageBox(BuildErrorMessageText(ex), MessageBoxIcon.Error);
			}
		}

		/// <summary>
		/// Shows the FindForm to search in the active listView
		/// </summary>
		private void menuItemFind_Click(object sender, System.EventArgs e)
		{
			findForm.ShowDialog();
			if (findForm.DialogResult==DialogResult.Cancel)
				return;

			menuItemFindNext_Click(sender, e);

			menuItemFindNext.Enabled=true;
		}

		/// <summary>
		/// Searches the active listView for the search criteria set with
		/// the FindForm.
		/// </summary>
		private void menuItemFindNext_Click(object sender, System.EventArgs e)
		{
			ListView listView=null;
			if (tabControl1.SelectedIndex==0)
				listView=listView1;
			else if (tabControl1.SelectedIndex==1)
				listView=listView2;
			else
				listView=listView3;

			if (listView==null)
				return;

			int startIndex=0;
			if (listView.SelectedIndices.Count>0)
				startIndex=listView.SelectedIndices[0]+1;

			string findText=findForm.MatchCase ? findForm.TextFind : findForm.TextFind.ToLower();

			bool found=false;
			for (int i=startIndex; findForm.SearchDown ? i<listView.Items.Count : i>=0; i+=findForm.SearchDown ? 1 : -1)
			{
				ListViewItem item=listView.Items[i];
				foreach (ListViewItem.ListViewSubItem subItem in item.SubItems)
				{
					string text=findForm.MatchCase ? subItem.Text : subItem.Text.ToLower();

					int pos=text.IndexOf(findText);
					if (pos>-1)
					{
						if (findForm.MatchWholeWord)
						{
							if (pos==0 && findText.Length==text.Length)
							{
								if (text!=findText)
									continue;
							}
							else if (pos==0)
							{
								if (Char.IsLetterOrDigit(text[pos+findText.Length]))
									continue;
							}
							else if (pos>0)
							{
								if (Char.IsLetterOrDigit(text[pos-1]))
									continue;

								if (text.Length>pos+findText.Length)
								{
									if (Char.IsLetterOrDigit(text[pos+findText.Length]))
										continue;
								}
							}
						}
						item.Selected=true;
						item.EnsureVisible();
						found=true;
						break;
					}
				}

				if (found)
					break;
			}

			if (!found)
			{
				ShowMessageBox("\""+findForm.TextFind+"\" was not found.", MessageBoxIcon.Information);
			}

		}

		#endregion

		#region Menu Item Handler Options

		/// <summary>
		/// Shows the UserForm where the name and email of the translator can be set
		/// </summary>
		private void menuItemUserName_Click(object sender, System.EventArgs e)
		{
			UserForm form=new UserForm();
			form.ShowDialog();
		}

		#endregion

		#region Menu Item Handler Tools

		/// <summary>
		/// Checks if a language file is valid
		/// </summary>
		private void menuItemValidateFile_Click(object sender, System.EventArgs e)
		{
			openFileDialog1.InitialDirectory=Settings.Instance.LanguageFolder;
			if (openFileDialog1.ShowDialog()==DialogResult.Cancel)
				return;

			StringArray strings=new StringArray();

			try
			{
				strings.Load(openFileDialog1.FileName);
			}
			catch(TranslatorException ex)
			{
				ShowMessageBox(BuildErrorMessageText(ex), MessageBoxIcon.Error);
				return;
			}

			ShowMessageBox("The file " + openFileDialog1.FileName + " is valid.", MessageBoxIcon.Information);
		}

		/// <summary>
		/// Converts a language file to the new format
		/// </summary>
		private void menuItemConvert_Click(object sender, System.EventArgs e)
		{
			openFileDialog1.InitialDirectory=Settings.Instance.LanguageFolder;
			if (openFileDialog1.ShowDialog()==DialogResult.Cancel)
				return;

			StringArray strings=new StringArray();

			try
			{
				strings.Load(openFileDialog1.FileName);
			}
			catch(TranslatorException ex)
			{
				ShowMessageBox(BuildErrorMessageText(ex), MessageBoxIcon.Error);
				return;
			}

			saveFileDialog1.InitialDirectory=openFileDialog1.FileName.Substring(0, openFileDialog1.FileName.LastIndexOf(@"\"));
			if (saveFileDialog1.ShowDialog()==DialogResult.Cancel)
				return;

			try
			{
				string[] comment=new string[1];
				comment[0]="$"+"Revision"+"$";
				strings.Save(saveFileDialog1.FileName, comment);
			}
			catch(TranslatorException ex)
			{
				ShowMessageBox(BuildErrorMessageText(ex), MessageBoxIcon.Error);
				return;
			}

			MessageBox.Show("File "+saveFileDialog1.FileName+" was succesfully saved.", Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Information);
		}

		#endregion

		#region Menu Item Handler ?

		/// <summary>
		/// Opens the standard webbrowser and shows the help page of the online manual
		/// </summary>
		private void menuItemHelp_Click(object sender, System.EventArgs e)
		{
			System.Diagnostics.Process.Start("http://wiki.xbmc.org/index.php?title=XBMC_Translator");
		}

		/// <summary>
		/// Shows the AboutForm
		/// </summary>
		private void menuItemAbout_Click(object sender, System.EventArgs e)
		{
			AboutForm form=new AboutForm();
			form.ShowDialog();
		}

		#endregion
	}
}
