// Author:  Bill Seddon
// Company: Lyquidity Solutions Limited
// 
// This work builds on code posted to SourceForge by
// Jon Rista (jrista@hotmail.com)
// 
// ContainerListView provides a listview type control
// that allows controls to be contained in sub item
// cells. The control is also fully compatible with
// Windows XP visual styles.
//
// This version fixes up drawing, mouse and keyboard
// event handling issues.
//
// This code is provided "as is" and no warranty about
// it fitness for any specific task is expressed or 
// implied.  If you choose to use this code, you do so
// at your own risk.
//
////////////////////////////////////////////////////////

using System;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.ComponentModel.Design.Serialization;
using System.Collections;
using System.Collections.Specialized;
using System.Diagnostics;
using System.Drawing;
using System.Drawing.Design;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
using System.Globalization;
using System.Reflection;
using System.Threading;
using System.Windows.Forms;
using System.Windows.Forms.Design;

namespace Lyquidity.Controls.ExtendedListViews
{
	#region Enumerations

	public enum WinMsg
	{
		WM_GETDLGCODE = 0x0087,
		WM_SETREDRAW = 0x000B,
		WM_CANCELMODE = 0x001F,
		WM_NOTIFY = 0x4e,
		WM_KEYDOWN = 0x100,
		WM_KEYUP = 0x101,
		WM_CHAR = 0x0102,
		WM_SYSKEYDOWN = 0x104,
		WM_SYSKEYUP = 0x105,
		WM_COMMAND = 0x111,
		WM_MENUCHAR = 0x120,
		WM_MOUSEMOVE = 0x200,
		WM_LBUTTONDOWN = 0x201,
		WM_MOUSELAST = 0x20a,
		WM_USER = 0x0400,
		WM_REFLECT = WM_USER + 0x1c00
	}

	public enum DialogCodes
	{
		DLGC_WANTARROWS =     0x0001,      /* Control wants arrow keys         */
		DLGC_WANTTAB =        0x0002,      /* Control wants tab keys           */
		DLGC_WANTALLKEYS =    0x0004,      /* Control wants all keys           */
		DLGC_WANTMESSAGE =    0x0004,      /* Pass message to control          */
		DLGC_HASSETSEL =      0x0008,      /* Understands EM_SETSEL message    */
		DLGC_DEFPUSHBUTTON =  0x0010,      /* Default pushbutton               */
		DLGC_UNDEFPUSHBUTTON = 0x0020,     /* Non-default pushbutton           */
		DLGC_RADIOBUTTON =    0x0040,      /* Radio button                     */
		DLGC_WANTCHARS =      0x0080,      /* Want WM_CHAR messages            */
		DLGC_STATIC =         0x0100,      /* Static item: don't include       */
		DLGC_BUTTON =         0x2000,      /* Button item: can be checked      */
	}

	public enum ColumnScaleStyle
	{
		Slide,
		Spring
	}

	public enum MultiSelectMode
	{
		Single,
		Range,
		Selective
	}
	#endregion

	#region Event Handlers and Arguments
	public class ItemsChangedEventArgs: EventArgs
	{
		public int IndexChanged = -1;

		public ItemsChangedEventArgs(int indexChanged)
		{
			IndexChanged = indexChanged;
		}
	}

	public delegate void ItemsChangedEventHandler(object sender, ItemsChangedEventArgs e);
	#endregion

	#region ContainerListViewItem classes
	[DesignTimeVisible(false), TypeConverter("Lyquidity.Controls.ExtendedListViews.ListViewItemConverter")]
	public class ContainerListViewItem: ICloneable
	{
		public event MouseEventHandler MouseDown;

		#region Variables
		private Color backcolor;
		// private Rectangle bounds;
		private bool ischecked;
		private bool focused;
		private Font font;
		private Color forecolor;
		private int imageindex;
		private int stateimageindex;
		private int index;
		private ContainerListView listview;
		private bool selected;		
		private ContainerSubListViewItemCollection subitems;
		private object tag;
		private string text;
		private bool styleall;
		private bool hovered;
		#endregion

		#region Constructors
		public ContainerListViewItem()
		{
			subitems = new ContainerSubListViewItemCollection();
			subitems.ItemsChanged += new ItemsChangedEventHandler(OnSubItemsChanged);
		}

		public ContainerListViewItem(ContainerListView Listview, int Index)
		{
			index = Index;
			listview = Listview;
		}
		#endregion

		private void OnSubItemsChanged(object sender, ItemsChangedEventArgs e)
		{
			subitems[e.IndexChanged].MouseDown += new MouseEventHandler(OnSubItemMouseDown);
		}

		private void OnSubItemMouseDown(object sender, MouseEventArgs e)
		{
			if (MouseDown != null)
				MouseDown(this, e);
		}

		#region Properties
		public Color BackColor
		{
			get { return backcolor;	}
			set { backcolor = value; }
		}
/*
		public Rectangle Bounds
		{
			get { return bounds; }
		}
*/
		public bool Checked
		{
			get { return ischecked; }
			set { ischecked = value; }
		}

		public bool Focused
		{
			get { return focused; }
			set { focused = value; }
		}

		public Font Font
		{
			get { return font; }
			set { font = value; }
		}

		public Color ForeColor
		{
			get { return forecolor; }
			set { forecolor = value; }
		}

		public int ImageIndex
		{
			get { return imageindex; }
			set { imageindex = value; }
		}

		public int Index
		{
			get { return index; }
			set { index = value; }
		}

		public ContainerListView ListView
		{
			get { return listview; }
		}

		public bool Selected
		{
			get { return selected; }
			set { selected = value; }
		}

		[
		Category("Behavior"),
		Description("The items collection of sub controls."),
		DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
		Editor(typeof(CollectionEditor), typeof(UITypeEditor))		 
		]
		public ContainerSubListViewItemCollection SubItems
		{
			get { return subitems; }
		}

		public int StateImageIndex
		{
			get { return stateimageindex; }
			set { stateimageindex = value; }
		}

		public object Tag
		{
			get { return tag; }
			set { tag = value; }
		}

		public string Text
		{
			get { return text; }
			set { text = value; }
		}

		public bool UseItemStyleForSubItems
		{
			get { return styleall; }
			set { styleall = value; }
		}

		[Browsable(false)]
		public bool Hovered
		{
			get { return hovered; }
			set { hovered = value; }
		}
		#endregion

		#region Methods
		public object Clone()
		{
			ContainerListViewItem lvi = new ContainerListViewItem();
			lvi.BackColor = backcolor;
			lvi.Focused = focused;
			lvi.Font = font;
			lvi.ForeColor = forecolor;
			lvi.ImageIndex = imageindex;
			lvi.Selected = selected;
			lvi.Tag = tag;
			lvi.Text = text;
			lvi.UseItemStyleForSubItems = styleall;

			return lvi;
		}
		#endregion
	}

	public class ContainerListViewItemCollection: CollectionBase
	{
		public event MouseEventHandler MouseDown;

		private void OnMouseDown(object sender, MouseEventArgs e)
		{
			if (MouseDown != null)
				MouseDown(sender, e);
		}

		#region Interface Implementations
		public ContainerListViewItem this[int index]
		{
			get { return List[index] as ContainerListViewItem; }
			set
			{
				List[index] = value;
				((ContainerListViewItem)List[index]).MouseDown += new MouseEventHandler(OnMouseDown);
			}
		}
		
		public int this[ContainerListViewItem item]
		{
			get { return List.IndexOf(item); }
		}
		public int Add(ContainerListViewItem item)
		{
			item.MouseDown += new MouseEventHandler(OnMouseDown);
			return item.Index = List.Add(item);
		}

		public void AddRange(ContainerListViewItem[] items)
		{
			lock(List.SyncRoot)
			{
				for (int i=0; i<items.Length; i++)
				{
					items[i].MouseDown += new MouseEventHandler(OnMouseDown);
					items[i].Index = List.Add(items[i]);
				}
			}
		}

		public void Remove(ContainerListViewItem item)
		{
			item.MouseDown -= new MouseEventHandler(OnMouseDown);
			List.Remove(item);
		}

		public new void Clear()
		{
			for (int i=0; i<List.Count; i++)
			{
				ContainerSubListViewItemCollection col = ((ContainerListViewItem)List[i]).SubItems;
				for (int j=0; j<col.Count; j++)
				{
					if (col[j].ItemControl != null)
					{
						col[j].ItemControl.Parent = null;
						col[j].ItemControl.Visible = false;
						col[j].ItemControl = null;
					}
				}
				
			}
			List.Clear();
		}
		public int IndexOf(ContainerListViewItem item)
		{
			return List.IndexOf(item);
		}
		#endregion
	}

	public class SelectedContainerListViewItemCollection: CollectionBase
	{
		#region Interface Implementations
		public ContainerListViewItem this[int index]
		{
			get { return List[index] as ContainerListViewItem; }
			set
			{
				List[index] = value;
			}
		}
		
		public int this[ContainerListViewItem item]
		{
			get { return List.IndexOf(item); }
		}
		public int Add(ContainerListViewItem item)
		{
			return item.Index = List.Add(item);
		}

		public void AddRange(ContainerListViewItem[] items)
		{
			lock(List.SyncRoot)
			{
				for (int i=0; i<items.Length; i++)
				{
					items[i].Index = List.Add(items[i]);
				}
			}
		}

		public void Remove(ContainerListViewItem item)
		{
			List.Remove(item);
		}

		public new void Clear()
		{
			List.Clear();
		}

		public int IndexOf(ContainerListViewItem item)
		{
			return List.IndexOf(item);
		}
		#endregion
	}

	#endregion

	#region ContainerSubListViewItem classes
	[DesignTimeVisible(false), TypeConverter("Lyquidity.Controls.ExtendedListViews.SubListViewItemConverter")]
	public class ContainerSubListViewItem: ICloneable
	{
		public event MouseEventHandler MouseDown;

		protected void OnMouseDown(object sender, MouseEventArgs e)
		{
			if (MouseDown != null)
				MouseDown(sender, e);
		}

		private string text;
		private Control childControl;

