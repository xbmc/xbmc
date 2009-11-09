// MediaInfoDLL - All info about media files
// This software is provided 'as-is', without any express or implied
// warranty.  In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
//
// Microsoft Visual C# example
//
// To make this example working, you must put MediaInfo.Dll and Example.ogg
// in the "./Bin/__ConfigurationName__" folder
// and add MediaInfoDll.cs to your project
//
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++

using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using System.Text;
using MediaInfoLib;

namespace MediaInfoLib_MSCS
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class Form1 : System.Windows.Forms.Form
	{
		private System.Windows.Forms.RichTextBox richTextBox1;
		/// <summary>
		/// Required designer variable.
		/// </summary>
		private System.ComponentModel.Container components = null;

		public Form1()
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
		/// Méthode requise pour la prise en charge du concepteur - ne modifiez pas
		/// le contenu de cette méthode avec l'éditeur de code.
		/// </summary>
		private void InitializeComponent()
		{
            this.richTextBox1 = new System.Windows.Forms.RichTextBox();
            this.SuspendLayout();
            // 
            // richTextBox1
            // 
            this.richTextBox1.Location = new System.Drawing.Point(0, 0);
            this.richTextBox1.Name = "richTextBox1";
            this.richTextBox1.Size = new System.Drawing.Size(768, 512);
            this.richTextBox1.TabIndex = 0;
            this.richTextBox1.Text = "";
            // 
            // Form1
            // 
            this.ClientSize = new System.Drawing.Size(770, 514);
            this.Controls.Add(this.richTextBox1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.Fixed3D;
            this.Name = "Form1";
            this.StartPosition = System.Windows.Forms.FormStartPosition.CenterScreen;
            this.Text = "How to use MediaInfo.Dll";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.ResumeLayout(false);

		}
		#endregion

		/// <summary>
		/// The main entry point for the application.
		/// </summary>


		[STAThread]
		static void Main() 
		{
			Application.Run(new Form1());
		}

		private void Form1_Load(object sender, System.EventArgs e)
		{
			//Test if version of DLL is compatible : 3rd argument is "version of DLL tested;Your application name;Your application version"
            String ToDisplay;
            MediaInfo MI = new MediaInfo();

            ToDisplay = MI.Option("Info_Version", "0.7.0.0;MediaInfoDLL_Example_CS;0.7.0.0");
			if (ToDisplay.Length == 0)
			{
                richTextBox1.Text = "MediaInfo.Dll: this version of the DLL is not compatible";
				return;
			}

			//Information about MediaInfo
			ToDisplay += "\r\n\r\nInfo_Parameters\r\n";
			ToDisplay += MI.Option("Info_Parameters");

			ToDisplay += "\r\n\r\nInfo_Capacities\r\n";
            ToDisplay += MI.Option("Info_Capacities");

			ToDisplay += "\r\n\r\nInfo_Codecs\r\n";
            ToDisplay += MI.Option("Info_Codecs");

			//An example of how to use the library
			ToDisplay += "\r\n\r\nOpen\r\n";
			MI.Open("Example.ogg");

			ToDisplay += "\r\n\r\nInform with Complete=false\r\n";
			MI.Option("Complete");
			ToDisplay += MI.Inform();

            ToDisplay += "\r\n\r\nInform with Complete=true\r\n";
			MI.Option("Complete", "1");
			ToDisplay += MI.Inform();

            ToDisplay += "\r\n\r\nCustom Inform\r\n";
            MI.Option("Inform", "General;File size is %FileSize% bytes");
            ToDisplay += MI.Inform();

            ToDisplay += "\r\n\r\nGet with Stream=General and Parameter='FileSize'\r\n";
            ToDisplay += MI.Get(0, 0, "FileSize");

            ToDisplay += "\r\n\r\nGet with Stream=General and Parameter=46\r\n";
            ToDisplay += MI.Get(0, 0, 46);

            ToDisplay += "\r\n\r\nCount_Get with StreamKind=Stream_Audio\r\n";
            ToDisplay += MI.Count_Get(StreamKind.Audio);

            ToDisplay += "\r\n\r\nGet with Stream=General and Parameter='AudioCount'\r\n";
            ToDisplay += MI.Get(StreamKind.General, 0, "AudioCount");

            ToDisplay += "\r\n\r\nGet with Stream=Audio and Parameter='StreamCount'\r\n";
            ToDisplay += MI.Get(StreamKind.Audio, 0, "StreamCount");

            ToDisplay += "\r\n\r\nClose\r\n";
            MI.Close();

			//Displaying the text
			richTextBox1.Text = ToDisplay;
		}
	}
}
