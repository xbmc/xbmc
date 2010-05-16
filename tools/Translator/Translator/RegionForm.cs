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
	/// Summary description for RegionForm.
	/// </summary>
	public class RegionForm : System.Windows.Forms.Form
	{
		private class Unit
		{
			public Unit(string nameShort, string nameLong)
			{
				this.nameShort=nameShort;
				this.nameLong=nameLong;
			}

			public string NameShort
			{
				get { return nameShort; } 
			}

			public string NameLong
			{
				get { return nameLong; } 
			}

			private string nameShort;
			private string nameLong;
		};

		private ArrayList speedUnits=new ArrayList();
		private ArrayList tempUnits=new ArrayList();
		private LanguageInfo.Region region=null;
		private System.Windows.Forms.Button buttonOK;
		private System.Windows.Forms.Button buttonCancel;
		private System.Windows.Forms.TabControl tabControl1;
		private System.Windows.Forms.TabPage tabPage1;
		private System.Windows.Forms.TabPage tabPage2;
		private System.Windows.Forms.TabPage tabPage3;
		private System.Windows.Forms.Label label1;
		private System.Windows.Forms.Label label2;
		private System.Windows.Forms.Label label3;
		private System.Windows.Forms.Label label4;
		private System.Windows.Forms.Label label5;
		private System.Windows.Forms.Label label6;
		private System.Windows.Forms.Label label7;
		private System.Windows.Forms.Label label8;
		private System.Windows.Forms.Label label9;
		private System.Windows.Forms.Label label10;
		private System.Windows.Forms.GroupBox groupBox1;
		private System.Windows.Forms.Label label11;
		private System.Windows.Forms.Label label12;
		private System.Windows.Forms.Label label13;
		private System.Windows.Forms.Label label14;
		private System.Windows.Forms.GroupBox groupBox2;
		private System.Windows.Forms.Label label15;
		private System.Windows.Forms.Label label17;
		private System.Windows.Forms.Label label18;
		private System.Windows.Forms.Label label19;
		private System.Windows.Forms.Label label20;
		private System.Windows.Forms.Label label16;
		private System.Windows.Forms.Label label21;
		private System.Windows.Forms.TextBox textBoxDateShort;
		private System.Windows.Forms.TextBox textBoxDateLong;
		private System.Windows.Forms.TextBox textBoxAM;
		private System.Windows.Forms.TextBox textBoxPM;
		private System.Windows.Forms.TextBox textBoxTime;
		private System.Windows.Forms.ComboBox comboBoxTempUnit;
		private System.Windows.Forms.ComboBox comboBoxSpeedUnit;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public RegionForm()
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
			this.tabControl1 = new System.Windows.Forms.TabControl();
			this.tabPage1 = new System.Windows.Forms.TabPage();
			this.groupBox1 = new System.Windows.Forms.GroupBox();
			this.label11 = new System.Windows.Forms.Label();
			this.label8 = new System.Windows.Forms.Label();
			this.label5 = new System.Windows.Forms.Label();
			this.label4 = new System.Windows.Forms.Label();
			this.label10 = new System.Windows.Forms.Label();
			this.label3 = new System.Windows.Forms.Label();
			this.label9 = new System.Windows.Forms.Label();
			this.label7 = new System.Windows.Forms.Label();
			this.label6 = new System.Windows.Forms.Label();
			this.textBoxDateShort = new System.Windows.Forms.TextBox();
			this.textBoxDateLong = new System.Windows.Forms.TextBox();
			this.label2 = new System.Windows.Forms.Label();
			this.label1 = new System.Windows.Forms.Label();
			this.tabPage2 = new System.Windows.Forms.TabPage();
			this.groupBox2 = new System.Windows.Forms.GroupBox();
			this.label20 = new System.Windows.Forms.Label();
			this.label19 = new System.Windows.Forms.Label();
			this.label18 = new System.Windows.Forms.Label();
			this.label17 = new System.Windows.Forms.Label();
			this.label15 = new System.Windows.Forms.Label();
			this.label14 = new System.Windows.Forms.Label();
			this.label13 = new System.Windows.Forms.Label();
			this.label12 = new System.Windows.Forms.Label();
			this.textBoxAM = new System.Windows.Forms.TextBox();
			this.textBoxPM = new System.Windows.Forms.TextBox();
			this.textBoxTime = new System.Windows.Forms.TextBox();
			this.tabPage3 = new System.Windows.Forms.TabPage();
			this.comboBoxTempUnit = new System.Windows.Forms.ComboBox();
			this.label21 = new System.Windows.Forms.Label();
			this.comboBoxSpeedUnit = new System.Windows.Forms.ComboBox();
			this.label16 = new System.Windows.Forms.Label();
			this.tabControl1.SuspendLayout();
			this.tabPage1.SuspendLayout();
			this.groupBox1.SuspendLayout();
			this.tabPage2.SuspendLayout();
			this.groupBox2.SuspendLayout();
			this.tabPage3.SuspendLayout();
			this.SuspendLayout();
			// 
			// buttonOK
			// 
			this.buttonOK.DialogResult = System.Windows.Forms.DialogResult.OK;
			this.buttonOK.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonOK.Location = new System.Drawing.Point(184, 336);
			this.buttonOK.Name = "buttonOK";
			this.buttonOK.TabIndex = 1;
			this.buttonOK.Text = "OK";
			this.buttonOK.Click += new System.EventHandler(this.buttonOK_Click);
			// 
			// buttonCancel
			// 
			this.buttonCancel.DialogResult = System.Windows.Forms.DialogResult.Cancel;
			this.buttonCancel.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.buttonCancel.Location = new System.Drawing.Point(272, 336);
			this.buttonCancel.Name = "buttonCancel";
			this.buttonCancel.TabIndex = 2;
			this.buttonCancel.Text = "Cancel";
			// 
			// tabControl1
			// 
			this.tabControl1.Controls.Add(this.tabPage1);
			this.tabControl1.Controls.Add(this.tabPage2);
			this.tabControl1.Controls.Add(this.tabPage3);
			this.tabControl1.Location = new System.Drawing.Point(8, 8);
			this.tabControl1.Name = "tabControl1";
			this.tabControl1.SelectedIndex = 0;
			this.tabControl1.Size = new System.Drawing.Size(344, 320);
			this.tabControl1.TabIndex = 0;
			// 
			// tabPage1
			// 
			this.tabPage1.BackColor = System.Drawing.SystemColors.ControlLightLight;
			this.tabPage1.Controls.Add(this.groupBox1);
			this.tabPage1.Controls.Add(this.textBoxDateShort);
			this.tabPage1.Controls.Add(this.textBoxDateLong);
			this.tabPage1.Controls.Add(this.label2);
			this.tabPage1.Controls.Add(this.label1);
			this.tabPage1.Location = new System.Drawing.Point(4, 22);
			this.tabPage1.Name = "tabPage1";
			this.tabPage1.Size = new System.Drawing.Size(336, 294);
			this.tabPage1.TabIndex = 0;
			this.tabPage1.Text = "Date";
			// 
			// groupBox1
			// 
			this.groupBox1.Controls.Add(this.label11);
			this.groupBox1.Controls.Add(this.label8);
			this.groupBox1.Controls.Add(this.label5);
			this.groupBox1.Controls.Add(this.label4);
			this.groupBox1.Controls.Add(this.label10);
			this.groupBox1.Controls.Add(this.label3);
			this.groupBox1.Controls.Add(this.label9);
			this.groupBox1.Controls.Add(this.label7);
			this.groupBox1.Controls.Add(this.label6);
			this.groupBox1.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.groupBox1.Location = new System.Drawing.Point(8, 72);
			this.groupBox1.Name = "groupBox1";
			this.groupBox1.Size = new System.Drawing.Size(312, 216);
			this.groupBox1.TabIndex = 12;
			this.groupBox1.TabStop = false;
			this.groupBox1.Text = "Date Format";
			// 
			// label11
			// 
			this.label11.Location = new System.Drawing.Point(32, 176);
			this.label11.Name = "label11";
			this.label11.Size = new System.Drawing.Size(208, 32);
			this.label11.TabIndex = 12;
			this.label11.Text = "Optional text can be placed between apostrophes (\' \')";
			// 
			// label8
			// 
			this.label8.Location = new System.Drawing.Point(32, 112);
			this.label8.Name = "label8";
			this.label8.Size = new System.Drawing.Size(208, 23);
			this.label8.TabIndex = 9;
			this.label8.Text = "M = numeric month without prefixed zero";
			// 
			// label5
			// 
			this.label5.Location = new System.Drawing.Point(32, 56);
			this.label5.Name = "label5";
			this.label5.Size = new System.Drawing.Size(208, 23);
			this.label5.TabIndex = 6;
			this.label5.Text = "D = numeric day without prefixed zero";
			// 
			// label4
			// 
			this.label4.Location = new System.Drawing.Point(32, 40);
			this.label4.Name = "label4";
			this.label4.Size = new System.Drawing.Size(208, 23);
			this.label4.TabIndex = 5;
			this.label4.Text = "DD = numeric day with prefixed zero";
			// 
			// label10
			// 
			this.label10.Location = new System.Drawing.Point(32, 152);
			this.label10.Name = "label10";
			this.label10.Size = new System.Drawing.Size(208, 23);
			this.label10.TabIndex = 11;
			this.label10.Text = "YY = Year as two-digit number";
			// 
			// label3
			// 
			this.label3.Location = new System.Drawing.Point(32, 24);
			this.label3.Name = "label3";
			this.label3.Size = new System.Drawing.Size(208, 16);
			this.label3.TabIndex = 4;
			this.label3.Text = "DDDD = Day of week as string";
			// 
			// label9
			// 
			this.label9.Location = new System.Drawing.Point(32, 136);
			this.label9.Name = "label9";
			this.label9.Size = new System.Drawing.Size(208, 23);
			this.label9.TabIndex = 10;
			this.label9.Text = "YYYY = Year as four-digit number";
			// 
			// label7
			// 
			this.label7.Location = new System.Drawing.Point(32, 96);
			this.label7.Name = "label7";
			this.label7.Size = new System.Drawing.Size(208, 23);
			this.label7.TabIndex = 8;
			this.label7.Text = "MM = numeric month with prefixed zero";
			// 
			// label6
			// 
			this.label6.Location = new System.Drawing.Point(32, 80);
			this.label6.Name = "label6";
			this.label6.Size = new System.Drawing.Size(208, 23);
			this.label6.TabIndex = 7;
			this.label6.Text = "MMMM = Month as string";
			// 
			// textBoxDateShort
			// 
			this.textBoxDateShort.Location = new System.Drawing.Point(96, 48);
			this.textBoxDateShort.Name = "textBoxDateShort";
			this.textBoxDateShort.Size = new System.Drawing.Size(224, 20);
			this.textBoxDateShort.TabIndex = 3;
			this.textBoxDateShort.Text = "";
			// 
			// textBoxDateLong
			// 
			this.textBoxDateLong.Location = new System.Drawing.Point(96, 16);
			this.textBoxDateLong.Name = "textBoxDateLong";
			this.textBoxDateLong.Size = new System.Drawing.Size(224, 20);
			this.textBoxDateLong.TabIndex = 1;
			this.textBoxDateLong.Text = "";
			// 
			// label2
			// 
			this.label2.Location = new System.Drawing.Point(8, 48);
			this.label2.Name = "label2";
			this.label2.Size = new System.Drawing.Size(80, 23);
			this.label2.TabIndex = 2;
			this.label2.Text = "Date Short:";
			// 
			// label1
			// 
			this.label1.Location = new System.Drawing.Point(8, 16);
			this.label1.Name = "label1";
			this.label1.Size = new System.Drawing.Size(80, 23);
			this.label1.TabIndex = 0;
			this.label1.Text = "Date Long:";
			// 
			// tabPage2
			// 
			this.tabPage2.BackColor = System.Drawing.SystemColors.ControlLightLight;
			this.tabPage2.Controls.Add(this.groupBox2);
			this.tabPage2.Controls.Add(this.label14);
			this.tabPage2.Controls.Add(this.label13);
			this.tabPage2.Controls.Add(this.label12);
			this.tabPage2.Controls.Add(this.textBoxAM);
			this.tabPage2.Controls.Add(this.textBoxPM);
			this.tabPage2.Controls.Add(this.textBoxTime);
			this.tabPage2.Location = new System.Drawing.Point(4, 22);
			this.tabPage2.Name = "tabPage2";
			this.tabPage2.Size = new System.Drawing.Size(336, 294);
			this.tabPage2.TabIndex = 1;
			this.tabPage2.Text = "Time";
			// 
			// groupBox2
			// 
			this.groupBox2.Controls.Add(this.label20);
			this.groupBox2.Controls.Add(this.label19);
			this.groupBox2.Controls.Add(this.label18);
			this.groupBox2.Controls.Add(this.label17);
			this.groupBox2.Controls.Add(this.label15);
			this.groupBox2.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.groupBox2.Location = new System.Drawing.Point(8, 112);
			this.groupBox2.Name = "groupBox2";
			this.groupBox2.Size = new System.Drawing.Size(320, 176);
			this.groupBox2.TabIndex = 6;
			this.groupBox2.TabStop = false;
			this.groupBox2.Text = "Time Format";
			// 
			// label20
			// 
			this.label20.Location = new System.Drawing.Point(16, 128);
			this.label20.Name = "label20";
			this.label20.Size = new System.Drawing.Size(232, 23);
			this.label20.TabIndex = 5;
			this.label20.Text = "hh, mm, ss = with prefixed zero";
			// 
			// label19
			// 
			this.label19.Location = new System.Drawing.Point(16, 104);
			this.label19.Name = "label19";
			this.label19.Size = new System.Drawing.Size(232, 23);
			this.label19.TabIndex = 4;
			this.label19.Text = "h, m, s = without prefixed zero";
			// 
			// label18
			// 
			this.label18.Location = new System.Drawing.Point(16, 80);
			this.label18.Name = "label18";
			this.label18.TabIndex = 3;
			this.label18.Text = "H=24 hour clock";
			// 
			// label17
			// 
			this.label17.Location = new System.Drawing.Point(16, 56);
			this.label17.Name = "label17";
			this.label17.TabIndex = 2;
			this.label17.Text = "h=12 hour clock";
			// 
			// label15
			// 
			this.label15.Location = new System.Drawing.Point(16, 32);
			this.label15.Name = "label15";
			this.label15.Size = new System.Drawing.Size(280, 23);
			this.label15.TabIndex = 0;
			this.label15.Text = "h=hour, m=minute, s=second, x=meridiem symbol";
			// 
			// label14
			// 
			this.label14.Location = new System.Drawing.Point(8, 80);
			this.label14.Name = "label14";
			this.label14.Size = new System.Drawing.Size(72, 23);
			this.label14.TabIndex = 5;
			this.label14.Text = "AM symbol:";
			// 
			// label13
			// 
			this.label13.Location = new System.Drawing.Point(8, 48);
			this.label13.Name = "label13";
			this.label13.Size = new System.Drawing.Size(72, 23);
			this.label13.TabIndex = 3;
			this.label13.Text = "PM symbol:";
			// 
			// label12
			// 
			this.label12.Location = new System.Drawing.Point(8, 16);
			this.label12.Name = "label12";
			this.label12.Size = new System.Drawing.Size(72, 23);
			this.label12.TabIndex = 1;
			this.label12.Text = "Time:";
			// 
			// textBoxAM
			// 
			this.textBoxAM.Location = new System.Drawing.Point(88, 80);
			this.textBoxAM.Name = "textBoxAM";
			this.textBoxAM.Size = new System.Drawing.Size(72, 20);
			this.textBoxAM.TabIndex = 6;
			this.textBoxAM.Text = "";
			// 
			// textBoxPM
			// 
			this.textBoxPM.Location = new System.Drawing.Point(88, 48);
			this.textBoxPM.Name = "textBoxPM";
			this.textBoxPM.Size = new System.Drawing.Size(72, 20);
			this.textBoxPM.TabIndex = 4;
			this.textBoxPM.Text = "";
			// 
			// textBoxTime
			// 
			this.textBoxTime.Location = new System.Drawing.Point(88, 16);
			this.textBoxTime.Name = "textBoxTime";
			this.textBoxTime.Size = new System.Drawing.Size(136, 20);
			this.textBoxTime.TabIndex = 2;
			this.textBoxTime.Text = "";
			// 
			// tabPage3
			// 
			this.tabPage3.BackColor = System.Drawing.SystemColors.ControlLightLight;
			this.tabPage3.Controls.Add(this.comboBoxTempUnit);
			this.tabPage3.Controls.Add(this.label21);
			this.tabPage3.Controls.Add(this.comboBoxSpeedUnit);
			this.tabPage3.Controls.Add(this.label16);
			this.tabPage3.Location = new System.Drawing.Point(4, 22);
			this.tabPage3.Name = "tabPage3";
			this.tabPage3.Size = new System.Drawing.Size(336, 294);
			this.tabPage3.TabIndex = 2;
			this.tabPage3.Text = "Other";
			// 
			// comboBoxTempUnit
			// 
			this.comboBoxTempUnit.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.comboBoxTempUnit.Location = new System.Drawing.Point(136, 112);
			this.comboBoxTempUnit.Name = "comboBoxTempUnit";
			this.comboBoxTempUnit.Size = new System.Drawing.Size(121, 21);
			this.comboBoxTempUnit.TabIndex = 4;
			// 
			// label21
			// 
			this.label21.Location = new System.Drawing.Point(24, 112);
			this.label21.Name = "label21";
			this.label21.TabIndex = 3;
			this.label21.Text = "Temperature Unit:";
			// 
			// comboBoxSpeedUnit
			// 
			this.comboBoxSpeedUnit.DropDownStyle = System.Windows.Forms.ComboBoxStyle.DropDownList;
			this.comboBoxSpeedUnit.Location = new System.Drawing.Point(136, 48);
			this.comboBoxSpeedUnit.Name = "comboBoxSpeedUnit";
			this.comboBoxSpeedUnit.Size = new System.Drawing.Size(121, 21);
			this.comboBoxSpeedUnit.TabIndex = 1;
			// 
			// label16
			// 
			this.label16.Location = new System.Drawing.Point(24, 48);
			this.label16.Name = "label16";
			this.label16.TabIndex = 0;
			this.label16.Text = "Speed Unit:";
			// 
			// RegionForm
			// 
			this.AcceptButton = this.buttonOK;
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.CancelButton = this.buttonCancel;
			this.ClientSize = new System.Drawing.Size(362, 368);
			this.Controls.Add(this.tabControl1);
			this.Controls.Add(this.buttonCancel);
			this.Controls.Add(this.buttonOK);
			this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedDialog;
			this.MaximizeBox = false;
			this.MinimizeBox = false;
			this.Name = "RegionForm";
			this.ShowInTaskbar = false;
			this.StartPosition = System.Windows.Forms.FormStartPosition.CenterParent;
			this.Text = "Region Properties";
			this.Load += new System.EventHandler(this.RegionForm_Load);
			this.tabControl1.ResumeLayout(false);
			this.tabPage1.ResumeLayout(false);
			this.groupBox1.ResumeLayout(false);
			this.tabPage2.ResumeLayout(false);
			this.groupBox2.ResumeLayout(false);
			this.tabPage3.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		#region Form initialization

		/// <summary>
		/// Updates controls with its values from a region
		/// </summary>
		private void RegionForm_Load(object sender, System.EventArgs e)
		{
			// setup abbreviation array for speed units
			speedUnits.Add(new Unit("kmh", "km/h"));
			speedUnits.Add(new Unit("mpmin", "m/min"));
			speedUnits.Add(new Unit("mps", "m/s"));
			speedUnits.Add(new Unit("fth", "foot/h"));
			speedUnits.Add(new Unit("ftm", "foot/min"));
			speedUnits.Add(new Unit("fts", "foot/s"));
			speedUnits.Add(new Unit("mph", "mph"));
			speedUnits.Add(new Unit("kts", "knots"));
			speedUnits.Add(new Unit("beaufort", "beaufort"));
			speedUnits.Add(new Unit("inchs", "inch/s"));
			speedUnits.Add(new Unit("yards", "yard/s"));
			speedUnits.Add(new Unit("fpf", "Furlong/Fortnight"));

			// fill combobox with long names
			foreach (Unit speedUnit in speedUnits)
			{
				comboBoxSpeedUnit.Items.Add(speedUnit.NameLong);
			}

			// select the combobox item based on the abbreviation
			foreach (Unit speedUnit in speedUnits)
			{
				if (speedUnit.NameShort==region.SpeedUnit)
				{
					comboBoxSpeedUnit.Text=speedUnit.NameLong;
				}
			}

			// setup abbreviation array for temp units
			tempUnits.Add(new Unit("F", "°F"));
			tempUnits.Add(new Unit("K", "K"));
			tempUnits.Add(new Unit("C", "°C"));
			tempUnits.Add(new Unit("Re", "°Ré"));
			tempUnits.Add(new Unit("Ra", "°Ra"));
			tempUnits.Add(new Unit("Ro", "°Rø"));
			tempUnits.Add(new Unit("De", "°De"));
			tempUnits.Add(new Unit("N", "°N"));

			// fill combobox with long names
			foreach (Unit tempUnit in tempUnits)
			{
				comboBoxTempUnit.Items.Add(tempUnit.NameLong);
			}

			// select the combobox item based on the abbreviation
			foreach (Unit tempUnit in tempUnits)
			{
				if (tempUnit.NameShort==region.TempUnit)
				{
					comboBoxTempUnit.Text=tempUnit.NameLong;
				}
			}

			// fill in the rest of the region info
			textBoxDateShort.Text=region.DateShort;
			textBoxDateLong.Text=region.DateLong;
			textBoxTime.Text=region.Time;
			textBoxAM.Text=region.SymbolAM;
			textBoxPM.Text=region.SymbolPM;
		}

		#endregion

		#region Control events

		/// <summary>
		/// Updates the region with the values of the controls
		/// </summary>
		private void buttonOK_Click(object sender, System.EventArgs e)
		{
			region.DateShort=textBoxDateShort.Text;
			region.DateLong=textBoxDateLong.Text;
			region.Time=textBoxTime.Text;
			region.SymbolAM=textBoxAM.Text;
			region.SymbolPM=textBoxPM.Text;

			// tranlate the speed unit text of the combobox to 
			// the abbreviation used in the xml file
			foreach (Unit speedUnit in speedUnits)
			{
				if (speedUnit.NameLong==comboBoxSpeedUnit.Text)
				{
					region.SpeedUnit=speedUnit.NameShort;
				}
			}

			// tranlate the temp unit text of the combobox to 
			// the abbreviation used in the xml file
			foreach (Unit tempUnit in tempUnits)
			{
				if (tempUnit.NameLong==comboBoxTempUnit.Text)
				{
					region.TempUnit=tempUnit.NameShort;
				}
			}
		}

		#endregion

		#region Properties

		/// <summary>
		/// Set/get the region to be edited
		/// </summary>
		public LanguageInfo.Region RegionInfo
		{
			get { return region; }
			set { region=value; }
		}

		#endregion
	}
}