		public ContainerSubListViewItem()
		{
			text = "SubItem";
		}

		public ContainerSubListViewItem(Control control)
		{
			text = "";
			Construct(control);
		}

		public ContainerSubListViewItem(string str)
		{
			text = str;
			Construct(null);
		}

		private void Construct(Control control)
		{
			childControl = control;

			if (childControl != null)
				childControl.MouseDown += new MouseEventHandler(OnMouseDown);
		}

		public Control ItemControl
		{
			get
			{
				return childControl;
			}
			set
			{
				childControl = (Control)value;

				if (childControl != null)
					childControl.MouseDown += new MouseEventHandler(OnMouseDown);
			}
		}

		public object Clone()
		{
			ContainerSubListViewItem slvi = new ContainerSubListViewItem();
			slvi.ItemControl = null;
			slvi.Text = text;

			return slvi;
		}

		public string Text
		{
			get { return text; }
			set { text = value; }
		}

		public override string ToString()
		{
			return (childControl == null ? text : childControl.ToString());
		}
	}

	public class ContainerSubListViewItemCollection: CollectionBase
	{
		public event ItemsChangedEventHandler ItemsChanged;

		protected void OnItemsChanged(ItemsChangedEventArgs e)
		{
			if (ItemsChanged != null)
				ItemsChanged(this, e);
		}

		#region Interface Implementations
		public ContainerSubListViewItem this[int index]
		{
			get 
			{ 
				try
				{
					return List[index] as ContainerSubListViewItem; 
				}
				catch
				{
					return null;
				}
			}
			set
			{
				List[index] = value; 
				OnItemsChanged(new ItemsChangedEventArgs(index));
			}
		}

		public int Add(ContainerSubListViewItem item)
		{
			int index = List.Add(item);
			OnItemsChanged(new ItemsChangedEventArgs(index));
			return index;
		}

		public ContainerSubListViewItem Add(Control control)
		{
			ContainerSubListViewItem slvi = new ContainerSubListViewItem(control);
			lock(List.SyncRoot)
				OnItemsChanged(new ItemsChangedEventArgs(List.Add(slvi)));
			return slvi;
		}

		public ContainerSubListViewItem Add(string str)
		{
			ContainerSubListViewItem slvi = new ContainerSubListViewItem(str);
			lock(List.SyncRoot)
				OnItemsChanged(new ItemsChangedEventArgs(List.Add(slvi)));
			return slvi;
		}

		public void AddRange(ContainerSubListViewItem[] items)
		{
			lock(List.SyncRoot)
			{
				for (int i=0; i<items.Length; i++)
				{
					OnItemsChanged(new ItemsChangedEventArgs(List.Add(items[i])));
				}
			}
		}
		#endregion
	}
	#endregion

	#region Column Header classes
	[DesignTimeVisible(false), TypeConverter("Lyquidity.Controls.ExtendedListViews.ToggleColumnHeaderConverter")]
	public class ToggleColumnHeader: ICloneable
	{
		// send an internal event when a column is resized
		internal event EventHandler WidthResized;
		private void OnWidthResized()
		{
			if (WidthResized != null)
				WidthResized(this, new EventArgs());
		}

		private int index;
		private ContainerListView listview;
		private string text;
		private HorizontalAlignment textAlign;
		private int width;
		private bool visible;
		private bool hovered;
		private bool pressed;
		private bool selected;
		private ColumnScaleStyle scaleStyle;
		private Bitmap image;

		public ToggleColumnHeader()
		{
			index = 0;
			listview = null;
			textAlign = HorizontalAlignment.Left;
			width = 90;
			visible = true;
			hovered = false;
			pressed = false;
			scaleStyle = ColumnScaleStyle.Slide;
		}


		[Browsable(false)]
		public bool Selected
		{
			get { return selected; }
			set { selected = value; }
		}

		[
		Category("Appearance"),
		Description("The image to display in this header.")
		]
		public Bitmap Image
		{
			get { return image; }
			set { image = value; }
		}

		[Category("Behavior"), Description("Determines how a column reacts when another column is scaled.")]
		public ColumnScaleStyle ScaleStyle
		{
			get { return scaleStyle; }
			set { scaleStyle = value; }
		}

		[Category("Data"), Description("The index of this column in the collection.")]
		public int Index
		{
			get { return index; }
			set { index = value; }
		}

		[Category("Data"), Description("The parent listview of this column header.")]
		public ContainerListView ListView
		{
			get { return listview; }
		}

		[Category("Appearance"), Description("The title of this column header.")]
		public string Text
		{
			get { return text; }
			set { text = value; }
		}

		[Category("Behavior"), Description("The alignment of the column headers title.")]
		public HorizontalAlignment TextAlign
		{
			get { return textAlign; }
			set { textAlign = value; }
		}

		[Category("Behavior"), Description("The width in pixels of this column header."), DefaultValue(90)]
		public int Width
		{
			get { return width; }
			set 
			{ 
				width = value; 
				OnWidthResized();
			}
		}

		[Category("Behavior"), Description("Determines wether the control is visible or hidden.")]
		public bool Visible
		{
			get { return visible; }
			set { visible = value; }
		}

		[Browsable(false)]
		public bool Hovered
		{
			get { return hovered; }
			set { hovered = value; }
		}

		[Browsable(false)]
		public bool Pressed
		{
			get { return pressed; }
			set { pressed = value; }
		}

		public object Clone()
		{
			ToggleColumnHeader ch = new ToggleColumnHeader();
			ch.Index = index;
			ch.Text = text;
			ch.TextAlign = textAlign;
			ch.Width = width;

			return ch;
		}

		public override string ToString()
		{
			return text;
		}
	}

	public class ColumnHeaderCollection: CollectionBase
	{
		internal event EventHandler WidthResized;
		private void OnWidthResized(object sender, EventArgs e)
		{
			if (WidthResized != null)
				WidthResized(sender, e);
		}

		#region Interface Implementations		
		public ToggleColumnHeader this[int index]
		{
			get
			{ 
				ToggleColumnHeader tch = new ToggleColumnHeader();
				try
				{
					tch = List[index] as ToggleColumnHeader;
				}
				catch
				{
					Debug.WriteLine("Column at index " + index + " does not exist.");
				}
				return tch;
			}
			set 
			{ 
				List[index] = value; 
				((ToggleColumnHeader)List[index]).WidthResized += new EventHandler(OnWidthResized);
			}
		}

		public virtual int Add(Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader colhead)
		{
			colhead.WidthResized += new EventHandler(OnWidthResized);
			return colhead.Index = List.Add(colhead);
		}

		public virtual ToggleColumnHeader Add(string str, int width, HorizontalAlignment textAlign)
		{
			ToggleColumnHeader tch = new ToggleColumnHeader();
			tch.Text = str;
			tch.Width = width;
			tch.TextAlign = textAlign;
			tch.WidthResized += new EventHandler(OnWidthResized);

			lock(List.SyncRoot)
			{
				tch.Index = List.Add(tch);
			}
			return tch;
		}

		public virtual void AddRange(Lyquidity.Controls.ExtendedListViews.ToggleColumnHeader[] items)
		{
			lock(List.SyncRoot)
			{
				for (int i=0; i< items.Length; i++)
				{
					items[i].WidthResized += new EventHandler(OnWidthResized);
					List.Add(items[i]);
				}
			}
		}
		#endregion
	}
	#endregion

	#region ContainerListView

	#region ContainerListView Delegates
	public delegate void ContextMenuEventHandler(object sender, MouseEventArgs e);
	public delegate void ItemMenuEventHandler(object sender, MouseEventArgs e);
	public delegate void HeaderMenuEventHandler(object sender, MouseEventArgs e);
	#endregion

	/// <summary>
	/// Provides a listview control in detail mode that
	/// provides containers for each cell in a row/column.
	/// The container can hold almost any object that derives
	/// directly or indirectly from Control.
	/// </summary>

	[DefaultProperty("Items")]
	[ToolboxItem(true)]
	[ToolboxBitmap(typeof(ContainerListView), "Resources.listview.bmp")]
	public class ContainerListView: Control
	{
		#region Events
		public event LabelEditEventHandler AfterLabelEdit;
		public event LabelEditEventHandler BeforeLabelEdit;
		public event ColumnClickEventHandler ColumnClick;
		public event EventHandler ItemActivate;
		public event EventHandler SelectedIndexChanged;

		protected void OnAfterLabelEdit(LabelEditEventArgs e)
		{
			if (AfterLabelEdit != null)
				try { AfterLabelEdit(this, e);  /* "this" should be the label?  BMS 2003/05/22 */ }
				catch {}
		}

		protected void OnBeforeLabelEdit(LabelEditEventArgs e)
		{
			if (BeforeLabelEdit != null)
				try { BeforeLabelEdit(this, e);  /* "this" should be the label?  BMS 2003/05/22  */ }
				catch {}
		}

		protected void OnColumnClick(ColumnClickEventArgs e)
		{
			if (ColumnClick != null)
				try { ColumnClick(this, e); }
				catch {}
		}

		protected void OnItemActivate(EventArgs e)
		{
			if (ItemActivate != null)
				try { ItemActivate(this, e); }
				catch {}
		}

		protected void OnSelectedIndexChanged(EventArgs e)
		{
			if (SelectedIndexChanged != null)
				try { SelectedIndexChanged(this, e); } 
				catch {}
		}

