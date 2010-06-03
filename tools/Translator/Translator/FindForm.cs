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

namespace TeamXBMC.Translator
{
	/// <summary>
	/// Form to enter find criteria.
	/// </summary>
	public sealed class FindForm : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.TextBox textBoxFind;
		private System.Windows.Forms.Button buttonCancel;
		private System.Windows.Forms.Button buttonFindNext;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.RadioButton radioButtonUp;
		private System.Windows.Forms.RadioButton radioButtonDown;
		private System.Windows.Forms.CheckBox checkBoxMatchCase;
		private System.Windows.Forms.CheckBox checkBoxMatchWholeWord;
		private bool matchCase=false;
		private bool matchWholeWord=false;
		private bool searchDown=true;
		private string textFind;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public FindForm()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();
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
			this.textBoxFind = new System.Windows.Forms.TextBox();
			this.label1 = new System.Windows.Forms.Label();
			this.buttonCancel = new System.Windows.Forms.Button();
			this.buttonFindNext = new System.Windows.Forms.Button();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.radioButtonDown = new System.Windows.Forms.RadioButton();
			this.radioButtonUp = new System.Windows.Forms.RadioButton();
			this.checkBoxMatchWholeWord = new System.Windows.Forms.CheckBox();
			this.checkBoxMatchCase = new System.Windows.Forms.CheckBox();
			this.groupBox1.SuspendLayout();
			this.SuspendLayout();
			// 
			// textBoxFind
			// 
			this.textBoxFind.Location = new System.Drawing.Point(72, 8);
			this.textBoxFind.Name = "textBoxFind";
			this.textBoxFind.Size = new System.Drawing.Size(184, 20);
			this.textBoxFind.TabIndex = 1;
			this.textBoxFind.Text = "";
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(8, 8);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(100, 24);
			this.label1.TabIndex = 1;
			this.label1.Text = "Find what:";
			// 
			// buttonCancel
			// 
			this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.buttonCancel.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonCancel.Location = new System.Drawing.Point(264, 40);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.TabIndex = 3;
			this.buttonCancel.Text = "Cancel";
			// 
			// buttonFindNext
			// 
			this.buttonFindNext.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.buttonFindNext.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonFindNext.Location = new System.Drawing.Point(264, 8);
			this.buttonFindNext.Name = "buttonFindNext";
			this.buttonFindNext.TabIndex = 2;
			this.buttonFindNext.Text = "Find &Next";
			this.buttonFindNext.Click += new System.EventHandler(this.buttonFindNext_Click);
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.Add(this.radioButtonDown);
			this.groupBox1.Controls.Add(this.radioButtonUp);
			this.groupBox1.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.groupBox1.Location = new System.Drawing.Point(168, 40);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(88, 72);
			this.groupBox1.TabIndex = 6;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Direction";
			// 
			// radioButtonDown
			// 
			this.radioButtonDown.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.radioButtonDown.Location = new System.Drawing.Point(16, 40);
			this.radioButtonDown.Name = "radioButtonDown";
			this.radioButtonDown.Size = new System.Drawing.Size(56, 24);
			this.radioButtonDown.TabIndex = 8;
			this.radioButtonDown.Text = "Down";
			// 
			// radioButtonUp
			// 
			this.radioButtonUp.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.radioButtonUp.Location = new System.Drawing.Point(16, 16);
			this.radioButtonUp.Name = "radioButtonUp";
			this.radioButtonUp.Size = new System.Drawing.Size(56, 24);
			this.radioButtonUp.TabIndex = 7;
			this.radioButtonUp.Text = "Up";
			// 
			// checkBoxMatchWholeWord
			// 
			this.checkBoxMatchWholeWord.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.checkBoxMatchWholeWord.Location = new System.Drawing.Point(8, 40);
			this.checkBoxMatchWholeWord.Name = "checkBoxMatchWholeWord";
			this.checkBoxMatchWholeWord.Size = new System.Drawing.Size(128, 24);
			this.checkBoxMatchWholeWord.TabIndex = 4;
			this.checkBoxMatchWholeWord.Text = "Match whole word";
			// 
			// checkBoxMatchCase
			// 
			this.checkBoxMatchCase.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.checkBoxMatchCase.Location = new System.Drawing.Point(8, 64);
			this.checkBoxMatchCase.Name = "checkBoxMatchCase";
			this.checkBoxMatchCase.Size = new System.Drawing.Size(128, 24);
			this.checkBoxMatchCase.TabIndex = 5;
			this.checkBoxMatchCase.Text = "Match case";
			// 
			// FindForm
			// 
			this.AcceptButton = this.buttonFindNext;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.buttonCancel;
			this.ClientSize = new System.Drawing.Size(346, 120);
			this.Controls.Add(this.checkBoxMatchCase);
			this.Controls.Add(this.checkBoxMatchWholeWord);
			this.Controls.Add(this.groupBox1);
			this.Controls.Add(this.buttonFindNext);
			this.Controls.Add(this.buttonCancel);
			this.Controls.Add(this.textBoxFind);
			this.Controls.Add(this.label1);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "FindForm";
			this.ShowInTaskbar = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Find";
			this.Load += new System.EventHandler(this.FindForm_Load);
			this.Activated += new EventHandler(FindForm_Activated);
			this.groupBox1.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		#region Form initialization

		/// <summary>
		/// Updates controls with the current seach criteria
		/// </summary>
		private void FindForm_Load(object sender, System.EventArgs e)
		{
			checkBoxMatchCase.Checked=matchCase;
			checkBoxMatchWholeWord.Checked=matchWholeWord;
			if (searchDown) radioButtonDown.Checked=true;
			else radioButtonUp.Checked=true;
			textBoxFind.Text=textFind;
		}

		private void FindForm_Activated(object sender, EventArgs e)
		{
			textBoxFind.Focus();
		}

		#endregion

		#region Control Events

		/// <summary>
		/// Closes the form and updates the properties
		/// </summary>
		private void buttonFindNext_Click(object sender, System.EventArgs e)
		{
			matchCase=checkBoxMatchCase.Checked;
			matchWholeWord=checkBoxMatchWholeWord.Checked;
			searchDown=radioButtonDown.Checked;
			textFind=textBoxFind.Text;
		}

		#endregion

		#region Properties

		/// <summary>
		/// Returns true if the search should be case sensitive
		/// </summary>
		public bool MatchCase
		{
			get { return matchCase; }
		}

		/// <summary>
		/// Returns true if the search should match the whole word
		/// </summary>
		public bool MatchWholeWord
		{
			get { return matchWholeWord; }
		}

		/// <summary>
		/// True if the search direction should be down, otherwise false
		/// </summary>
		public bool SearchDown
		{
			get { return searchDown; }
		}

		/// <summary>
		/// Gets the text to be seached for
		/// </summary>
		public string TextFind
		{
			get { return textFind; }
		}

		#endregion
	}
}
