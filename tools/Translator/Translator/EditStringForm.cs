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
			this.groupBox1.Controls.Add(this.textBoxTranslated);
			this.groupBox1.Controls.Add(this.textBoxOriginal);
			this.groupBox1.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.groupBox1.Location = new System.Drawing.Point(8, 8);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(392, 144);
			this.groupBox1.TabIndex = 1;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Selection";
			// 
			// textBoxTranslated
			// 
			this.textBoxTranslated.Location = new System.Drawing.Point(16, 40);
			this.textBoxTranslated.Name = "textBoxTranslated";
			this.textBoxTranslated.Size = new System.Drawing.Size(360, 20);
			this.textBoxTranslated.TabIndex = 3;
			this.textBoxTranslated.Text = "";
			// 
			// textBoxOriginal
			// 
			this.textBoxOriginal.Location = new System.Drawing.Point(16, 96);
			this.textBoxOriginal.Name = "textBoxOriginal";
			this.textBoxOriginal.ReadOnly = true;
			this.textBoxOriginal.Size = new System.Drawing.Size(360, 20);
			this.textBoxOriginal.TabIndex = 5;
			this.textBoxOriginal.Text = "";
			// 
			// label2
			// 
			this.label2.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.label2.Location = new System.Drawing.Point(24, 32);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(360, 16);
			this.label2.TabIndex = 2;
			this.label2.Text = "&Translated:";
			// 
			// label1
			// 
			this.label1.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.label1.Location = new System.Drawing.Point(24, 88);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(360, 16);
			this.label1.TabIndex = 4;
			this.label1.Text = "English:";
			// 
			// buttonCancel
			// 
			this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.buttonCancel.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonCancel.Location = new System.Drawing.Point(328, 160);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.TabIndex = 7;
			this.buttonCancel.Text = "Cancel";
			// 
			// buttonOk
			// 
			this.buttonOk.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.buttonOk.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonOk.Location = new System.Drawing.Point(240, 160);
			this.buttonOk.Name = "buttonOk";
			this.buttonOk.TabIndex = 6;
			this.buttonOk.Text = "OK";
			// 
			// EditStringForm
			// 
			this.AcceptButton = this.buttonOk;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.buttonCancel;
			this.ClientSize = new System.Drawing.Size(410, 192);
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