		// This handler links any click events on an items subcontrols
		// to this control. Clicks will activate or deactivate the 
		// row containing the subcontrol.
		protected virtual void OnSubControlMouseDown(object sender, MouseEventArgs e)
		{
			//Debug.WriteLine("Subcontrol clicked.");

			ContainerListViewItem item = ((ContainerListViewItem)sender);

			if (multiSelectMode == MultiSelectMode.Single)
			{
				selectedIndices.Clear();
				selectedItems.Clear();

				for (int i=0; i<items.Count; i++)
				{

					items[i].Focused = false;
					items[i].Selected = false;
					if (items[i].Equals(item))
					{
						items[i].Focused = true;
						items[i].Selected = true;						
						focusedIndex = firstSelected = i;

						// set selected items and indices collections							
						selectedIndices.Add(i);						
						selectedItems.Add(items[i]);
					}
				}
				OnSelectedIndexChanged(new EventArgs());
			}
			else if (multiSelectMode == MultiSelectMode.Range)
			{
			}
			else if (multiSelectMode == MultiSelectMode.Selective)
			{
				// unfocus the previously focused item
				for (int i=0; i<items.Count; i++)
					items[i].Focused = false;

				if (item.Selected)
				{
					item.Focused = false;
					item.Selected = false;
					selectedIndices.Remove(items[item]);
					selectedItems.Remove(item);
					OnSelectedIndexChanged(new EventArgs());
				}
				else
				{
					item.Focused = true;
					item.Selected = true;
					selectedIndices.Add(items[item]);
					selectedItems.Add(item);
					OnSelectedIndexChanged(new EventArgs());
				}
				focusedIndex = items[item];
					
			}			

			Invalidate(this.ClientRectangle);
		}

		// The ContainerListView provides three context menus.
		// One for the header, one for the visible rows, and
		// one for the whole control, which is fallen back
		// on when the header and item menus do not exist.
		public event ContextMenuEventHandler ContextMenuEvent;
		public event ItemMenuEventHandler ItemMenuEvent;
		public event HeaderMenuEventHandler HeaderMenuEvent;

		protected void OnContextMenuEvent(MouseEventArgs e)
		{
			if (ContextMenuEvent != null)
				ContextMenuEvent(this, e);

			PopMenu(contextMenu, e);
		}

		protected void OnItemMenuEvent(MouseEventArgs e)
		{
			if (ItemMenuEvent != null)
				ItemMenuEvent(this, e);
			else if (itemMenu == null)
				OnContextMenuEvent(e);
			else
				PopMenu(itemMenu, e);
		}

		protected void OnHeaderMenuEvent(MouseEventArgs e)
		{
			if (HeaderMenuEvent != null)
				HeaderMenuEvent(this, e);
			else if (headerMenu == null)
				OnContextMenuEvent(e);
			else
				PopMenu(headerMenu, e);
		}

		protected void PopMenu(System.Windows.Forms.ContextMenu theMenu, MouseEventArgs e)
		{
			if (theMenu != null)
				theMenu.Show(this, new Point(e.X, e.Y));
		}

		// Handlers for scrollbars scroll
		protected void OnScroll(object sender, EventArgs e)
		{
			GenerateColumnRects();
			Invalidate();
		}

		// Handler for column width resizing
		protected void OnColumnWidthResize(object sender, EventArgs e)
		{
			GenerateColumnRects();
		}
		#endregion

		#region Variables

		protected ItemActivation activation;
		protected bool allowColumnReorder = false;
		protected BorderStyle borderstyle = BorderStyle.Fixed3D;
		private int borderWid = 2;
		protected Lyquidity.Controls.ExtendedListViews.ColumnHeaderCollection columns;			
		protected ColumnHeaderStyle headerStyle = ColumnHeaderStyle.Nonclickable;
		protected int headerBuffer = 20;

		protected bool hideSelection = true;
		protected bool hoverSelection = false;
		protected bool multiSelect = false;
		protected MultiSelectMode multiSelectMode = MultiSelectMode.Single;

		protected ContainerListViewItemCollection items;
		protected bool labelEdit = false;
		protected ImageList smallImageList, stateImageList;
		protected Comparer sortComparer;		
		protected bool scrollable = true;
		protected SortOrder sorting;
		protected string text;
		protected ContainerListViewItem topItem;

		//protected ContainerListViewItemCollection selectedItems = null;
		protected SelectedContainerListViewItemCollection selectedItems = null;
		protected ArrayList selectedIndices = null;

		protected bool visualStyles = false;

		protected Lyquidity.Controls.ExtendedListViews.ContainerListViewItem focusedItem;	
		protected int focusedIndex = -1;
		protected bool isFocused = false;

		protected Rectangle headerRect;
		protected Rectangle[] columnRects;
		protected Rectangle[] columnSizeRects;
		protected int lastColHovered = -1;
		protected int lastColPressed = -1;
		protected int lastColSelected = -1;
		protected bool doColTracking = false;
		protected Color colTrackColor = Color.WhiteSmoke;
		protected Color colSortColor = Color.Gainsboro;
		protected int allColsWidth = 0;
		protected bool colScaleMode = false;
		protected int colScalePos = 0;
		protected int colScaleWid = 0;
		protected int scaledCol = -1;

		protected Rectangle rowsRect;
		protected Rectangle[] rowRects;
		protected int lastRowHovered = -1;
		protected int rowHeight = 18;
		protected int allRowsHeight = 0;
		protected bool doRowTracking = true;
		protected Color rowTrackColor = Color.WhiteSmoke;
		protected Color rowSelectColor = SystemColors.Highlight;
		protected bool fullRowSelect = true;
		protected int firstSelected=-1, lastSelected=-1;

		protected bool gridLines = false;
		protected Color gridLineColor = Color.WhiteSmoke;

		protected Point lastClickedPoint;

		protected bool captureFocusClick = false;

		protected ContextMenu headerMenu, itemMenu, contextMenu;
		protected HScrollBar hscrollBar;
		protected VScrollBar vscrollBar;

		protected bool ensureVisible = true;

		private Stack m_cUpdateTransactions = new Stack();

		// private System.ComponentModel.Container components = null;

		#endregion

		#region Constructor
		public ContainerListView()
		{
			Construct();
		}

		private void Construct()
		{
			rowHeight = 18;

			//base.BackColor = SystemColors.Window;

			SetStyle(ControlStyles.AllPaintingInWmPaint|ControlStyles.ResizeRedraw|
				ControlStyles.Opaque|ControlStyles.UserPaint|ControlStyles.DoubleBuffer|
				ControlStyles.Selectable|ControlStyles.UserMouse, true);

			this.BackColor = SystemColors.Window;

			columns = new Lyquidity.Controls.ExtendedListViews.ColumnHeaderCollection();
			items = new ContainerListViewItemCollection();
			selectedIndices = new ArrayList();
			// selectedItems = new ContainerListViewItemCollection();
			selectedItems = new SelectedContainerListViewItemCollection();

			hscrollBar = new HScrollBar();
			hscrollBar.Parent = this;
			hscrollBar.Minimum = 0;
			hscrollBar.Maximum = 0;
			hscrollBar.SmallChange = 10;
			hscrollBar.Hide();

			vscrollBar = new VScrollBar();
			vscrollBar.Parent = this;
			vscrollBar.Minimum = 0;
			vscrollBar.Maximum = 0;
			vscrollBar.SmallChange = rowHeight;
			vscrollBar.Hide();

			Attach();  // (Events to scrollbars etc.

			GenerateColumnRects();
			GenerateHeaderRect();
		}
		#endregion

		#region Methods
		public void BeginUpdate()
		{
			m_cUpdateTransactions.Push(this);
		}

		public void EndUpdate()
		{
			if (m_cUpdateTransactions.Count > 0) m_cUpdateTransactions.Pop();
			if (!this.InUpdateTransaction) this.Invalidate();  // Force repaint when update ended
		}

		#endregion

		#region Properties
		[
		Category("Behavior"),
		Description("Specifies wether the selected item is always visible"),
		DefaultValue(true)
		]
		public bool EnsureVisible
		{
			get { return ensureVisible; }
			set { ensureVisible = value; }
		}

		[
		Category("Behavior"),
		Description("Specifies wether the control will capture the click used to focus the control and adjust the selection accordingly, or not."),
		DefaultValue(false)
		]
		public bool CaptureFocusClick
		{
			get { return captureFocusClick; }
			set { captureFocusClick = value; }
		}

		[
		Category("Behavior"),
		Description("The context menu displayed when the header is right-clicked.")
		]
		public ContextMenu HeaderMenu
		{
			get { return headerMenu; }
			set { headerMenu = value; }
		}

		[
		Category("Behavior"),
		Description("The context menu displayed when an item is right-clicked.")
		]
		public ContextMenu ItemMenu
		{
			get { return itemMenu; }
			set { itemMenu = value; }
		}

		[
		Category("Behavior"),
		Description("The context menu displayed when the control is right-clicked.")
		]
		public override ContextMenu ContextMenu
		{
			get { return contextMenu; }
			set { contextMenu = value; }
		}

		[
		Category("Behavior"),
		Description("The lists column headers."),
		DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
		Editor(typeof(CollectionEditor), typeof(UITypeEditor))
		]
		public Lyquidity.Controls.ExtendedListViews.ColumnHeaderCollection Columns
		{
			get { return columns; }
		}

		[
		Category("Data"),
		Description("The items contained in the list."),
		DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
		Editor(typeof(CollectionEditor), typeof(UITypeEditor))
		]
		public virtual ContainerListViewItemCollection Items
		{
			get { return items; }
		}

		[
		Category("Behavior"),
		Description("Specifies what action activates and item."),
		DefaultValue(ItemActivation.Standard)
		]
		public ItemActivation Activation
		{
			get
			{
				return activation;
			}
			set
			{
				activation = value;
			}
		}

		[
		Category("Behavior"),
		Description("Specifies whether column headers may be reordered."),
		DefaultValue(false)
		]
		public bool AllowColumnReorder
		{
			get 
			{
				return allowColumnReorder;
			}
			set
			{
				allowColumnReorder = value;
			}
		}

		[
		Category("Appearance"),
		Description("Specifies what style border the control has."),
		DefaultValue(BorderStyle.Fixed3D)
		]
		public BorderStyle BorderStyle
		{
			get { return borderstyle; }
			set
			{ 
				borderstyle = value;
				if (borderstyle == BorderStyle.Fixed3D)
				{
					borderWid = 2;
				}
				else
				{
					borderWid = 1;
				}
				Invalidate(this.ClientRectangle);
			}
		}

		[
		Category("Appearance"),
		Description("Specifies wether to show column headers, and wether they respond to mouse clicks."),
		DefaultValue(ColumnHeaderStyle.Nonclickable)		 
		]
		public ColumnHeaderStyle HeaderStyle
		{
			get { return headerStyle; }
			set 
			{ 
				headerStyle = value; 
				Invalidate(this.ClientRectangle);
				if (headerStyle == ColumnHeaderStyle.None)
					headerBuffer = 0;
				else
					headerBuffer = 20;
			}
		}

