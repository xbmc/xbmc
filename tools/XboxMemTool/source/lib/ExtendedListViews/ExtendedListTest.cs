using System;
using System.Drawing;
using System.Collections;
using System.ComponentModel;
using System.Windows.Forms;
using System.Data;
using Lyquidity.Controls.ExtendedListViews;

namespace SampleApplication
{
	/// <summary>
	/// Summary description for Form1.
	/// </summary>
	public class frmExtendedListTest : System.Windows.Forms.Form
	{
		private System.Windows.Forms.Panel panel1;
		private System.Windows.Forms.Button btnExit;
		private System.Windows.Forms.Panel panel2;
		private System.Windows.Forms.TabControl tabControl1;
		private System.Windows.Forms.TabPage tabPage1;
		private System.Windows.Forms.TabPage tabPage2;
		private ContainerListView containerListView1;
		private TreeListView treeListView1;
		private System.Windows.Forms.ImageList listImages;
		private System.Windows.Forms.ProgressBar progressBar1;
		private System.Windows.Forms.ProgressBar progressBar2;
		private System.Windows.Forms.ProgressBar progressBar3;
		private System.Windows.Forms.ProgressBar progressBar4;
		private System.Windows.Forms.ProgressBar progressBar5;
		private System.Windows.Forms.ContextMenu contextCList;
		private System.Windows.Forms.MenuItem menuItem1;
		private System.Windows.Forms.MenuItem menuItem2;
		private System.Windows.Forms.MenuItem menuItem3;
		private System.Windows.Forms.ContextMenu contextTList;
		private System.Windows.Forms.MenuItem menuItem4;
		private System.Windows.Forms.MenuItem menuItem5;
		private System.Windows.Forms.MenuItem menuItem6;
		private System.Windows.Forms.MenuItem menuItem7;
		private System.Windows.Forms.MenuItem menuItem8;
		private System.Windows.Forms.MenuItem menuItem9;
		private System.Windows.Forms.MenuItem menuItem10;
		private System.Windows.Forms.TabPage tabPage3;
		private TreeListView treeListView2;
		private System.Windows.Forms.Button btnAddItems;
		private System.Windows.Forms.Label lblItemsIns;
		private System.Windows.Forms.Button btnClearHi;
		private System.Windows.Forms.Button button1;
		private System.ComponentModel.IContainer components;

