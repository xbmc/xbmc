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
using TeamXBMC.TranslatorCore;

namespace TeamXBMC.Translator
{
	/// <summary>
	/// Form to edit a selected string of the listview from the main form.
	/// </summary>
	public class EditStringForm : System.Windows.Forms.Form
	{
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.TextBox textBoxTranslated;
		private System.Windows.Forms.TextBox textBoxOriginal;
		private System.Windows.Forms.Button buttonCancel;
		private System.Windows.Forms.Button buttonOk;
		private System.Windows.Forms.Button buttonNewline;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public EditStringForm()
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
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.buttonNewline = new System.Windows.Forms.Button();
			this.textBoxTranslated = new System.Windows.Forms.TextBox();
			this.textBoxOriginal = new System.Windows.Forms.TextBox();
			this.label2 = new System.Windows.Forms.Label();
			this.label1 = new System.Windows.Forms.Label();
			this.buttonCancel = new System.Windows.Forms.Button();
			this.buttonOk = new System.Windows.Forms.Button();
			this.groupBox1.SuspendLayout();
			this.SuspendLayout();
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.Add(this.buttonNewline);
			this.groupBox1.Controls.Add(this.textBoxTranslated);
			this.groupBox1.Controls.Add(this.textBoxOriginal);
			this.groupBox1.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.groupBox1.Location = new System.Drawing.Point(8, 8);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(392, 152);
			this.groupBox1.TabIndex = 1;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Selection";
			// 
			// buttonNewline
			// 
			this.buttonNewline.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonNewline.Location = new System.Drawing.Point(352, 48);
			this.buttonNewline.Name = "buttonNewline";
			this.buttonNewline.Size = new System.Drawing.Size(24, 23);
			this.buttonNewline.TabIndex = 4;
			this.buttonNewline.Text = "¶";
			this.buttonNewline.Click += new System.EventHandler(this.buttonNewline_Click);
			// 
			// textBoxTranslated
			// 
			this.textBoxTranslated.Location = new System.Drawing.Point(16, 48);
			this.textBoxTranslated.Name = "textBoxTranslated";
			this.textBoxTranslated.Size = new System.Drawing.Size(328, 20);
			this.textBoxTranslated.TabIndex = 3;
			this.textBoxTranslated.Text = "";
			// 
			// textBoxOriginal
			// 
			this.textBoxOriginal.Location = new System.Drawing.Point(16, 112);
			this.textBoxOriginal.Name = "textBoxOriginal";
			this.textBoxOriginal.ReadOnly = true;
			this.textBoxOriginal.Size = new System.Drawing.Size(360, 20);
			this.textBoxOriginal.TabIndex = 6;
			this.textBoxOriginal.TabStop = false;
			this.textBoxOriginal.Text = "";
			// 
			// label2
			// 
			this.label2.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.label2.Location = new System.Drawing.Point(24, 32);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(352, 16);
			this.label2.TabIndex = 2;
			this.label2.Text = "&Translated:";
			// 
			// label1
			// 
			this.label1.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.label1.Location = new System.Drawing.Point(24, 96);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(360, 16);
			this.label1.TabIndex = 5;
			this.label1.Text = "English:";
			// 
			// buttonCancel
			// 
			this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.buttonCancel.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonCancel.Location = new System.Drawing.Point(328, 168);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.TabIndex = 8;
			this.buttonCancel.Text = "Cancel";
			// 
			// buttonOk
			// 
			this.buttonOk.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.buttonOk.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonOk.Location = new System.Drawing.Point(240, 168);
			this.buttonOk.Name = "buttonOk";
			this.buttonOk.TabIndex = 7;
			this.buttonOk.Text = "OK";
			// 
			// EditStringForm
			// 
			this.AcceptButton = this.buttonOk;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.buttonCancel;
			this.ClientSize = new System.Drawing.Size(410, 200);
			this.Controls.Add(this.buttonOk);
			this.Controls.Add(this.buttonCancel);
			this.Controls.Add(this.label1);
			this.Controls.Add(this.label2);
			this.Controls.Add(this.groupBox1);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "EditStringForm";
			this.ShowInTaskbar = false;
			this.SizeGripStyle = System.Windows.Forms.SizeGripStyle.Hide;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Edit String";
			this.Load += new System.EventHandler(this.EditStringForm_Load);
			this.groupBox1.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		#region Form initialization

		/// <summary>
		/// Sets the label controls with the names of the languages
		/// </summary>
		private void EditStringForm_Load(object sender, System.EventArgs e)
		{
			label1.Text=TranslationManager.Instance.LanguageOriginal;
			label2.Text=TranslationManager.Instance.LanguageTranslated;
		}

		#endregion

		#region Control Events

		private void buttonNewline_Click(object sender, System.EventArgs e)
		{
			if (textBoxTranslated.SelectionStart>=0)
			{
				int start=textBoxTranslated.SelectionStart;
				int length=textBoxTranslated.SelectionLength;

				if (length==0)
				{ // caret at a position but no text marked,
					// just insert the ¶
					textBoxTranslated.Text=textBoxTranslated.Text.Insert(start, "¶");
				}
				else
				{ // caret at a position with text marked,
					// replace the marked text with ¶
					textBoxTranslated.Text=textBoxTranslated.Text.Remove(start, length);
					textBoxTranslated.Text=textBoxTranslated.Text.Insert(start, "¶");
				}

				// Update caret position to where we inserted the newline
				textBoxTranslated.Focus(); // Needs focus to move the caret
				textBoxTranslated.SelectionStart=start+1;
				textBoxTranslated.SelectionLength=0;
			}
		}

		#endregion

		#region Properties

		/// <summary>
		/// Gets/Sets the translated string of the language
		/// </summary>
		public string Translated
		{
			get { return textBoxTranslated.Text; }
			set { textBoxTranslated.Text=value; }
		}

		/// <summary>
		/// Gets/Sets the original string of the language
		/// </summary>
		public string Original
		{
			set { textBoxOriginal.Text=value; }
		}

		#endregion
	}
}