		[
		Category("Behavior"),
		Description("Enables column tracking, highlighting the column gray when the mouse hovers over its header."),
		DefaultValue(false)
		]
		public bool ColumnTracking
		{
			get { return doColTracking; }
			set { doColTracking = value; }
		}

		[
		Category("Appearance"),
		Description("Specifies the color used for column hot-tracking."),
		DefaultValue(typeof(Color), "Color.WhiteSmoke")
		]
		public Color ColumnTrackColor
		{
			get { return colTrackColor; }
			set { colTrackColor = value; }
		}

		[
		Category("Appearance"),
		Description("Specifies the color used for the currently selected sorting column."),
		DefaultValue(typeof(Color), "Color.Gainsboro")
		]
		public Color ColumnSortColor
		{
			get { return colSortColor; }
			set { colSortColor = value; }
		}

		[
		Category("Behavior"),
		Description("Enables row tracking, highlighting the row gray when the mouse hovers over it."),
		DefaultValue(true)
		]
		public bool RowTracking
		{
			get { return doRowTracking; }
			set { doRowTracking = value; }
		}

		[
		Category("Appearance"),
		Description("Specifies the color used for row hot-tracking."),
		DefaultValue(typeof(Color), "Color.WhiteSmoke")
		]
		public Color RowTrackColor
		{
			get { return rowTrackColor; }
			set { rowTrackColor = value; }
		}

		[
		Category("Appearance"),
		Description("Specifies the color used for selected rows."),
		DefaultValue(typeof(Color), "SystemColors.Highlight")
		]
		public Color RowSelectColor
		{
			get { return rowSelectColor; }
			set { rowSelectColor = value; }
		}

		[
		Category("Behavior"),
		Description("Determines wether to highlight the full row or just the label of selected items."),
		DefaultValue(true)
		]
		public bool FullRowSelect
		{
			get { return fullRowSelect; }
			set { fullRowSelect = value; }
		}

		[
		Category("Appearance"),
		Description("Specifies the color used for grid lines."),
		DefaultValue(typeof(Color), "Color.WhiteSmoke")
		]
		public Color GridLineColor
		{
			get { return gridLineColor; }
			set { gridLineColor = value; }
		}

		[
		Category("Behavior"),
		Description("Specifies wether to show grid lines."),
		DefaultValue(false)
		]
		public bool GridLines
		{
			get { return gridLines; }
			set 
			{
				gridLines = value; 
				Invalidate(this.ClientRectangle);
			}
		}

		[
		Category("Behavior"),
		Description("The lists small image list (16x16).")
		]
		public ImageList SmallImageList
		{
			get { return smallImageList; }
			set 
			{
				smallImageList = value; 
				Invalidate(this.ClientRectangle);
			}
		}

		[
		Category("Behavior"),
		Description("The lists custom state image list (16x16).")
		]
		public ImageList StateImageList
		{
			get { return stateImageList; }
			set { stateImageList = value; }
		}

		[
		Category("Behavior"),
		Description("Determins wether primary name of each item is editable or not.")
		]
		public bool LabelEdit
		{
			get { return labelEdit; }
			set { labelEdit = value; }
		}

		[
		Category("Behavior"),
		Description("Determines wether to hide the selected rows when the control looses focus."),
		DefaultValue(true)
		]
		public bool HideSelection
		{
			get { return hideSelection; }
			set { hideSelection = value; }
		}
        
		[
		Category("Behavior"),
		Description("Determines wether to automatically select a row when the mouse is hovered over it for a short time."),
		DefaultValue(false)
		]
		public bool HoverSelection
		{
			get { return hoverSelection; }
			set { hoverSelection = value; }
		}

		[
		Category("Behavior"),
		Description("Determins wether the control will allow multiple selections."),
		DefaultValue(false)
		]
		public bool MultiSelect
		{
			get { return multiSelect; }
			set { multiSelect = value; }
		}

		[
		Category("Appearance"),
		Description("Specifies wether to use WindowsXP visual styles on this control."),
		DefaultValue(false)
		]
		public bool	VisualStyles
		{
			get 
			{
				bool val;
				try
				{
					val = visualStyles && Lyquidity.Controls.ExtendedListViews.uxTheme.Wrapper.IsAppThemed();
				}
				catch
				{
					val = visualStyles;
				}
				return val;
			}
			set
			{
				visualStyles = value;				
			}
		}

		[Browsable(false)]
		public ArrayList SelectedIndices
		{
			get { return selectedIndices; }
		}

		[Browsable(false)]
		// public ContainerListViewItemCollection SelectedItems
		public SelectedContainerListViewItemCollection SelectedItems
		{
			get { return selectedItems; }
		}

		protected bool InUpdateTransaction
		{
			get { return this.m_cUpdateTransactions.Count > 0; }
		}

		#endregion

		#region Overrides
		protected const int WM_KEYDOWN = 0x0100;
		protected const int VK_LEFT = 0x0025;
		protected const int VK_UP = 0x0026;
		protected const int VK_RIGHT = 0x0027;
		protected const int VK_DOWN = 0x0028;
/*
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
*/


		protected override void OnPaint(PaintEventArgs e)
		{
			Rectangle r = ClientRectangle;
			Graphics g = e.Graphics;

			DrawBackground(g, r);
			DrawRows(g, r);
			DrawHeaders(g, r);
			DrawExtra(g, r);			
			DrawBorder(g, r);			
		}

		protected override void OnResize(EventArgs e)
		{
			base.OnResize(e);
			GenerateHeaderRect();
			GenerateRowsRect();
			AdjustScrollbars();
           
			// invalidate subitem controls
			for (int i=0; i<items.Count; i++)
			{
				ContainerListViewItem lvi = ((ContainerListViewItem)items[i]);
				for (int j=0; j< lvi.SubItems.Count; j++)
				{
					ContainerSubListViewItem slvi = lvi.SubItems[j];
					if (slvi.ItemControl != null)
						slvi.ItemControl.Invalidate(slvi.ItemControl.ClientRectangle);
				}
			}
			Invalidate();										   
		}

		protected override void WndProc(ref Message m)
		{
			base.WndProc(ref m);
			if (m.Msg == (int)WinMsg.WM_GETDLGCODE)
			{
				// This line makes Arrow and Tab key events cause OnKeyXXX events to fire
				m.Result = new IntPtr((int)DialogCodes.DLGC_WANTCHARS | (int)DialogCodes.DLGC_WANTARROWS | m.Result.ToInt32());
			}
		}

		private int mouseMoveTicks = 0;
		protected override void OnMouseMove(MouseEventArgs e)
		{
			int i;

			base.OnMouseMove(e);

			//lastColHovered = -1;
			lastRowHovered = -1;

			// if the mouse button is currently
			// pressed down on a header column,
			// moving will attempt to move the
			// position of that column
			if (lastColPressed >= 0 && allowColumnReorder)
			{
				if (Math.Abs(e.X-lastClickedPoint.X) > 3)
				{
					// set rect for drag pos
				}				
			}
			else if (colScaleMode)
			{
				lastColHovered = -1;
				Cursor.Current = Cursors.VSplit;
				colScalePos = e.X-lastClickedPoint.X;
				if (colScalePos+colScaleWid <= 0)
					columns[scaledCol].Width = 1;
				else
					columns[scaledCol].Width = colScalePos+colScaleWid;				
				
				Invalidate();				
			}
			else
			{
				if (columns.Count > 0)
				{
					// check region mouse is in
					//  header			
					if (headerStyle != ColumnHeaderStyle.None)
					{
						//GenerateHeaderRect();
                    
						Cursor.Current = Cursors.Default;
						if (MouseInRect(e, headerRect))
						{
							if (columnRects.Length < columns.Count)
								GenerateColumnRects();

							for (i=0; i<columns.Count; i++)
							{							
								if (MouseInRect(e, columnRects[i]))
								{
									columns[i].Hovered = true;
									lastColHovered = i;
								}
								else
								{
									columns[i].Hovered = false;
								}

								if (MouseInRect(e, columnSizeRects[i]))
								{
									Cursor.Current = Cursors.VSplit;
								}
							}
							Invalidate();
							if (++mouseMoveTicks > 10)
							{
								mouseMoveTicks = 0;
								Thread.Sleep(1);
							}
							return;
						}					
					}
				}

				if (lastColHovered >= 0)
				{
					columns[lastColHovered].Hovered = false;
					lastColHovered = -1;
					Invalidate();
				}
			
				if (items.Count > 0)
				{
					//  rows
					GenerateRowsRect();				
					if (MouseInRect(e, rowsRect))
					{					
						GenerateRowRects();
						for (i=0; i<items.Count; i++)
						{					
							if (MouseInRect(e, rowRects[i]))
							{
								items[i].Hovered = true;
								lastRowHovered = i;
								break;
							}
							items[i].Hovered = false;
						}
						Invalidate();
					}
				}
			}

			if (++mouseMoveTicks > 10)
			{
				mouseMoveTicks = 0;
				Thread.Sleep(1);
			}
		}


