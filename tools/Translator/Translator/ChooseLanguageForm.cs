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
using TeamXBMC.TranslatorCore;

namespace TeamXBMC.Translator
{
	/// <summary>
	/// Shows all available languages. The user can choose 
	/// the language to be translated.
	/// </summary>
	public sealed class ChooseLanguageForm : System.Windows.Forms.Form
	{
		private System.Windows.Forms.ListBox listBox1;
		private System.Windows.Forms.Button buttonOK;
		private System.Windows.Forms.Button buttonCancel;
		private System.Windows.Forms.Label label1;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public ChooseLanguageForm()
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
			this.buttonCancel = new System.Windows.Forms.Button();
			this.listBox1 = new System.Windows.Forms.ListBox();
			this.label1 = new System.Windows.Forms.Label();
			this.SuspendLayout();
			// 
			// buttonOK
			// 
			this.buttonOK.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.buttonOK.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonOK.Location = new System.Drawing.Point(80, 200);
			this.buttonOK.Name = "buttonOK";
			this.buttonOK.TabIndex = 3;
			this.buttonOK.Text = "OK";
			this.buttonOK.Click += new System.EventHandler(this.buttonOK_Click);
			// 
			// buttonCancel
			// 
			this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.buttonCancel.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonCancel.Location = new System.Drawing.Point(168, 200);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.TabIndex = 4;
			this.buttonCancel.Text = "Cancel";
			// 
			// listBox1
			// 
			this.listBox1.Location = new System.Drawing.Point(8, 32);
			this.listBox1.Name = "listBox1";
			this.listBox1.Size = new System.Drawing.Size(232, 160);
			this.listBox1.Sorted = true;
			this.listBox1.TabIndex = 2;
			this.listBox1.DoubleClick += new System.EventHandler(this.listBox1_DoubleClick);
			this.listBox1.SelectedIndexChanged += new System.EventHandler(this.listBox1_SelectedIndexChanged);
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(8, 8);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(240, 23);
			this.label1.TabIndex = 1;
			this.label1.Text = "Select the language to be translated";
			// 
			// ChooseLanguageForm
			// 
			this.AcceptButton = this.buttonOK;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.buttonCancel;
			this.ClientSize = new System.Drawing.Size(250, 232);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.listBox1);
			this.Controls.Add(this.buttonCancel);
			this.Controls.Add(this.buttonOK);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "ChooseLanguageForm";
			this.ShowInTaskbar = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Choose Language";
			this.Load += new System.EventHandler(this.ChooseLanguageForm_Load);
			this.ResumeLayout(false);

		}
		#endregion

		#region Form initialization

		/// <summary>
		/// Fills the listbox with all available languages and selects the one currently edited
		/// </summary>
		private void ChooseLanguageForm_Load(object sender, System.EventArgs e)
		{
			string root=Settings.Instance.LanguageFolder;
			string[] languages=TranslationManager.Instance.Languages;

			foreach (string language in languages)
			{
				listBox1.Items.Add(language);

				if (Settings.Instance.Language==language)
				{
					listBox1.SelectedItem=language;
				}
			}

			if (Settings.Instance.Language=="")
			{ // No language selected yet, disable ok button
				buttonOK.Enabled=false;
			}
		}


		#endregion

		#region Control Events

		/// <summary>
		/// The double clicked language in the listbox is selected as the current one
		/// and afterwards the dialog is closed
		/// </summary>
		private void listBox1_DoubleClick(object sender, System.EventArgs e)
		{
			Settings.Instance.Language=(string)listBox1.SelectedItem;

			DialogResult=DialogResult.OK;
			Close();
		}

		/// <summary>
		/// Sets the language selected in the listbox as the current one.
		/// </summary>
		private void buttonOK_Click(object sender, System.EventArgs e)
		{
			Settings.Instance.Language=(string)listBox1.SelectedItem;
		}

		private void listBox1_SelectedIndexChanged(object sender, System.EventArgs e)
		{
			buttonOK.Enabled=true;
		}

		#endregion
	}
}