		public frmExtendedListTest()
		{
			//
			// Required for Windows Form Designer support
			//
			InitializeComponent();

			// Add third tier of nodes to tree
			TreeListNode tln = new TreeListNode();
			tln.Text = "Sample.wav";
			tln.SubItems.Add("Audio File");
			tln.SubItems.Add("644074 bytes");
			tln.ImageIndex = 1;
			treeListView1.Nodes[1].Nodes[0].Nodes.Add(tln);

			tln = new TreeListNode();
			tln.Text = "Sample.mp3";
			tln.SubItems.Add("Audio File");
			tln.SubItems.Add("21074 bytes");
			tln.ImageIndex = 1;
			treeListView1.Nodes[1].Nodes[1].Nodes.Add(tln);

			treeListView1.Nodes[2].Nodes[0].Collapse();
			tln = new TreeListNode();
			tln.Text = "Readme.txt";
			tln.SubItems.Add("Text File");
			tln.SubItems.Add("24030 bytes");
			tln.ImageIndex = 2;
			treeListView1.Nodes[2].Nodes[0].Nodes.Add(tln);

			tln = new TreeListNode();
			tln.Text = "Readme";
			tln.SubItems.Add("File");
			tln.SubItems.Add("12893 bytes");
			tln.ImageIndex = 2;
			treeListView1.Nodes[2].Nodes[0].Nodes.Add(tln);

			tln = new TreeListNode();
			tln.Text = "Readme.doc";
			tln.SubItems.Add("Document File");
			tln.SubItems.Add("96069 bytes");
			tln.ImageIndex = 2;
			treeListView1.Nodes[2].Nodes[1].Nodes.Add(tln);

			treeListView1.Nodes[3].Nodes[0].Collapse();
			tln = new TreeListNode();
			tln.Text = "Sample.jpg";
			tln.SubItems.Add("Jpeg Image");
			tln.SubItems.Add("10100 bytes");
			tln.ImageIndex = 3;
			treeListView1.Nodes[3].Nodes[0].Nodes.Add(tln);

			tln = new TreeListNode();
			tln.Text = "Sample2.jpg";
			tln.SubItems.Add("Jpeg Image");
			tln.SubItems.Add("8842 bytes");
			tln.ImageIndex = 3;
			treeListView1.Nodes[3].Nodes[0].Nodes.Add(tln);

			tln = new TreeListNode();
			tln.Text = "Sample.gif";
			tln.SubItems.Add("GIF Image");
			tln.SubItems.Add("6423 bytes");
			tln.ImageIndex = 3;
			treeListView1.Nodes[3].Nodes[1].Nodes.Add(tln);

			tln = new TreeListNode();
			tln.Text = "Sample.png";
			tln.SubItems.Add("PNG Image");
			tln.SubItems.Add("89251 bytes");
			tln.ImageIndex = 3;
			treeListView1.Nodes[3].Nodes[2].Nodes.Add(tln);

			tln = new TreeListNode();
			tln.Text = "Sample2.png";
			tln.SubItems.Add("PNG Image");
			tln.SubItems.Add("104658 bytes");
			tln.ImageIndex = 3;
			treeListView1.Nodes[3].Nodes[2].Nodes.Add(tln);

			tln = new TreeListNode();
			tln.Text = "Sample3.png";
			tln.SubItems.Add("PNG Image");
			tln.SubItems.Add("320901 bytes");
			tln.ImageIndex = 3;
			treeListView1.Nodes[3].Nodes[2].Nodes.Add(tln);

			tln = new TreeListNode();
			tln.Text = "X-Files - 8.16 - Vienen.mpg";
			tln.SubItems.Add("MPEG Video");
			tln.SubItems.Add("161895063 bytes");
			tln.ImageIndex = 4;
			treeListView1.Nodes[4].Nodes[0].Nodes.Add(tln);

			tln = new TreeListNode();
			tln.Text = "X-Files - 9.03 - Daemonicus.avi";
			tln.SubItems.Add("AVI Video");
			tln.SubItems.Add("124636969 bytes");
			tln.ImageIndex = 4;
			treeListView1.Nodes[4].Nodes[0].Nodes.Add(tln);

			tln = new TreeListNode();
			tln.Text = "Stargate SG-1 6.05.4.vob";
			tln.SubItems.Add("VOB File");
			tln.SubItems.Add("869854 bytes");
			tln.ImageIndex = 4;
			treeListView1.Nodes[4].Nodes[1].Nodes.Add(tln);
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
			this.components = new System.ComponentModel.Container();
			Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader toggleColumnHeader1 = new Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader();
			System.Resources.ResourceManager resources = new System.Resources.ResourceManager(typeof(frmExtendedListTest));
			Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader toggleColumnHeader2 = new Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader();
			Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader toggleColumnHeader3 = new Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader();
			Lyquidity.Controls.ExtendedListViews.ContainerListViewItem containerListViewItem1 = new Lyquidity.Controls.ExtendedListViews.ContainerListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem containerSubListViewItem1 = new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem containerSubListViewItem2 = new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerListViewItem containerListViewItem2 = new Lyquidity.Controls.ExtendedListViews.ContainerListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem containerSubListViewItem3 = new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem containerSubListViewItem4 = new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerListViewItem containerListViewItem3 = new Lyquidity.Controls.ExtendedListViews.ContainerListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem containerSubListViewItem5 = new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem containerSubListViewItem6 = new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerListViewItem containerListViewItem4 = new Lyquidity.Controls.ExtendedListViews.ContainerListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem containerSubListViewItem7 = new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem containerSubListViewItem8 = new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerListViewItem containerListViewItem5 = new Lyquidity.Controls.ExtendedListViews.ContainerListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem containerSubListViewItem9 = new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem();
			Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem containerSubListViewItem10 = new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem();
			Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader toggleColumnHeader4 = new Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader();
			Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader toggleColumnHeader5 = new Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader();
			Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader toggleColumnHeader6 = new Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode1 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode2 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode3 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode4 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode5 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode6 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode7 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode8 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode9 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode10 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode11 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode12 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode13 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode14 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode15 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode16 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.TreeListNode treeListNode17 = new Lyquidity.Controls.ExtendedListViews.TreeListNode();
			Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader toggleColumnHeader7 = new Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader();
			Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader toggleColumnHeader8 = new Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader();
			Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader toggleColumnHeader9 = new Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader();
			Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader toggleColumnHeader10 = new Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader();
			this.progressBar4 = new System.Windows.Forms.ProgressBar();
			this.progressBar2 = new System.Windows.Forms.ProgressBar();
			this.progressBar5 = new System.Windows.Forms.ProgressBar();
			this.progressBar3 = new System.Windows.Forms.ProgressBar();
			this.progressBar1 = new System.Windows.Forms.ProgressBar();
			this.panel1 = new System.Windows.Forms.Panel();
			this.btnExit = new System.Windows.Forms.Button();
			this.panel2 = new System.Windows.Forms.Panel();
			this.tabControl1 = new System.Windows.Forms.TabControl();
			this.tabPage1 = new System.Windows.Forms.TabPage();
			this.containerListView1 = new Lyquidity.Controls.ExtendedListViews.ContainerListView();
			this.contextCList = new System.Windows.Forms.ContextMenu();
			this.menuItem1 = new System.Windows.Forms.MenuItem();
			this.menuItem2 = new System.Windows.Forms.MenuItem();
			this.menuItem3 = new System.Windows.Forms.MenuItem();
			this.menuItem7 = new System.Windows.Forms.MenuItem();
			this.menuItem8 = new System.Windows.Forms.MenuItem();
			this.listImages = new System.Windows.Forms.ImageList(this.components);
			this.tabPage2 = new System.Windows.Forms.TabPage();
			this.treeListView1 = new Lyquidity.Controls.ExtendedListViews.TreeListView();
			this.contextTList = new System.Windows.Forms.ContextMenu();
			this.menuItem4 = new System.Windows.Forms.MenuItem();
			this.menuItem5 = new System.Windows.Forms.MenuItem();
			this.menuItem6 = new System.Windows.Forms.MenuItem();
			this.menuItem9 = new System.Windows.Forms.MenuItem();
			this.menuItem10 = new System.Windows.Forms.MenuItem();
			this.tabPage3 = new System.Windows.Forms.TabPage();
			this.btnClearHi = new System.Windows.Forms.Button();
			this.lblItemsIns = new System.Windows.Forms.Label();
			this.btnAddItems = new System.Windows.Forms.Button();
			this.treeListView2 = new Lyquidity.Controls.ExtendedListViews.TreeListView();
			this.button1 = new System.Windows.Forms.Button();
			this.panel1.SuspendLayout();
			this.panel2.SuspendLayout();
			this.tabControl1.SuspendLayout();
			this.tabPage1.SuspendLayout();
			this.containerListView1.SuspendLayout();
			this.tabPage2.SuspendLayout();
			this.tabPage3.SuspendLayout();
			this.SuspendLayout();
			// 
			// progressBar4
			// 
			this.progressBar4.Location = new System.Drawing.Point(204, 24);
			this.progressBar4.Name = "progressBar4";
			this.progressBar4.Size = new System.Drawing.Size(174, 14);
			this.progressBar4.TabIndex = 4;
			this.progressBar4.Value = 8;
			// 
			// progressBar2
			// 
			this.progressBar2.Location = new System.Drawing.Point(204, 42);
			this.progressBar2.Name = "progressBar2";
			this.progressBar2.Size = new System.Drawing.Size(174, 14);
			this.progressBar2.TabIndex = 2;
			this.progressBar2.Value = 82;
			// 
			// progressBar5
			// 
			this.progressBar5.Location = new System.Drawing.Point(204, 60);
			this.progressBar5.Name = "progressBar5";
			this.progressBar5.Size = new System.Drawing.Size(174, 14);
			this.progressBar5.TabIndex = 5;
			this.progressBar5.Value = 2;
			// 
			// progressBar3
			// 
			this.progressBar3.Location = new System.Drawing.Point(204, 78);
			this.progressBar3.Name = "progressBar3";
			this.progressBar3.Size = new System.Drawing.Size(174, 14);
			this.progressBar3.TabIndex = 3;
			this.progressBar3.Value = 11;
			// 
			// progressBar1
			// 
			this.progressBar1.Location = new System.Drawing.Point(204, 96);
			this.progressBar1.Name = "progressBar1";
			this.progressBar1.Size = new System.Drawing.Size(174, 14);
			this.progressBar1.TabIndex = 1;
			this.progressBar1.Value = 100;
			// 
			// panel1
			// 
			this.panel1.Controls.Add(this.button1);
			this.panel1.Controls.Add(this.btnExit);
			this.panel1.Dock = System.Windows.Forms.DockStyle.Bottom;
			this.panel1.Location = new System.Drawing.Point(0, 366);
			this.panel1.Name = "panel1";
			this.panel1.Size = new System.Drawing.Size(408, 40);
			this.panel1.TabIndex = 0;
			// 
			// btnExit
			// 
			this.btnExit.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.btnExit.Location = new System.Drawing.Point(8, 8);
			this.btnExit.Name = "btnExit";
			this.btnExit.TabIndex = 0;
			this.btnExit.Text = "Exit";
			this.btnExit.Click += new System.EventHandler(this.btnExit_Click);
			// 
			// panel2
			// 
			this.panel2.Controls.Add(this.tabControl1);
			this.panel2.Dock = System.Windows.Forms.DockStyle.Fill;
			this.panel2.DockPadding.All = 8;
			this.panel2.Location = new System.Drawing.Point(0, 0);
			this.panel2.Name = "panel2";
			this.panel2.Size = new System.Drawing.Size(408, 366);
			this.panel2.TabIndex = 1;
			// 
			// tabControl1
			// 
			this.tabControl1.Controls.Add(this.tabPage1);
			this.tabControl1.Controls.Add(this.tabPage2);
			this.tabControl1.Controls.Add(this.tabPage3);
			this.tabControl1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.tabControl1.Location = new System.Drawing.Point(8, 8);
			this.tabControl1.Name = "tabControl1";
			this.tabControl1.SelectedIndex = 0;
			this.tabControl1.Size = new System.Drawing.Size(392, 350);
			this.tabControl1.TabIndex = 0;
			// 
			// tabPage1
			// 
			this.tabPage1.Controls.Add(this.containerListView1);
			this.tabPage1.Location = new System.Drawing.Point(4, 22);
			this.tabPage1.Name = "tabPage1";
			this.tabPage1.Size = new System.Drawing.Size(384, 324);
			this.tabPage1.TabIndex = 0;
			this.tabPage1.Text = "ContainerListView";
			// 
			// containerListView1
			// 
			this.containerListView1.BackColor = System.Drawing.SystemColors.Window;
			toggleColumnHeader1.Hovered = false;
			toggleColumnHeader1.Image = ((System.Drawing.Bitmap)(resources.GetObject("toggleColumnHeader1.Image")));
			toggleColumnHeader1.Index = 0;
			toggleColumnHeader1.Pressed = false;
			toggleColumnHeader1.ScaleStyle = Lyquidity.Controls.ExtendedListViews.ColumnScaleStyle.Slide;
			toggleColumnHeader1.Selected = false;
			toggleColumnHeader1.Text = "Type";
			toggleColumnHeader1.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
			toggleColumnHeader1.Visible = true;
			toggleColumnHeader1.Width = 100;
			toggleColumnHeader2.Hovered = false;
			toggleColumnHeader2.Image = null;
			toggleColumnHeader2.Index = 0;
			toggleColumnHeader2.Pressed = false;
			toggleColumnHeader2.ScaleStyle = Lyquidity.Controls.ExtendedListViews.ColumnScaleStyle.Slide;
			toggleColumnHeader2.Selected = false;
			toggleColumnHeader2.Text = "Name";
			toggleColumnHeader2.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
			toggleColumnHeader2.Visible = true;
			toggleColumnHeader2.Width = 100;
			toggleColumnHeader3.Hovered = false;
			toggleColumnHeader3.Image = null;
			toggleColumnHeader3.Index = 0;
			toggleColumnHeader3.Pressed = false;
			toggleColumnHeader3.ScaleStyle = Lyquidity.Controls.ExtendedListViews.ColumnScaleStyle.Slide;
			toggleColumnHeader3.Selected = false;
			toggleColumnHeader3.Text = "Size";
			toggleColumnHeader3.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
			toggleColumnHeader3.Visible = true;
			toggleColumnHeader3.Width = 180;
			this.containerListView1.Columns.AddRange(new Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader[] {
																													   toggleColumnHeader1,
																													   toggleColumnHeader2,
																													   toggleColumnHeader3});
			this.containerListView1.ColumnSortColor = System.Drawing.Color.Gainsboro;
			this.containerListView1.ColumnTrackColor = System.Drawing.Color.WhiteSmoke;
			this.containerListView1.ContextMenu = this.contextCList;
			this.containerListView1.Controls.Add(this.progressBar4);
			this.containerListView1.Controls.Add(this.progressBar2);
			this.containerListView1.Controls.Add(this.progressBar5);
			this.containerListView1.Controls.Add(this.progressBar3);
			this.containerListView1.Controls.Add(this.progressBar1);
			this.containerListView1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.containerListView1.GridLineColor = System.Drawing.Color.WhiteSmoke;
			this.containerListView1.GridLines = true;
			this.containerListView1.HeaderMenu = null;
			this.containerListView1.ItemMenu = null;
			containerListViewItem1.BackColor = System.Drawing.Color.Empty;
			containerListViewItem1.Checked = false;
			containerListViewItem1.Focused = false;
			containerListViewItem1.Font = null;
			containerListViewItem1.ForeColor = System.Drawing.Color.Empty;
			containerListViewItem1.Hovered = false;
			containerListViewItem1.ImageIndex = 0;
			containerListViewItem1.Index = 0;
			containerListViewItem1.Selected = false;
			containerListViewItem1.StateImageIndex = 0;
			containerSubListViewItem1.ItemControl = null;
			containerSubListViewItem1.Text = "Test.exe";
			containerSubListViewItem2.ItemControl = this.progressBar4;
			containerSubListViewItem2.Text = "57344 bytes";
			containerListViewItem1.SubItems.AddRange(new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem[] {
																															 containerSubListViewItem1,
																															 containerSubListViewItem2});
			containerListViewItem1.Tag = null;
			containerListViewItem1.Text = "Application";
			containerListViewItem1.UseItemStyleForSubItems = false;
			containerListViewItem2.BackColor = System.Drawing.Color.Empty;
			containerListViewItem2.Checked = false;
			containerListViewItem2.Focused = false;
			containerListViewItem2.Font = null;
			containerListViewItem2.ForeColor = System.Drawing.Color.Empty;
			containerListViewItem2.Hovered = false;
			containerListViewItem2.ImageIndex = 1;
			containerListViewItem2.Index = 1;
			containerListViewItem2.Selected = false;
			containerListViewItem2.StateImageIndex = 0;
			containerSubListViewItem3.ItemControl = null;
			containerSubListViewItem3.Text = "Sandra Collins - Tranceport 3.mp3";
			containerSubListViewItem4.ItemControl = this.progressBar2;
			containerSubListViewItem4.Text = "106,226,127 bytes";
			containerListViewItem2.SubItems.AddRange(new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem[] {
																															 containerSubListViewItem3,
																															 containerSubListViewItem4});
			containerListViewItem2.Tag = null;
			containerListViewItem2.Text = "Audio File";
			containerListViewItem2.UseItemStyleForSubItems = false;
			containerListViewItem3.BackColor = System.Drawing.Color.Empty;
			containerListViewItem3.Checked = false;
			containerListViewItem3.Focused = false;
			containerListViewItem3.Font = null;
			containerListViewItem3.ForeColor = System.Drawing.Color.Empty;
			containerListViewItem3.Hovered = false;
			containerListViewItem3.ImageIndex = 2;
			containerListViewItem3.Index = 2;
			containerListViewItem3.Selected = false;
			containerListViewItem3.StateImageIndex = 0;
			containerSubListViewItem5.ItemControl = null;
			containerSubListViewItem5.Text = "Readme.txt";
			containerSubListViewItem6.ItemControl = this.progressBar5;
			containerSubListViewItem6.Text = "980 bytes";
			containerListViewItem3.SubItems.AddRange(new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem[] {
																															 containerSubListViewItem5,
																															 containerSubListViewItem6});
			containerListViewItem3.Tag = null;
			containerListViewItem3.Text = "Document";
			containerListViewItem3.UseItemStyleForSubItems = false;
			containerListViewItem4.BackColor = System.Drawing.Color.Empty;
			containerListViewItem4.Checked = false;
			containerListViewItem4.Focused = false;
			containerListViewItem4.Font = null;
			containerListViewItem4.ForeColor = System.Drawing.Color.Empty;
			containerListViewItem4.Hovered = false;
			containerListViewItem4.ImageIndex = 3;
			containerListViewItem4.Index = 3;
			containerListViewItem4.Selected = false;
			containerListViewItem4.StateImageIndex = 0;
			containerSubListViewItem7.ItemControl = null;
			containerSubListViewItem7.Text = "Inferno.jpg";
			containerSubListViewItem8.ItemControl = this.progressBar3;
			containerSubListViewItem8.Text = "207,770 bytes";
			containerListViewItem4.SubItems.AddRange(new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem[] {
																															 containerSubListViewItem7,
																															 containerSubListViewItem8});
			containerListViewItem4.Tag = null;
			containerListViewItem4.Text = "Image File";
			containerListViewItem4.UseItemStyleForSubItems = false;
			containerListViewItem5.BackColor = System.Drawing.Color.Empty;
			containerListViewItem5.Checked = false;
			containerListViewItem5.Focused = false;
			containerListViewItem5.Font = null;
			containerListViewItem5.ForeColor = System.Drawing.Color.Empty;
			containerListViewItem5.Hovered = false;
			containerListViewItem5.ImageIndex = 4;
			containerListViewItem5.Index = 4;
			containerListViewItem5.Selected = false;
			containerListViewItem5.StateImageIndex = 0;
			containerSubListViewItem9.ItemControl = null;
			containerSubListViewItem9.Text = "X-Files - 8.16 - Vienen.mpg";
			containerSubListViewItem10.ItemControl = this.progressBar1;
			containerSubListViewItem10.Text = "161895023 bytes";
			containerListViewItem5.SubItems.AddRange(new Lyquidity.Controls.ExtendedListViews.ContainerSubListViewItem[] {
																															 containerSubListViewItem9,
																															 containerSubListViewItem10});
			containerListViewItem5.Tag = null;
			containerListViewItem5.Text = "Video File";
			containerListViewItem5.UseItemStyleForSubItems = false;
			this.containerListView1.Items.AddRange(new Lyquidity.Controls.ExtendedListViews.ContainerListViewItem[] {
																														containerListViewItem1,
																														containerListViewItem2,
																														containerListViewItem3,
																														containerListViewItem4,
																														containerListViewItem5});
			this.containerListView1.LabelEdit = false;
			this.containerListView1.Location = new System.Drawing.Point(0, 0);
			this.containerListView1.MultiSelect = true;
			this.containerListView1.Name = "containerListView1";
			this.containerListView1.RowSelectColor = System.Drawing.SystemColors.Highlight;
			this.containerListView1.RowTrackColor = System.Drawing.Color.WhiteSmoke;
			this.containerListView1.Size = new System.Drawing.Size(384, 324);
			this.containerListView1.SmallImageList = this.listImages;
			this.containerListView1.StateImageList = null;
			this.containerListView1.TabIndex = 0;
			this.containerListView1.Text = "containerListView1";
			this.containerListView1.VisualStyles = true;
			this.containerListView1.SelectedIndexChanged += new System.EventHandler(this.containerListView1_SelectedIndexChanged);
			// 
			// contextCList
			// 
			this.contextCList.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																						 this.menuItem1,
																						 this.menuItem2,
																						 this.menuItem3,
																						 this.menuItem7,
																						 this.menuItem8});
			// 
			// menuItem1
			// 
			this.menuItem1.Index = 0;
			this.menuItem1.Text = "Column Tracking";
			this.menuItem1.Click += new System.EventHandler(this.menuItem1_Click);
			// 
			// menuItem2
			// 
			this.menuItem2.Checked = true;
			this.menuItem2.Index = 1;
			this.menuItem2.Text = "Row Tracking";
			this.menuItem2.Click += new System.EventHandler(this.menuItem2_Click);
			// 
			// menuItem3
			// 
			this.menuItem3.Checked = true;
			this.menuItem3.Index = 2;
			this.menuItem3.Text = "Gridlines";
			this.menuItem3.Click += new System.EventHandler(this.menuItem3_Click);
			// 
			// menuItem7
			// 
			this.menuItem7.Index = 3;
			this.menuItem7.Text = "-";
			// 
			// menuItem8
			// 
			this.menuItem8.Checked = true;
			this.menuItem8.Index = 4;
			this.menuItem8.Text = "Use Visual Styles";
			this.menuItem8.Click += new System.EventHandler(this.menuItem8_Click);
			// 
			// listImages
			// 
			this.listImages.ColorDepth = System.Windows.Forms.ColorDepth.Depth32Bit;
			this.listImages.ImageSize = new System.Drawing.Size(16, 16);
			this.listImages.ImageStream = ((System.Windows.Forms.ImageListStreamer)(resources.GetObject("listImages.ImageStream")));
			this.listImages.TransparentColor = System.Drawing.Color.Transparent;
			// 
			// tabPage2
			// 
			this.tabPage2.Controls.Add(this.treeListView1);
			this.tabPage2.Location = new System.Drawing.Point(4, 22);
			this.tabPage2.Name = "tabPage2";
			this.tabPage2.Size = new System.Drawing.Size(384, 324);
			this.tabPage2.TabIndex = 1;
			this.tabPage2.Text = "TreeListView";
			// 
			// treeListView1
			// 
			this.treeListView1.BackColor = System.Drawing.SystemColors.Window;
			toggleColumnHeader4.Hovered = false;
			toggleColumnHeader4.Image = ((System.Drawing.Bitmap)(resources.GetObject("toggleColumnHeader4.Image")));
			toggleColumnHeader4.Index = 0;
			toggleColumnHeader4.Pressed = false;
			toggleColumnHeader4.ScaleStyle = Lyquidity.Controls.ExtendedListViews.ColumnScaleStyle.Slide;
			toggleColumnHeader4.Selected = false;
			toggleColumnHeader4.Text = "Title";
			toggleColumnHeader4.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
			toggleColumnHeader4.Visible = true;
			toggleColumnHeader4.Width = 222;
			toggleColumnHeader5.Hovered = false;
			toggleColumnHeader5.Image = ((System.Drawing.Bitmap)(resources.GetObject("toggleColumnHeader5.Image")));
			toggleColumnHeader5.Index = 0;
			toggleColumnHeader5.Pressed = false;
			toggleColumnHeader5.ScaleStyle = Lyquidity.Controls.ExtendedListViews.ColumnScaleStyle.Slide;
			toggleColumnHeader5.Selected = false;
			toggleColumnHeader5.Text = "Type";
			toggleColumnHeader5.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
			toggleColumnHeader5.Visible = true;
			toggleColumnHeader5.Width = 100;
			toggleColumnHeader6.Hovered = false;
			toggleColumnHeader6.Image = null;
			toggleColumnHeader6.Index = 0;
			toggleColumnHeader6.Pressed = false;
			toggleColumnHeader6.ScaleStyle = Lyquidity.Controls.ExtendedListViews.ColumnScaleStyle.Slide;
			toggleColumnHeader6.Selected = false;
			toggleColumnHeader6.Text = "Size";
			toggleColumnHeader6.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
			toggleColumnHeader6.Visible = true;
			this.treeListView1.Columns.AddRange(new Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader[] {
																												  toggleColumnHeader4,
																												  toggleColumnHeader5,
																												  toggleColumnHeader6});
			this.treeListView1.ColumnSortColor = System.Drawing.Color.Gainsboro;
			this.treeListView1.ColumnTrackColor = System.Drawing.Color.WhiteSmoke;
			this.treeListView1.ColumnTracking = true;
			this.treeListView1.ContextMenu = this.contextTList;
			this.treeListView1.Dock = System.Windows.Forms.DockStyle.Fill;
			this.treeListView1.GridLineColor = System.Drawing.Color.WhiteSmoke;
			this.treeListView1.HeaderMenu = null;
			this.treeListView1.ItemHeight = 20;
			this.treeListView1.ItemMenu = null;
			this.treeListView1.LabelEdit = false;
			this.treeListView1.Location = new System.Drawing.Point(0, 0);
			this.treeListView1.MultiSelect = true;
			this.treeListView1.Name = "treeListView1";
			treeListNode1.BackColor = System.Drawing.SystemColors.Window;
			treeListNode1.Checked = false;
			treeListNode1.Focused = false;
			treeListNode1.Font = null;
			treeListNode1.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode1.Hovered = false;
			treeListNode1.ImageIndex = 5;
			treeListNode1.Index = 0;
			treeListNode2.BackColor = System.Drawing.SystemColors.Window;
			treeListNode2.Checked = false;
			treeListNode2.Focused = false;
			treeListNode2.Font = null;
			treeListNode2.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode2.Hovered = false;
			treeListNode2.ImageIndex = 0;
			treeListNode2.Index = 0;
			treeListNode2.Selected = false;
			treeListNode2.StateImageIndex = 0;
			treeListNode2.Tag = null;
			treeListNode2.Text = "Test.exe";
			treeListNode2.UseItemStyleForSubItems = false;
			treeListNode3.BackColor = System.Drawing.SystemColors.Window;
			treeListNode3.Checked = false;
			treeListNode3.Focused = false;
			treeListNode3.Font = null;
			treeListNode3.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode3.Hovered = false;
			treeListNode3.ImageIndex = 0;
			treeListNode3.Index = 1;
			treeListNode3.Selected = false;
			treeListNode3.StateImageIndex = 0;
			treeListNode3.Tag = null;
			treeListNode3.Text = "Sample.exe";
			treeListNode3.UseItemStyleForSubItems = false;
			treeListNode1.Nodes.AddRange(new Lyquidity.Controls.ExtendedListViews.TreeListNode[] {
																									 treeListNode2,
																									 treeListNode3});
			treeListNode1.Selected = false;
			treeListNode1.StateImageIndex = 0;
			treeListNode1.Tag = null;
			treeListNode1.Text = "Applications";
			treeListNode1.UseItemStyleForSubItems = false;
			treeListNode4.BackColor = System.Drawing.SystemColors.Window;
			treeListNode4.Checked = false;
			treeListNode4.Focused = false;
			treeListNode4.Font = null;
			treeListNode4.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode4.Hovered = false;
			treeListNode4.ImageIndex = 5;
			treeListNode4.Index = 1;
			treeListNode5.BackColor = System.Drawing.SystemColors.Window;
			treeListNode5.Checked = false;
			treeListNode5.Focused = false;
			treeListNode5.Font = null;
			treeListNode5.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode5.Hovered = false;
			treeListNode5.ImageIndex = 5;
			treeListNode5.Index = 0;
			treeListNode5.Selected = false;
			treeListNode5.StateImageIndex = 0;
			treeListNode5.Tag = null;
			treeListNode5.Text = "Wave Files (.wav)";
			treeListNode5.UseItemStyleForSubItems = false;
			treeListNode6.BackColor = System.Drawing.Color.Empty;
			treeListNode6.Checked = false;
			treeListNode6.Focused = false;
			treeListNode6.Font = null;
			treeListNode6.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode6.Hovered = false;
			treeListNode6.ImageIndex = 5;
			treeListNode6.Index = 1;
			treeListNode6.Selected = false;
			treeListNode6.StateImageIndex = 0;
			treeListNode6.Tag = null;
			treeListNode6.Text = "Mpeg Layer 3 Files (.mp3)";
			treeListNode6.UseItemStyleForSubItems = false;
			treeListNode4.Nodes.AddRange(new Lyquidity.Controls.ExtendedListViews.TreeListNode[] {
																									 treeListNode5,
																									 treeListNode6});
			treeListNode4.Selected = false;
			treeListNode4.StateImageIndex = 0;
			treeListNode4.Tag = null;
			treeListNode4.Text = "Audio Files";
			treeListNode4.UseItemStyleForSubItems = false;
			treeListNode7.BackColor = System.Drawing.SystemColors.Window;
			treeListNode7.Checked = false;
			treeListNode7.Focused = false;
			treeListNode7.Font = null;
			treeListNode7.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode7.Hovered = false;
			treeListNode7.ImageIndex = 5;
			treeListNode7.Index = 2;
			treeListNode8.BackColor = System.Drawing.SystemColors.Window;
			treeListNode8.Checked = false;
			treeListNode8.Focused = false;
			treeListNode8.Font = null;
			treeListNode8.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode8.Hovered = false;
			treeListNode8.ImageIndex = 5;
			treeListNode8.Index = 0;
			treeListNode8.Selected = false;
			treeListNode8.StateImageIndex = 0;
			treeListNode8.Tag = null;
			treeListNode8.Text = "Text Files (.txt)";
			treeListNode8.UseItemStyleForSubItems = false;
			treeListNode9.BackColor = System.Drawing.SystemColors.Window;
			treeListNode9.Checked = false;
			treeListNode9.Focused = false;
			treeListNode9.Font = null;
			treeListNode9.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode9.Hovered = false;
			treeListNode9.ImageIndex = 5;
			treeListNode9.Index = 1;
			treeListNode9.Selected = false;
			treeListNode9.StateImageIndex = 0;
			treeListNode9.Tag = null;
			treeListNode9.Text = "Document Files (.doc)";
			treeListNode9.UseItemStyleForSubItems = false;
			treeListNode10.BackColor = System.Drawing.SystemColors.Window;
			treeListNode10.Checked = false;
			treeListNode10.Focused = false;
			treeListNode10.Font = null;
			treeListNode10.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode10.Hovered = false;
			treeListNode10.ImageIndex = 5;
			treeListNode10.Index = 2;
			treeListNode10.Selected = false;
			treeListNode10.StateImageIndex = 0;
			treeListNode10.Tag = null;
			treeListNode10.Text = "Rich Text Files (.rtf)";
			treeListNode10.UseItemStyleForSubItems = false;
			treeListNode7.Nodes.AddRange(new Lyquidity.Controls.ExtendedListViews.TreeListNode[] {
																									 treeListNode8,
																									 treeListNode9,
																									 treeListNode10});
			treeListNode7.Selected = false;
			treeListNode7.StateImageIndex = 0;
			treeListNode7.Tag = null;
			treeListNode7.Text = "Documents";
			treeListNode7.UseItemStyleForSubItems = false;
			treeListNode11.BackColor = System.Drawing.SystemColors.Window;
			treeListNode11.Checked = false;
			treeListNode11.Focused = false;
			treeListNode11.Font = null;
			treeListNode11.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode11.Hovered = false;
			treeListNode11.ImageIndex = 5;
			treeListNode11.Index = 3;
			treeListNode12.BackColor = System.Drawing.SystemColors.Window;
			treeListNode12.Checked = false;
			treeListNode12.Focused = false;
			treeListNode12.Font = null;
			treeListNode12.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode12.Hovered = false;
			treeListNode12.ImageIndex = 5;
			treeListNode12.Index = 0;
			treeListNode12.Selected = false;
			treeListNode12.StateImageIndex = 0;
			treeListNode12.Tag = null;
			treeListNode12.Text = "JPEG Files (.jpg)";
			treeListNode12.UseItemStyleForSubItems = false;
			treeListNode13.BackColor = System.Drawing.SystemColors.Window;
			treeListNode13.Checked = false;
			treeListNode13.Focused = false;
			treeListNode13.Font = null;
			treeListNode13.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode13.Hovered = false;
			treeListNode13.ImageIndex = 5;
			treeListNode13.Index = 1;
			treeListNode13.Selected = false;
			treeListNode13.StateImageIndex = 0;
			treeListNode13.Tag = null;
			treeListNode13.Text = "GIF Files (.gif)";
			treeListNode13.UseItemStyleForSubItems = false;
			treeListNode14.BackColor = System.Drawing.SystemColors.Window;
			treeListNode14.Checked = false;
			treeListNode14.Focused = false;
			treeListNode14.Font = null;
			treeListNode14.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode14.Hovered = false;
			treeListNode14.ImageIndex = 5;
			treeListNode14.Index = 2;
			treeListNode14.Selected = false;
			treeListNode14.StateImageIndex = 0;
			treeListNode14.Tag = null;
			treeListNode14.Text = "PNG Files (.png)";
			treeListNode14.UseItemStyleForSubItems = false;
			treeListNode11.Nodes.AddRange(new Lyquidity.Controls.ExtendedListViews.TreeListNode[] {
																									  treeListNode12,
																									  treeListNode13,
																									  treeListNode14});
			treeListNode11.Selected = false;
			treeListNode11.StateImageIndex = 0;
			treeListNode11.Tag = null;
			treeListNode11.Text = "Image Files";
			treeListNode11.UseItemStyleForSubItems = false;
			treeListNode15.BackColor = System.Drawing.SystemColors.Window;
			treeListNode15.Checked = false;
			treeListNode15.Focused = false;
			treeListNode15.Font = null;
			treeListNode15.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode15.Hovered = false;
			treeListNode15.ImageIndex = 5;
			treeListNode15.Index = 4;
			treeListNode16.BackColor = System.Drawing.SystemColors.Window;
			treeListNode16.Checked = false;
			treeListNode16.Focused = false;
			treeListNode16.Font = null;
			treeListNode16.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode16.Hovered = false;
			treeListNode16.ImageIndex = 5;
			treeListNode16.Index = 0;
			treeListNode16.Selected = false;
			treeListNode16.StateImageIndex = 0;
			treeListNode16.Tag = null;
			treeListNode16.Text = "MPEG 4 (.mpg, .avi)";
			treeListNode16.UseItemStyleForSubItems = false;
			treeListNode17.BackColor = System.Drawing.SystemColors.Window;
			treeListNode17.Checked = false;
			treeListNode17.Focused = false;
			treeListNode17.Font = null;
			treeListNode17.ForeColor = System.Drawing.SystemColors.WindowText;
			treeListNode17.Hovered = false;
			treeListNode17.ImageIndex = 5;
			treeListNode17.Index = 1;
			treeListNode17.Selected = false;
			treeListNode17.StateImageIndex = 0;
			treeListNode17.Tag = null;
			treeListNode17.Text = "Video Object (.vob)";
			treeListNode17.UseItemStyleForSubItems = false;
			treeListNode15.Nodes.AddRange(new Lyquidity.Controls.ExtendedListViews.TreeListNode[] {
																									  treeListNode16,
																									  treeListNode17});
			treeListNode15.Selected = false;
			treeListNode15.StateImageIndex = 0;
			treeListNode15.Tag = null;
			treeListNode15.Text = "Video Files";
			treeListNode15.UseItemStyleForSubItems = false;
			this.treeListView1.Nodes.AddRange(new Lyquidity.Controls.ExtendedListViews.TreeListNode[] {
																										  treeListNode1,
																										  treeListNode4,
																										  treeListNode7,
																										  treeListNode11,
																										  treeListNode15});
			this.treeListView1.RowSelectColor = System.Drawing.SystemColors.Highlight;
			this.treeListView1.RowTrackColor = System.Drawing.Color.WhiteSmoke;
			this.treeListView1.RowTracking = false;
			this.treeListView1.ShowLines = true;
			this.treeListView1.ShowRootLines = true;
			this.treeListView1.Size = new System.Drawing.Size(384, 324);
			this.treeListView1.SmallImageList = this.listImages;
			this.treeListView1.StateImageList = null;
			this.treeListView1.TabIndex = 0;
			this.treeListView1.Text = "treeListView1";
			this.treeListView1.VisualStyles = true;
			this.treeListView1.ItemActivate += new System.EventHandler(this.treeListView1_ItemActivate);
			// 
			// contextTList
			// 
			this.contextTList.MenuItems.AddRange(new System.Windows.Forms.MenuItem[] {
																						 this.menuItem4,
																						 this.menuItem5,
																						 this.menuItem6,
																						 this.menuItem9,
																						 this.menuItem10});
			// 
			// menuItem4
			// 
			this.menuItem4.Checked = true;
			this.menuItem4.Index = 0;
			this.menuItem4.Text = "Column Tracking";
			this.menuItem4.Click += new System.EventHandler(this.menuItem4_Click);
			// 
			// menuItem5
			// 
			this.menuItem5.Index = 1;
			this.menuItem5.Text = "Row Tracking";
			this.menuItem5.Click += new System.EventHandler(this.menuItem5_Click);
			// 
			// menuItem6
			// 
			this.menuItem6.Index = 2;
			this.menuItem6.Text = "Gridlines";
			this.menuItem6.Click += new System.EventHandler(this.menuItem6_Click);
			// 
			// menuItem9
			// 
			this.menuItem9.Index = 3;
			this.menuItem9.Text = "-";
			// 
			// menuItem10
			// 
			this.menuItem10.Checked = true;
			this.menuItem10.Index = 4;
			this.menuItem10.Text = "Use Visual Styles";
			this.menuItem10.Click += new System.EventHandler(this.menuItem10_Click);
			// 
			// tabPage3
			// 
			this.tabPage3.Controls.Add(this.btnClearHi);
			this.tabPage3.Controls.Add(this.lblItemsIns);
			this.tabPage3.Controls.Add(this.btnAddItems);
			this.tabPage3.Controls.Add(this.treeListView2);
			this.tabPage3.Location = new System.Drawing.Point(4, 22);
			this.tabPage3.Name = "tabPage3";
			this.tabPage3.Size = new System.Drawing.Size(384, 324);
			this.tabPage3.TabIndex = 2;
			this.tabPage3.Text = "High Volume TreeList";
			// 
			// btnClearHi
			// 
			this.btnClearHi.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.btnClearHi.Enabled = false;
			this.btnClearHi.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.btnClearHi.Location = new System.Drawing.Point(216, 296);
			this.btnClearHi.Name = "btnClearHi";
			this.btnClearHi.TabIndex = 3;
			this.btnClearHi.Text = "Clear";
			this.btnClearHi.Click += new System.EventHandler(this.btnClearHi_Click);
			// 
			// lblItemsIns
			// 
			this.lblItemsIns.Anchor = ((System.Windows.Forms.AnchorStyles)(((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.lblItemsIns.Location = new System.Drawing.Point(16, 296);
			this.lblItemsIns.Name = "lblItemsIns";
			this.lblItemsIns.Size = new System.Drawing.Size(200, 24);
			this.lblItemsIns.TabIndex = 2;
			this.lblItemsIns.Text = "Items Inserted: 0";
			this.lblItemsIns.TextAlign = System.Drawing.ContentAlignment.MiddleLeft;
			// 
			// btnAddItems
			// 
			this.btnAddItems.Anchor = ((System.Windows.Forms.AnchorStyles)((System.Windows.Forms.AnchorStyles.Bottom | System.Windows.Forms.AnchorStyles.Right)));
			this.btnAddItems.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.btnAddItems.Location = new System.Drawing.Point(296, 296);
			this.btnAddItems.Name = "btnAddItems";
			this.btnAddItems.TabIndex = 1;
			this.btnAddItems.Text = "Add Items";
			this.btnAddItems.Click += new System.EventHandler(this.btnAddItems_Click);
			// 
			// treeListView2
			// 
			this.treeListView2.Anchor = ((System.Windows.Forms.AnchorStyles)((((System.Windows.Forms.AnchorStyles.Top | System.Windows.Forms.AnchorStyles.Bottom) 
				| System.Windows.Forms.AnchorStyles.Left) 
				| System.Windows.Forms.AnchorStyles.Right)));
			this.treeListView2.BackColor = System.Drawing.SystemColors.Window;
			toggleColumnHeader7.Hovered = false;
			toggleColumnHeader7.Image = null;
			toggleColumnHeader7.Index = 0;
			toggleColumnHeader7.Pressed = false;
			toggleColumnHeader7.ScaleStyle = Lyquidity.Controls.ExtendedListViews.ColumnScaleStyle.Slide;
			toggleColumnHeader7.Selected = false;
			toggleColumnHeader7.Text = "Item";
			toggleColumnHeader7.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
			toggleColumnHeader7.Visible = true;
			toggleColumnHeader7.Width = 200;
			toggleColumnHeader8.Hovered = false;
			toggleColumnHeader8.Image = null;
			toggleColumnHeader8.Index = 0;
			toggleColumnHeader8.Pressed = false;
			toggleColumnHeader8.ScaleStyle = Lyquidity.Controls.ExtendedListViews.ColumnScaleStyle.Slide;
			toggleColumnHeader8.Selected = false;
			toggleColumnHeader8.Text = "Value";
			toggleColumnHeader8.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
			toggleColumnHeader8.Visible = true;
			toggleColumnHeader8.Width = 100;
			toggleColumnHeader9.Hovered = false;
			toggleColumnHeader9.Image = null;
			toggleColumnHeader9.Index = 0;
			toggleColumnHeader9.Pressed = false;
			toggleColumnHeader9.ScaleStyle = Lyquidity.Controls.ExtendedListViews.ColumnScaleStyle.Slide;
			toggleColumnHeader9.Selected = false;
			toggleColumnHeader9.Text = "Content";
			toggleColumnHeader9.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
			toggleColumnHeader9.Visible = true;
			toggleColumnHeader9.Width = 200;
			toggleColumnHeader10.Hovered = false;
			toggleColumnHeader10.Image = null;
			toggleColumnHeader10.Index = 0;
			toggleColumnHeader10.Pressed = false;
			toggleColumnHeader10.ScaleStyle = Lyquidity.Controls.ExtendedListViews.ColumnScaleStyle.Slide;
			toggleColumnHeader10.Selected = false;
			toggleColumnHeader10.Text = "Other Info";
			toggleColumnHeader10.TextAlign = System.Windows.Forms.HorizontalAlignment.Left;
			toggleColumnHeader10.Visible = true;
			this.treeListView2.Columns.AddRange(new Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader[] {
																												  toggleColumnHeader7,
																												  toggleColumnHeader8,
																												  toggleColumnHeader9,
																												  toggleColumnHeader10});
			this.treeListView2.ColumnSortColor = System.Drawing.Color.Gainsboro;
			this.treeListView2.ColumnTrackColor = System.Drawing.Color.WhiteSmoke;
			this.treeListView2.GridLineColor = System.Drawing.Color.WhiteSmoke;
			this.treeListView2.HeaderMenu = null;
			this.treeListView2.ItemHeight = 20;
			this.treeListView2.ItemMenu = null;
			this.treeListView2.LabelEdit = false;
			this.treeListView2.Location = new System.Drawing.Point(8, 8);
			this.treeListView2.Name = "treeListView2";
			this.treeListView2.RowSelectColor = System.Drawing.SystemColors.Highlight;
			this.treeListView2.RowTrackColor = System.Drawing.Color.WhiteSmoke;
			this.treeListView2.ShowLines = true;
			this.treeListView2.ShowRootLines = true;
			this.treeListView2.Size = new System.Drawing.Size(368, 280);
			this.treeListView2.SmallImageList = this.listImages;
			this.treeListView2.StateImageList = null;
			this.treeListView2.TabIndex = 0;
			this.treeListView2.Text = "treeListView2";
			this.treeListView2.VisualStyles = true;
			// 
			// button1
			// 
			this.button1.FlatStyle = System.Windows.Forms.FlatStyle.System;
			this.button1.Location = new System.Drawing.Point(320, 8);
			this.button1.Name = "button1";
			this.button1.TabIndex = 1;
			this.button1.Text = "Expand All";
			this.button1.Click += new System.EventHandler(this.button1_Click);
			// 
			// frmExtendedListTest
			// 
			this.AutoScaleBaseSize = new System.Drawing.Size(5, 13);
			this.ClientSize = new System.Drawing.Size(408, 406);
			this.Controls.Add(this.panel2);
			this.Controls.Add(this.panel1);
			this.Name = "frmExtendedListTest";
			this.Text = "Extended ListView Test";
			this.panel1.ResumeLayout(false);
			this.panel2.ResumeLayout(false);
			this.tabControl1.ResumeLayout(false);
			this.tabPage1.ResumeLayout(false);
			this.containerListView1.ResumeLayout(false);
			this.tabPage2.ResumeLayout(false);
			this.tabPage3.ResumeLayout(false);
			this.ResumeLayout(false);

		}
		#endregion

		private void btnExit_Click(object sender, System.EventArgs e)
		{
			Application.Exit();
		}

		private void menuItem1_Click(object sender, System.EventArgs e)
		{
			if (menuItem1.Checked)
			{
				menuItem1.Checked = false;
				containerListView1.ColumnTracking = false;
			}
			else
			{
				menuItem1.Checked = true;
				containerListView1.ColumnTracking = true;
			}
		}

		private void menuItem2_Click(object sender, System.EventArgs e)
		{
			if (menuItem2.Checked)
			{
				menuItem2.Checked = false;
				containerListView1.RowTracking = false;
			}
			else
			{
				menuItem2.Checked = true;
				containerListView1.RowTracking = true;
			}
		}

		private void menuItem3_Click(object sender, System.EventArgs e)
		{
			if (menuItem3.Checked)
			{
				menuItem3.Checked = false;
				containerListView1.GridLines = false;
			}
			else
			{
				menuItem3.Checked = true;
				containerListView1.GridLines = true;
			}
		}

		private void menuItem4_Click(object sender, System.EventArgs e)
		{
			if (menuItem4.Checked)
			{
				menuItem4.Checked = false;
				treeListView1.ColumnTracking = false;
			}
			else
			{
				menuItem4.Checked = true;
				treeListView1.ColumnTracking = true;
			}
		}

		private void menuItem5_Click(object sender, System.EventArgs e)
		{
			if (menuItem5.Checked)
			{
				menuItem5.Checked = false;
				treeListView1.RowTracking = false;
			}
			else
			{
				menuItem5.Checked = true;
				treeListView1.RowTracking = true;
			}
		}

		private void menuItem6_Click(object sender, System.EventArgs e)
		{
			if (menuItem6.Checked)
			{
				menuItem6.Checked = false;
				treeListView1.GridLines = false;
			}
			else
			{
				menuItem6.Checked = true;
				treeListView1.GridLines = true;
			}
		}

		private void menuItem10_Click(object sender, System.EventArgs e)
		{
			if (menuItem10.Checked)
			{

				menuItem10.Checked = false;
				treeListView1.VisualStyles = false;
				treeListView1.Invalidate();
			}
			else
			{
				menuItem10.Checked = true;
				treeListView1.VisualStyles = true;
				treeListView1.Invalidate();
			}
		}

		private void menuItem8_Click(object sender, System.EventArgs e)
		{
			if (menuItem8.Checked)
			{

				menuItem8.Checked = false;
				containerListView1.VisualStyles = false;
			}
			else
			{
				menuItem8.Checked = true;
				containerListView1.VisualStyles = true;
			}
		}

		private void btnAddItems_Click(object sender, System.EventArgs e)
		{
			if (treeListView2.Nodes.Count == 0)
			{
				MessageBox.Show("The insertion procedure inserts over 10,000 items. This may take several minutes, and the program will appear to be locked up. Please be patient.", "Inserting Items");
				btnAddItems.Enabled = false;
				Random rnd = new Random(unchecked((int)DateTime.Now.Ticks));

				treeListView2.BeginUpdate();

				try
				{
					int cnt = 0;
					for (int i=0; i<=500; i++)
					{
						TreeListNode node = new TreeListNode();
						node.Text = "Tree Node #" + i;
						node.ImageIndex = 5;

						for (int s=0; s<3; s++)
						{
							node.SubItems.Add("Sub item #" + i + "-" + s+1);
						}

						for (int la=0; la<rnd.Next(20); la++)
							//for (int la=0; la<20; la++)
						{
							TreeListNode na = new TreeListNode();
							na.Text = "Tree Node #" + i + "." + la;
							na.ImageIndex = 5;
							for (int sa=0; sa<3; sa++)
							{
								na.SubItems.Add("Sub item #" + i + "." + la + "-" + sa+1);
							}

							for (int lb=0; lb<rnd.Next(30)%20+5; lb++)
								//for (int lb=0; lb<10; lb++)
							{
								TreeListNode nb = new TreeListNode();
								nb.Text = "Tree Node #" + i + "." + la + "." + lb;
								nb.ImageIndex = 2;
								for (int sb=0; sb<3; sb++)
								{
									nb.SubItems.Add("Sub item #" + i + "." + la + "." + lb + "-" + sb+1);
								}

								for (int lc=0; lc<rnd.Next(7); lc++)
									//for (int lc=0; lc<5; lc++)
								{
									TreeListNode nc = new TreeListNode();
									nc.Text = "Tree Node #" + i + "." + la + "." + lb + "." + lc;
									nc.ImageIndex = 6;
									for (int sc=0; sc<3; sc++)
									{
										nc.SubItems.Add("Sub item #" + i + "." + la + "." + lb + "." + lc + "-" + sc+1);
									}

									nb.Nodes.Add(nc);
									cnt++;
								}

								na.Nodes.Add(nb);
								cnt++;
							}

							node.Nodes.Add(na);
							cnt++;
						}

						node.Expand();
						treeListView2.Nodes.Add(node);
						cnt++;
						// lblItemsIns.Text = "Items Inserted: " + cnt;
						lblItemsIns.Update();
					}
					btnClearHi.Enabled = true;
				}
				finally
				{
					treeListView2.EndUpdate();
				}
				treeListView2.ReportStatus();
			}
		}

		private void btnClearHi_Click(object sender, System.EventArgs e)
		{
			if (treeListView2.Nodes.Count > 0)
			{
				MessageBox.Show("Clearing a treelistview with many items can be a very timeconsuming proces, even longer than interting items. Please be patient while the list clears.", "Clearing Items");

				try
				{
					treeListView2.BeginUpdate();
					treeListView2.Nodes.Clear();
					lblItemsIns.Text = "Items Inserted: 0";
					btnClearHi.Enabled = false;
					btnAddItems.Enabled = true;
				}
				finally
				{
					this.treeListView2.EndUpdate();
				}
				treeListView2.ReportStatus();
			}
		}

		private void treeListView1_ItemActivate(object sender, System.EventArgs e)
		{
			TreeListView tv = (TreeListView)sender;
			int i = tv.SelectedNodes.Count;
			if (i == 0) return;
			Console.WriteLine(((TreeListNode)tv.SelectedNodes[0]).Text + " (" + i.ToString() + ")");
		}
		private void containerListView1_SelectedIndexChanged(object sender, System.EventArgs e)
		{
			ContainerListView lv = (ContainerListView)sender;
			int i = lv.SelectedItems.Count;
			if (i == 0) return;
			Console.WriteLine(((ContainerListViewItem)lv.SelectedItems[0]).Text + " (" + i.ToString() + ")");
		}

		private void button1_Click(object sender, System.EventArgs e)
		{
			if (this.tabControl1.SelectedIndex == 1) this.treeListView1.ExpandAll();
		}

		/// <summary>
		/// The main entry point for the application.
		/// </summary>
		[STAThread]
		static void Main() 
		{
			Application.Run(new frmExtendedListTest());
		}
	}
}