		protected override void OnMouseDown(MouseEventArgs e)
		{
			base.OnMouseDown(e);

			if (!isFocused)
			{
				this.Focus();
				
				if (!captureFocusClick)
					return;
			}

			lastClickedPoint = new Point(e.X, e.Y);

			#region Columns Headers

			// determine if a header was pressed
			if (headerStyle != ColumnHeaderStyle.None)
			{
				if (MouseInRect(e, headerRect) && e.Button == MouseButtons.Left)
				{
					for (int i=0; i<columns.Count; i++)
					{
						columns[i].Pressed = false;						
						if (MouseInRect(e, columnSizeRects[i]) && items.Count > 0)
						{
							if (columns[i].ScaleStyle == ColumnScaleStyle.Slide)
							{
								// autosize column
								if (e.Clicks == 2 && e.Button == MouseButtons.Left && items.Count > 0)
								{
									int mwid = 0;
									int twid = 0;
									for (int j=0; j<items.Count; j++)
									{
										if (i > 0 && items[j].SubItems.Count > 0)
											twid = GetStringWidth(items[j].SubItems[i-1].Text) + 10;
										else if (i == 0)
											twid = GetStringWidth(items[j].Text) + 10;
										twid += 5;
										if (twid > mwid)
											mwid = twid;
									}
								
									twid = GetStringWidth(columns[i].Text);
									if (twid > mwid)
										mwid = twid;

									mwid += 5;

									columns[i].Width = mwid;
								}
									// scale column
								else
								{
									colScaleMode = true;
									colScaleWid = columnRects[i].Width;
									scaledCol = i;
								}
							}
						}
						else if (MouseInRect(e, columnRects[i]) && !MouseInRect(e, columnSizeRects[i]))
						{
							columns[i].Pressed = true;
							lastColPressed = i;
						}
					}
					Invalidate();
					return;
				}
			}

			#endregion


			// determine if a row was pressed			
			if (e.Button == MouseButtons.Left || e.Button == MouseButtons.Right)
			{
				if (MouseInRect(e, rowsRect) && items.Count>0)
				{
					#region Rows

					GenerateRowRects();

					for (int i=0; i<items.Count; i++)
					{

						if (MouseInRect(e, rowRects[i]))
						{
							// Found the rect in wich the user clicked so we have the item to select
							switch(this.multiSelectMode)
							{
								case MultiSelectMode.Single:
									this.MoveToIndex(i);
									break;
								case MultiSelectMode.Range:
									this.MoveToIndex(i);
									break;
								case MultiSelectMode.Selective:
									SelectiveSelection(i);
									break;
							}
						}
					}

				
					#endregion
				}		
			}
		}

		protected override void OnMouseUp(MouseEventArgs e)
		{
			int i;

			base.OnMouseUp(e);

			lastClickedPoint = Point.Empty;			

			if (colScaleMode)
			{
				//columns[scaledCol].Width += colScalePos;
				colScaleMode = false;
				colScalePos = 0;
				scaledCol = -1;
				colScaleWid = 0;

				AdjustScrollbars();
			}

			if (lastColPressed >= 0)
			{				
				columns[lastColPressed].Pressed = false;
				if (MouseInRect(e, columnRects[lastColPressed]) && !MouseInRect(e, columnSizeRects[lastColPressed]) && e.Button == MouseButtons.Left)
				{
					// invoke column click event
					OnColumnClick(new ColumnClickEventArgs(lastColPressed));

					// change currently selected column
					if (lastColSelected >= 0)
						columns[lastColSelected].Selected = false;

					columns[lastColPressed].Selected = true;					
					lastColSelected = lastColPressed;
				}
			}			
			lastColPressed = -1;

			// Check for context click
			if (e.Button == MouseButtons.Right)
			{
				if (MouseInRect(e, headerRect))
					OnHeaderMenuEvent(e);
				else if (MouseInRect(e, rowsRect))
				{
					for (i=0; i<items.Count; i++)
					{
						if (MouseInRect(e, rowRects[i]))
						{
							OnItemMenuEvent(e);
							break;
						}
					}
					if (i>=items.Count)
						OnContextMenuEvent(e);
				}
				else
					OnContextMenuEvent(e);
			}
		}

		protected override void OnMouseWheel(MouseEventArgs e)
		{
			if (e.Delta > 0)
			{
				if (vscrollBar.Visible)
					vscrollBar.Value = (vscrollBar.Value-vscrollBar.SmallChange*(e.Delta/100) < 0 ? 0 : vscrollBar.Value-vscrollBar.SmallChange*(e.Delta/100));
				else if (hscrollBar.Visible)
					hscrollBar.Value = (hscrollBar.Value-hscrollBar.SmallChange*(e.Delta/100) < 0 ? 0 : hscrollBar.Value-hscrollBar.SmallChange*(e.Delta/100));
			}
			else if (e.Delta < 0)
			{
				if (vscrollBar.Visible)
					vscrollBar.Value = (vscrollBar.Value-vscrollBar.SmallChange*(e.Delta/100) > vscrollBar.Maximum-vscrollBar.LargeChange ? vscrollBar.Maximum-vscrollBar.LargeChange : vscrollBar.Value-vscrollBar.SmallChange*(e.Delta/100));
				else if (hscrollBar.Visible)
					hscrollBar.Value = (hscrollBar.Value-hscrollBar.SmallChange*(e.Delta/100) > hscrollBar.Maximum-hscrollBar.LargeChange ? hscrollBar.Maximum-hscrollBar.LargeChange : hscrollBar.Value-hscrollBar.SmallChange*(e.Delta/100));
			}
		}

		protected override void OnKeyDown(KeyEventArgs e)
		{
			
			OnCheckShiftState(e);

			switch (e.KeyCode)
			{
				case Keys.Home:
					OnPageKeys(e);
					break;
				case Keys.End:
					OnPageKeys(e);
					break;
				case Keys.PageUp:
					OnPageKeys(e);
					break;
				case Keys.PageDown:
					OnPageKeys(e);
					break;
				case Keys.Up:
					OnUpDownKeys(e);
					break;
				case Keys.Down:
					OnUpDownKeys(e);
					break;
				default:
					base.OnKeyDown(e);
					break;
			}

			Invalidate(ClientRectangle);
		}

		protected virtual void OnPageKeys(KeyEventArgs e)
		{
			switch (e.KeyCode)
			{
				case Keys.Home:
					if (vscrollBar.Visible)
						vscrollBar.Value = 0;
					else 
					{
						MoveToIndex(0);
					}
					if (hscrollBar.Visible)
						hscrollBar.Value = 0;
					e.Handled = true;
					break;

				case Keys.End:
					if (vscrollBar.Visible)
						vscrollBar.Value = vscrollBar.Maximum-vscrollBar.LargeChange;
					else 
					{
						MoveToIndex(this.items.Count-1);
					}
					if (hscrollBar.Visible)
						hscrollBar.Value = hscrollBar.Maximum-hscrollBar.LargeChange;
					e.Handled = true;
					break;

				case Keys.PageUp:
					if (vscrollBar.Visible)
						vscrollBar.Value = (vscrollBar.LargeChange > vscrollBar.Value ? 0 : vscrollBar.Value-vscrollBar.LargeChange);
					else 
					{
						MoveToIndex(0);
					}
					e.Handled = true;
					break;

				case Keys.PageDown:
					if (vscrollBar.Visible)
						vscrollBar.Value = (vscrollBar.Value+vscrollBar.LargeChange > vscrollBar.Maximum-vscrollBar.LargeChange ? vscrollBar.Maximum-vscrollBar.LargeChange : vscrollBar.Value+vscrollBar.LargeChange);
					else
					{
						MoveToIndex(this.items.Count-1);
					}
					e.Handled = true;
					break;

			}
		}

		protected virtual void OnUpDownKeys(KeyEventArgs e)
		{
			if (focusedItem != null && items.Count > 0)
			{
				int iIndex = focusedIndex;

				switch (e.KeyCode)
				{
					case Keys.Down:
						iIndex++;
						break;

					case Keys.Up:
						iIndex--;
						break;
				}

				MoveToIndex(iIndex);
				e.Handled = true;
			}
		}

		protected virtual void OnCheckShiftState(KeyEventArgs e)
		{
			if (multiSelect)
			{
				if (e.KeyCode == Keys.ControlKey)
				{
					multiSelectMode = MultiSelectMode.Selective;
				}
				else if (e.KeyCode == Keys.ShiftKey)
				{
					multiSelectMode = MultiSelectMode.Range;
				}
			}

			if (!multiSelect && e.KeyCode == Keys.Return)
			{
				OnItemActivate(new EventArgs());
			}
		}

		protected override void OnKeyUp(KeyEventArgs e)
		{
			base.OnKeyUp(e);
			if (!e.Shift)
			{
				multiSelectMode = MultiSelectMode.Single;
			}
		}
		protected override void OnGotFocus(EventArgs e)
		{
			base.OnGotFocus(e);
			isFocused = true;
			Invalidate(this.ClientRectangle);
		}

		protected override void OnLostFocus(EventArgs e)
		{
			base.OnLostFocus(e);
			isFocused = false;
			Invalidate(this.ClientRectangle);
		}
		#endregion

		#region Helper Functions
		// wireing of child control events
		protected virtual void Attach()
		{
			items.MouseDown += new MouseEventHandler(OnSubControlMouseDown);
			columns.WidthResized += new EventHandler(OnColumnWidthResize);

			hscrollBar.ValueChanged += new EventHandler(OnScroll);
			vscrollBar.ValueChanged += new EventHandler(OnScroll);
		}

		protected virtual void Detach()
		{
			items.MouseDown -= new MouseEventHandler(OnSubControlMouseDown);
			columns.WidthResized -= new EventHandler(OnColumnWidthResize);

			hscrollBar.ValueChanged -= new EventHandler(OnScroll);
			vscrollBar.ValueChanged -= new EventHandler(OnScroll);
		}

		// Rectangle and region generation functions
		protected void GenerateColumnRects()
		{
			columnRects = new Rectangle[columns.Count];
			columnSizeRects = new Rectangle[columns.Count];
			int lwidth = 2-hscrollBar.Value;
			int colwid = 0;
			allColsWidth = 0;

			CalcSpringWids(ClientRectangle);
			for (int i=0; i<columns.Count; i++)
			{
				colwid = (columns[i].ScaleStyle == ColumnScaleStyle.Spring ? springWid : columns[i].Width);
				columnRects[i] = new Rectangle(lwidth, 2, colwid, 20);
				columnSizeRects[i] = new Rectangle(lwidth+colwid-4, ClientRectangle.Top+2, 4, headerBuffer);
				lwidth += colwid+1;
				allColsWidth += colwid;
			}
		}

		protected void GenerateHeaderRect()
		{
			headerRect = new Rectangle(this.ClientRectangle.Left+2-hscrollBar.Value, this.ClientRectangle.Top+2, this.ClientRectangle.Width-4, 20);
		}

		protected void GenerateRowsRect()
		{
			rowsRect = new Rectangle(this.ClientRectangle.Left+2-hscrollBar.Value, this.ClientRectangle.Top+(headerStyle == ColumnHeaderStyle.None ? 2 : 22), this.ClientRectangle.Width-4, this.ClientRectangle.Height-(headerStyle == ColumnHeaderStyle.None ? 2 : 22));
		}

		protected void GenerateRowRects()
		{
			rowRects = new Rectangle[items.Count];
			int lheight = 2+headerBuffer-vscrollBar.Value;
			int lftpos = ClientRectangle.Left+2;
			allRowsHeight = items.Count*rowHeight;
			for (int i=0; i<items.Count; i++)
			{
				rowRects[i] = new Rectangle(lftpos, lheight, ClientRectangle.Width-4, rowHeight-1);
				lheight += rowHeight;				
			}
		}

		// Adjust scroll bar settings and visibility
		private int vsize, hsize;
		public virtual void AdjustScrollbars()
		{
			if (items.Count > 0 || columns.Count > 0 && !colScaleMode)
			{
				allColsWidth = 0;
				for (int i=0; i<columns.Count; i++)
				{
					allColsWidth += columns[i].Width;
				}

				allRowsHeight = items.Count*rowHeight;

				vsize = vscrollBar.Width;
				hsize = hscrollBar.Height;

				hscrollBar.Left = this.ClientRectangle.Left+2;
				hscrollBar.Width = this.ClientRectangle.Width-vsize-4;
				hscrollBar.Top = this.ClientRectangle.Top+this.ClientRectangle.Height-hscrollBar.Height-2;
				hscrollBar.Maximum = allColsWidth;
				hscrollBar.LargeChange = (this.ClientRectangle.Width - vsize - 4 > 0 ? this.ClientRectangle.Width - vsize - 4 : 0);
				if (allColsWidth > this.ClientRectangle.Width-4-vsize)
					hscrollBar.Show();
				else
				{
					hscrollBar.Hide();
					hsize = 0;
					hscrollBar.Value = 0;
				}

				vscrollBar.Left = this.ClientRectangle.Left+this.ClientRectangle.Width-vscrollBar.Width-2;
				vscrollBar.Top = this.ClientRectangle.Top+headerBuffer+2;
				vscrollBar.Height = this.ClientRectangle.Height-hsize-headerBuffer-4;
				vscrollBar.Maximum = allRowsHeight;
				vscrollBar.LargeChange = (this.ClientRectangle.Height-headerBuffer-hsize-4 > 0 ? this.ClientRectangle.Height-headerBuffer-hsize-4 : 0);
				if (allRowsHeight > this.ClientRectangle.Height-headerBuffer-4-hsize)
					vscrollBar.Show();
				else
				{
					vscrollBar.Hide();
					vsize = 0;
					vscrollBar.Value = 0;
				}

				hscrollBar.Width = this.ClientRectangle.Width-vsize-4;
				hscrollBar.Top = this.ClientRectangle.Top+this.ClientRectangle.Height-hscrollBar.Height-2;
				hscrollBar.LargeChange = (this.ClientRectangle.Width - vsize - 4 > 0 ? this.ClientRectangle.Width - vsize - 4 : 0);
				if (allColsWidth > this.ClientRectangle.Width-4-vsize)
					hscrollBar.Show();
				else
				{
					hscrollBar.Hide();
					hsize = 0;
					hscrollBar.Value = 0;
				}
			}
		}
		// mouse movement/click helpers
		private void UnselectAll()
		{
			for (int i=0; i<items.Count; i++)
			{
				items[i].Focused = false;
				items[i].Selected = false;
			}
		}

		protected bool MouseInRect(MouseEventArgs me, Rectangle rect)
		{
			if ((me.X >= rect.Left && me.X <= rect.Left+rect.Width) 
				&& (me.Y >= rect.Top && me.Y <= rect.Top + rect.Height))
			{
				return true;
			}

			return false;
		}

		protected void MakeSelectedVisible()
		{
			if (focusedIndex > -1 && ensureVisible)
			{
				ContainerListViewItem item = items[focusedIndex];
				if (item != null && item.Focused && item.Selected)
				{
					Rectangle r = ClientRectangle;
					int i = items.IndexOf(item);
					int pos = r.Top+(rowHeight*i)+headerBuffer+2-vscrollBar.Value;
					try
					{

						if (pos+rowHeight+rowHeight > r.Top+r.Height)
						{
							vscrollBar.Value += Math.Abs((r.Top+r.Height)-(pos+rowHeight+rowHeight));
						}
						else if (pos < r.Top+headerBuffer)
						{
							vscrollBar.Value -= Math.Abs(r.Top+headerBuffer-pos);
						}
					}
					catch (ArgumentException)
					{
						if (vscrollBar.Value > vscrollBar.Maximum)
							vscrollBar.Value = vscrollBar.Maximum;
						else if (vscrollBar.Value < vscrollBar.Minimum)
							vscrollBar.Value = vscrollBar.Minimum;
					}
				}
			}
		}

		protected int GetStringWidth(string s)
		{
			Graphics g = Graphics.FromImage(new Bitmap(32, 32));
			SizeF strSize = g.MeasureString(s, this.Font);
			return (int)strSize.Width;
		}
		protected string TruncatedString(string s, int width, int offset, Graphics g)
		{
			string sprint = "";
			int swid;
			int i;
			SizeF strSize;

			try
			{
				strSize = g.MeasureString(s, this.Font);
				swid = ((int)strSize.Width);
				i=s.Length;

				for (i=s.Length; i>0 && swid > width-offset; i--)
				{
					strSize = g.MeasureString(s.Substring(0, i), this.Font);
					swid = ((int)strSize.Width);				
				}
			
				if (i < s.Length)
					if (i-3 <= 0)
						sprint = s.Substring(0, 1) + "...";
					else
						sprint = s.Substring(0, i-3) + "...";
				else
					sprint = s.Substring(0, i);
			}
			catch
			{
			}

			return sprint;
		}

		private void ShowSelectedItems()
		{
			if (firstSelected == focusedIndex)
			{
				ShowSelectedItem(firstSelected);
			} 
			else
				if (firstSelected > focusedIndex)
			{
				for(int iCtr=firstSelected; iCtr >= focusedIndex; iCtr--)
				{
					items[iCtr].Selected = true;
					selectedIndices.Add(iCtr);						
					selectedItems.Add(items[iCtr]);
				}
			} 
			else
				if (firstSelected < focusedIndex)
			{
				for(int iCtr=firstSelected; iCtr <= focusedIndex; iCtr++)
				{
					items[iCtr].Selected = true;
					selectedIndices.Add(iCtr);						
					selectedItems.Add(items[iCtr]);
				}

			}

		}

		private void ShowSelectedItem(int iCtr)
		{
			items[iCtr].Selected = true;
			selectedIndices.Add(iCtr);						
			selectedItems.Add(items[iCtr]);
		}

		private void MoveToIndex(int iIndex)
		{
			if ((iIndex < -1) | (iIndex >= this.items.Count)) return;

			if (iIndex == focusedIndex) return; /// Might occur on End on first item or
												///             on Home on the first item or
												///             if the user clicks on the current node

			UnselectAll();

			this.selectedItems.Clear();
			this.selectedIndices.Clear();

			if (focusedItem != null)
			{
				focusedItem.Selected = false;
				focusedItem.Focused = false;
			}

			focusedIndex = iIndex;

			if ((this.multiSelectMode == MultiSelectMode.Single) | (firstSelected == -1))
			{
				firstSelected = focusedIndex;
				items[focusedIndex].Focused = true;
				focusedItem = items[focusedIndex];
			}

			ShowSelectedItems();

			MakeSelectedVisible();
			OnSelectedIndexChanged(new EventArgs());

			Invalidate(this.ClientRectangle);
		}

		private void SelectiveSelection(int iIndex)
		{
			// This is a special case and will be used when the user clicks on an item
			// while the control key is pressed or presses the space bar button while an
			// item has the focus.

			// unfocus the previously focused item
			if (focusedIndex >= 0 && focusedIndex < items.Count)
				items[focusedIndex].Focused = false;

			ContainerListViewItem item = items[iIndex];

			if (items[iIndex].Selected)  // Already selected? De-select.
			{
				items[iIndex].Focused = false;
				items[iIndex].Selected = false;

				int i = selectedItems[item];
				selectedIndices.Remove(i);
				selectedItems.Remove(item);

				if (this.focusedItem == item) focusedItem = null;

				MakeSelectedVisible();

				OnSelectedIndexChanged(new EventArgs());
			}
			else
			{
				item.Focused = true;
				item.Selected = true;

				selectedIndices.Add(iIndex);
				selectedItems.Add(item);
				this.focusedItem = item;

				MakeSelectedVisible();

				OnSelectedIndexChanged(new EventArgs());
			}

			Invalidate();
		}

		// rendering helpers
		protected int springWid = 0;
		protected int springCnt = 0;

		protected void CalcSpringWids(Rectangle r)
		{
			springCnt = 0;
			springWid = (r.Width-borderWid*2);
			for (int i=0; i<columns.Count; i++)
			{
				if (columns[i].ScaleStyle == ColumnScaleStyle.Slide)
					springWid -= columns[i].Width;
				else
					springCnt++;
			}

			if (springCnt > 0 && springWid > 0)
				springWid = springWid/springCnt;
		}

		protected virtual void DrawBorder(Graphics g, Rectangle r)
		{
			// if running in XP with styles
			if (VisualStyles)
			{
				DrawBorderStyled(g, r);
				return;
			}

			Rectangle rect = this.ClientRectangle;
			if (borderstyle == BorderStyle.FixedSingle)
			{
				g.DrawRectangle(SystemPens.ControlDarkDark, r.Left, r.Top, r.Width, r.Height);
			}
			else if (borderstyle == BorderStyle.Fixed3D)
			{
				ControlPaint.DrawBorder3D(g, r.Left, r.Top, r.Width, r.Height, Border3DStyle.Sunken);
			}
			else if (borderstyle == BorderStyle.None)
			{
				// do not render any border
			}
		}

		protected virtual void DrawBackground(Graphics g, Rectangle r)
		{
			int i;
			int lwidth = 2, lheight=1;

			g.FillRectangle(new SolidBrush(BackColor), r);

			// Selected Column
			if (headerStyle == ColumnHeaderStyle.Clickable)
			{
				for (i=0; i<columns.Count; i++)
				{
					if (columns[i].Selected)
					{
						g.FillRectangle(new SolidBrush(colSortColor), r.Left+lwidth-hscrollBar.Value, r.Top+2+headerBuffer, columns[i].Width, r.Height-4-headerBuffer);
						break;
					}
					lwidth += columns[i].Width;
				}
			}

			// hot-tracked column
			if (doColTracking && (lastColHovered >= 0 && lastColHovered < columns.Count))
			{
				g.FillRectangle(new SolidBrush(colTrackColor), columnRects[lastColHovered].Left, 22, columnRects[lastColHovered].Width, r.Height-22);
			}

			// hot-tracked row
			if (doRowTracking && (lastRowHovered >= 0 && lastRowHovered < items.Count))
			{
				g.FillRectangle(new SolidBrush(rowTrackColor), r.Left+2, rowRects[lastRowHovered].Top, r.Left+r.Width-4, rowHeight);
			}		

			// gridlines
			if (gridLines)
			{
				Pen p = new Pen(new SolidBrush(gridLineColor), 1.0f);
				lwidth = lheight = 1;

				// vertical
				for (i=0; i<columns.Count; i++)
				{
					if (r.Left+lwidth+columns[i].Width >= r.Left+r.Width-2)
						break;

					g.DrawLine(p, r.Left+lwidth+columns[i].Width-hscrollBar.Value, r.Top+2+headerBuffer, r.Left+lwidth+columns[i].Width-hscrollBar.Value, r.Top+r.Height-2); 
					lwidth += columns[i].Width;
				}
				
				// horizontal
				for (i=0; i<items.Count; i++)
				{
					g.DrawLine(p, r.Left+2, r.Top+headerBuffer+rowHeight+lheight-vscrollBar.Value, r.Left+r.Width, r.Top+headerBuffer+rowHeight+lheight-vscrollBar.Value);
					lheight += rowHeight;
				}
			}
		}

		protected virtual void DrawHeaders(Graphics g, Rectangle r)
		{
			// if running in XP with styles
			if (VisualStyles)
			{
				DrawHeadersStyled(g, r);
				return;
			}

			if (headerStyle != ColumnHeaderStyle.None)
			{
				g.FillRectangle(new SolidBrush(SystemColors.Control), r.Left+2, r.Top+2, r.Width-2, headerBuffer);

				CalcSpringWids(r);

				// render column headers and trailing column header
				int last = 2;
				int i;

				int lp_scr = r.Left-hscrollBar.Value;
                
				g.Clip = new Region(new Rectangle(r.Left+2, r.Top+2, r.Width-5, r.Top+headerBuffer));
				for (i=0; i<columns.Count; i++)
				{
					if ((lp_scr+last+columns[i].Width > r.Left+2)
						&& (lp_scr+last < r.Left+r.Width-2))
					{						
						if (headerStyle == ColumnHeaderStyle.Clickable && columns[i].Pressed)
							System.Windows.Forms.ControlPaint.DrawButton(g, lp_scr+last, r.Top+2, columns[i].Width, r.Top+headerBuffer, ButtonState.Flat);
						else
							System.Windows.Forms.ControlPaint.DrawButton(g, lp_scr+last, r.Top+2, columns[i].Width, r.Top+headerBuffer, ButtonState.Normal);
					
						if (columns[i].Image != null)
						{
							g.DrawImage(columns[i].Image, new Rectangle(lp_scr+last+4, r.Top+3, 16, 16));						
							g.DrawString(TruncatedString(columns[i].Text, columns[i].Width, 25, g), this.Font, SystemBrushes.ControlText, (float)(lp_scr+last+22), (float)(r.Top+5));
						}
						else
						{
							string sp = "";
							if (columns[i].TextAlign == HorizontalAlignment.Left)
								g.DrawString(TruncatedString(columns[i].Text, columns[i].Width, 0, g), this.Font, SystemBrushes.ControlText, (float)(lp_scr+last+4), (float)(r.Top+5));
							else if (columns[i].TextAlign == HorizontalAlignment.Right)
							{
								sp = TruncatedString(columns[i].Text, columns[i].Width, 0, g);
								g.DrawString(sp, this.Font, SystemBrushes.ControlText, (float)(lp_scr+last+columns[i].Width-Helpers.StringTools.MeasureDisplayStringWidth(g, sp, this.Font)-4), (float)(r.Top+5));
							}
							else
							{
								sp = TruncatedString(columns[i].Text, columns[i].Width, 0, g);
								g.DrawString(sp, this.Font, SystemBrushes.ControlText, (float)(lp_scr+last+(columns[i].Width/2)-(Helpers.StringTools.MeasureDisplayStringWidth(g, sp, this.Font)/2)), (float)(r.Top+5));
							}
						}
					}
					last += columns[i].Width;
				}

				// only render trailing column header if the end of the
				// last column ends before the boundary of the listview 
				if (!(lp_scr+last+5 > r.Left+r.Width))
				{
					g.Clip = new Region(new Rectangle(r.Left+2, r.Top+2, r.Width-5, r.Top+headerBuffer));
					System.Windows.Forms.ControlPaint.DrawButton(g, lp_scr+last, r.Top+2, r.Width-(r.Left+last)-3+hscrollBar.Value , r.Top+headerBuffer, ButtonState.Normal);
				}
			}
		}		

		protected virtual void DrawRows(Graphics g, Rectangle r)
		{
			// Don't paint if in transaction
			if (this.InUpdateTransaction) return;

			CalcSpringWids(r);

			if (columns.Count > 0)
			{
				// render listview item rows
				int last;
				int j, i;

				// set up some commonly used values
				// to cut down on cpu cycles and boost
				// the lists performance
				int lp_scr = r.Left+2-hscrollBar.Value;	// left viewport position less scrollbar position
				int lp = r.Left+2;						// left viewport position
				int tp_scr = r.Top+2+headerBuffer-vscrollBar.Value;	// top viewport position less scrollbar position
				int tp = r.Top+2+headerBuffer;			// top viewport position

				for (i=0; i<items.Count; i++)
				{
					j=0;
					last = 0;

					int iColWidth = (columns[j].ScaleStyle == ColumnScaleStyle.Spring ? springWid : columns[j].Width); // BMS 2003-05-24

					// render item, but only if its within the viewport
					if ((tp_scr+(rowHeight*i)+2 > r.Top+2) 
						&& (tp_scr+(rowHeight*i)+2 < r.Top+r.Height-2-hsize))
					{
						g.Clip = new Region(new Rectangle(r.Left+2, r.Top+headerBuffer+2, r.Width-vsize-5, r.Height-hsize-5));

						int rowSelWidth = (allColsWidth < (r.Width-5) || hscrollBar.Visible ? allColsWidth : r.Width-5);
						if (!fullRowSelect)
							rowSelWidth = iColWidth /* BMS 2003-05-24 */ -2;

						// render selected item highlights
						if (items[i].Selected && isFocused)
						{
							g.FillRectangle(new SolidBrush(rowSelectColor), lp, tp_scr+(rowHeight*i), rowSelWidth, rowHeight);
						}
						else if (items[i].Selected && !isFocused && hideSelection)
						{
							ControlPaint.DrawFocusRectangle(g, new Rectangle(lp_scr, tp_scr+(rowHeight*i), rowSelWidth, rowHeight));
						}
						else if (items[i].Selected && !isFocused && !hideSelection)
						{
							g.FillRectangle(SystemBrushes.Control, lp, tp_scr+(rowHeight*i), rowSelWidth, rowHeight);
						}

						if (items[i].Focused && multiSelect && isFocused)
						{
							ControlPaint.DrawFocusRectangle(g, new Rectangle(lp_scr, tp_scr+(rowHeight*i), rowSelWidth, rowHeight));
						}

						// render item
						if ((lp_scr+2+iColWidth /* BMS 2003-05-24 */  > r.Left+4))
						{	
							g.Clip = new Region(new Rectangle(lp+2, tp, (iColWidth /* BMS 2003-05-24 */ > r.Width ? r.Width-6 : iColWidth /* BMS 2003-05-24 */ -2), r.Height-hsize-5));

							if (smallImageList != null && (items[i].ImageIndex >= 0 && items[i].ImageIndex < smallImageList.Images.Count))
							{
								smallImageList.Draw(g, lp_scr+4, tp_scr+(rowHeight*i)+1, 16, 16, items[i].ImageIndex);					
								g.DrawString(TruncatedString(items[i].Text, iColWidth /* BMS 2003-05-24 */ , 18, g), this.Font, (items[i].Selected && isFocused ? SystemBrushes.HighlightText : new SolidBrush(ForeColor)), (float)(lp_scr+22), (float)(tp_scr+(rowHeight*i)+2));
							}
							else
							{
								g.DrawString(TruncatedString(items[i].Text, iColWidth /* BMS 2003-05-24 */ , 0, g), this.Font, (items[i].Selected && isFocused ? SystemBrushes.HighlightText : new SolidBrush(ForeColor)), (float)(lp_scr+4), (float)(tp_scr+(rowHeight*i)+2));
							}
						}
					}

					// render sub items
					if (columns.Count > 0)
					{
						for (j=0; j<items[i].SubItems.Count && j<columns.Count-1; j++)
						{
							iColWidth = (columns[j].ScaleStyle == ColumnScaleStyle.Spring ? springWid : columns[j].Width);
							int iColWidthPlus1 = (columns[j+1].ScaleStyle == ColumnScaleStyle.Spring ? springWid : columns[j+1].Width);
							last += iColWidth;  // BMS 2003-05-24

							// only render subitem if it is visible within the viewport
							if ((lp_scr+2+last+iColWidthPlus1     /* BMS 2003-05-24 */ > r.Left+4) 
								&& (lp_scr+last < r.Left+r.Width-4)
								&& (tp_scr+(rowHeight*i)+2 >= tp) /* BMS 2003-05-25 */
								&& (tp_scr+(rowHeight*i)+2 < r.Top+r.Height-2-hsize))
							{
								g.Clip = new Region(new Rectangle(lp_scr+last+4, tp, (last+iColWidthPlus1 /* BMS 2003-05-24 */ > r.Width-6 ? r.Width-6 : iColWidthPlus1-6), r.Height-hsize-5)); 
								Control c = items[i].SubItems[j].ItemControl;
								if (c != null)
								{
									c.Visible = true; // (tp_scr+(rowHeight*i)+2) > tp;  // Can only be visible when the 
									c.Location = new Point(lp_scr+last+2, tp_scr+(rowHeight*i)+2);
									c.ClientSize = new Size(iColWidthPlus1 /* BMS 2003-05-24 */ -6, rowHeight-4); 
									c.Parent = this;
								}						
								else
								{
									string sp = "";
									if (columns[j+1].TextAlign == HorizontalAlignment.Left)
									{
										g.DrawString(TruncatedString(items[i].SubItems[j].Text, iColWidthPlus1 /* BMS 2003-05-24 */, 12, g), this.Font, (items[i].Selected && isFocused ? SystemBrushes.HighlightText : SystemBrushes.WindowText), (float)(lp_scr+last+4), (float)(tp_scr+(rowHeight*i)+2));
									}
									else if (columns[j+1].TextAlign == HorizontalAlignment.Right)
									{
										sp = TruncatedString(items[i].SubItems[j].Text, iColWidthPlus1 /* BMS 2003-05-24 */, 12, g);
										g.DrawString(sp, this.Font, (items[i].Selected && isFocused ? SystemBrushes.HighlightText : new SolidBrush(ForeColor)), (float)(lp_scr+last+iColWidthPlus1 /* BMS 2003-05-24 */-Helpers.StringTools.MeasureDisplayStringWidth(g, sp, this.Font)-2), (float)(tp_scr+(rowHeight*i)+2));
									}
									else
									{
										sp = TruncatedString(items[i].SubItems[j].Text, iColWidthPlus1 /* BMS 2003-05-24 */ , 12, g);
										g.DrawString(sp, this.Font, (items[i].Selected && isFocused ? SystemBrushes.HighlightText : new SolidBrush(ForeColor)), (float)(lp_scr+last+(iColWidthPlus1 /* BMS 2003-05-24 */ /2)-(Helpers.StringTools.MeasureDisplayStringWidth(g, sp, this.Font)/2)), (float)(tp_scr+(rowHeight*i)+2));
									}
								}
							}
							else /* BMS 2003-05-25 */
							{
								Control c = items[i].SubItems[j].ItemControl;
								if (c != null)
								{
									c.Visible = false;
								}						
							}
						}
					}
				}
			}
		}

		protected virtual void DrawExtra(Graphics g, Rectangle r)
		{
			if (hscrollBar.Visible && vscrollBar.Visible)
			{
				g.ResetClip();
				g.FillRectangle(SystemBrushes.Control, r.Width-vscrollBar.Width-borderWid, r.Height-hscrollBar.Height-borderWid, vscrollBar.Width, hscrollBar.Height);
			}
		}
		// visual styles rendering functions
		protected virtual void DrawBorderStyled(Graphics g, Rectangle r)
		{
			Region oldreg = g.Clip;
			g.Clip = new Region(r);
			g.DrawRectangle(new Pen(SystemBrushes.InactiveBorder), r.Left, r.Top, r.Width-1, r.Height-1);
			g.DrawRectangle(new Pen(BackColor), r.Left+1, r.Top+1, r.Width-3, r.Height-3);
			g.Clip = oldreg;
		}

		protected virtual void DrawHeadersStyled(Graphics g, Rectangle r)
		{
			if (headerStyle != ColumnHeaderStyle.None)
			{
				int colwid = 0;
				int i;
				int last = 2;
				CalcSpringWids(r);

				int lp_scr = r.Left-hscrollBar.Value;
				int lp = r.Left;
				int tp = r.Top+2;

				System.IntPtr hdc = g.GetHdc();
				try
				{
					// render column headers and trailing column header					
					for (i=0; i<columns.Count; i++)
					{
						colwid = (columns[i].ScaleStyle == ColumnScaleStyle.Spring ? springWid : columns[i].Width);
						if (headerStyle == ColumnHeaderStyle.Clickable && columns[i].Pressed)
							Lyquidity.Controls.ExtendedListViews.uxTheme.Wrapper.DrawBackground("HEADER", "HEADERITEM", "PRESSED", hdc,
								lp_scr+last, tp, 
								colwid, headerBuffer,
								lp, tp, r.Width-6, headerBuffer);
						else if (headerStyle != ColumnHeaderStyle.None && columns[i].Hovered)
							Lyquidity.Controls.ExtendedListViews.uxTheme.Wrapper.DrawBackground("HEADER", "HEADERITEM", "HOT", hdc,
								lp_scr+last, tp, 
								colwid, headerBuffer,  
								lp, tp, r.Width-6, headerBuffer);
						else
							Lyquidity.Controls.ExtendedListViews.uxTheme.Wrapper.DrawBackground("HEADER", "HEADERITEM", "NORMAL", hdc,
								lp_scr+last, tp, 
								colwid, headerBuffer,  
								lp, tp, r.Width-6, headerBuffer);
						last += colwid;
					}
					// only render trailing column header if the end of the
					// last column ends before the boundary of the listview 
					if (!(r.Left+last+2-hscrollBar.Value > r.Left+r.Width))
					{
						Lyquidity.Controls.ExtendedListViews.uxTheme.Wrapper.DrawBackground("HEADER", "HEADERITEM", "NORMAL", hdc, lp_scr+last, tp, r.Width-last-2+hscrollBar.Value, headerBuffer,  r.Left, /* r.Top BMS 2003/05/22 */ tp, r.Width, headerBuffer);
					}
				}
				catch
				{
				}
				finally
				{
					g.ReleaseHdc(hdc);
				}

				last = 1;
				for (i=0; i<columns.Count; i++)
				{
					colwid = (columns[i].ScaleStyle == ColumnScaleStyle.Spring ? springWid : columns[i].Width);
					g.Clip = new Region(new Rectangle(lp_scr+last+2, tp, (r.Left+last+colwid > r.Left+r.Width ? (r.Width - (r.Left+last))-4 : colwid-2)+hscrollBar.Value, r.Top+headerBuffer));
					if (columns[i].Image != null)
					{
						g.DrawImage(columns[i].Image, new Rectangle(lp_scr+last+4, r.Top+3, 16, 16));
						g.DrawString(TruncatedString(columns[i].Text, colwid, 25, g), this.Font, SystemBrushes.ControlText, (float)(r.Left+last+22-hscrollBar.Value), (float)(r.Top+5));
					}
					else
					{
						string sp = TruncatedString(columns[i].Text, colwid, 0, g);
						if (columns[i].TextAlign == HorizontalAlignment.Left)
						{
							g.DrawString(TruncatedString(columns[i].Text, colwid, 0, g), this.Font, SystemBrushes.ControlText, (float)(last+4-hscrollBar.Value), (float)(r.Top+5));
						}
						else if (columns[i].TextAlign == HorizontalAlignment.Right)
						{
							g.DrawString(sp, this.Font, SystemBrushes.ControlText, (float)(last+colwid-Helpers.StringTools.MeasureDisplayStringWidth(g, sp, this.Font)-4-hscrollBar.Value), (float)(r.Top+5));
						}
						else
						{
							g.DrawString(sp, this.Font, SystemBrushes.ControlText, (float)(last+(colwid/2)-(Helpers.StringTools.MeasureDisplayStringWidth(g, sp, this.Font)/2)-hscrollBar.Value), (float)(r.Top+5));
						}
					}
					last += colwid;
				}
			}
		}

		#endregion
	}
	#endregion

	#region Type Converters
	public class ToggleColumnHeaderConverter: TypeConverter
	{
		public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
		{
			if (destinationType == typeof(InstanceDescriptor))
			{
				return true;
			}
			return base.CanConvertTo(context, destinationType);
		}

		public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
		{
			if (destinationType == typeof(InstanceDescriptor) && value is ToggleColumnHeader)
			{
				ToggleColumnHeader lvi = (ToggleColumnHeader)value;

				ConstructorInfo ci = typeof(ToggleColumnHeader).GetConstructor(new Type[] {});
				if (ci != null)
				{
					return new InstanceDescriptor(ci, null, false);
				}
			}
			return base.ConvertTo(context, culture, value, destinationType);
		}
	}
	
	public class ListViewItemConverter: TypeConverter
	{
		public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
		{
			if (destinationType == typeof(InstanceDescriptor))
			{
				return true;
			}
			return base.CanConvertTo(context, destinationType);
		}

		public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
		{
			if (destinationType == typeof(InstanceDescriptor) && value is ContainerListViewItem)
			{
				ContainerListViewItem lvi = (ContainerListViewItem)value;

				ConstructorInfo ci = typeof(ContainerListViewItem).GetConstructor(new Type[] {});
				if (ci != null)
				{
					return new InstanceDescriptor(ci, null, false);
				}
			}
			return base.ConvertTo(context, culture, value, destinationType);
		}
	}

	public class SubListViewItemConverter: TypeConverter
	{
		public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType)
		{
			if (destinationType == typeof(InstanceDescriptor))
			{
				return true;
			}
			return base.CanConvertTo(context, destinationType);
		}

		public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType)
		{
			if (destinationType == typeof(InstanceDescriptor) && value is ContainerSubListViewItem)
			{
				ContainerSubListViewItem lvi = (ContainerSubListViewItem)value;

				ConstructorInfo ci = typeof(ContainerSubListViewItem).GetConstructor(new Type[] {});
				if (ci != null)
				{
					return new InstanceDescriptor(ci, null, false);
				}
			}
			return base.ConvertTo(context, culture, value, destinationType);
		}
	}
	#endregion
}
