// Author:  Bill Seddon
// Company: Lyquidity Solutions Limited
// 
// This work builds on code posted to SourceForge by
// Jon Rista (jrista@hotmail.com)
// 
// TreeListView extends ContainerListView to provide
// a fully-featured hybrid listview that allows the
// first column to behave as a tree.
//
// This version fixes up drawing, mouse and keyboard
// event handling issues.  For example the focus row
// would not track correctly in all circumstances when
// the user used the up and down cursor keys.
//
// The way the tree is built has been changed quite a
// bit to address the performance issues Jon mentioned 
// in his article. These changes allow the drawing 
// routines to be more "intelligent" about the rows to
// be painted.  In the original version, accommodating
// the arbitrary exapansions of nodes meant that the
// list needed to be counted each time the control was
// painted.  This is no issue when the list is small
// but for large lists meant a sluggish response.
//
// In this version, nodes know about the state
// of their chilldren, information that is updated as
// new nodes are added or when existing ones are 
// expanded, collapsed or removed.  This requirement
// puts some additional burden on the control when nodes
// are manipulated (which is relatively infrequently)
// in exchange for lightening the load on the drawing
// routine.
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
using System.IO;
using System.Reflection;
using System.Windows.Forms;

namespace Lyquidity.Controls.ExtendedListViews
{
	#region Enumerations
	#endregion

	#region TreeListView
	/// <summary>
	/// TreeListView provides a hybrid listview whos first
	/// column can behave as a treeview. This control extends
	/// ContainerListView, allowing subitems to contain 
	/// controls.
	/// </summary>
	[ToolboxItem(true)]
	[ToolboxBitmap(typeof(Lyquidity.Controls.ExtendedListViews.TreeListView), "Resources.treeview.bmp")]
	[DefaultEvent("SelectedItemChanged")]
	public class TreeListView : Lyquidity.Controls.ExtendedListViews.ContainerListView
	{
		#region Events

		protected override void OnSubControlMouseDown(object sender, MouseEventArgs e)
		{
			TreeListNode node = (TreeListNode)sender;
			
			UnselectNodes(nodes);
			
			node.Focused = true;
			node.Selected = true;				
			//focusedIndex = firstSelected = i;

			if (e.Clicks >= 2)
				node.Toggle();

			// set selected items and indices collections							
			//selectedIndices.Add(i);						
			//selectedItems.Add(items[i]);
			Invalidate(ClientRectangle);
		}

		protected virtual void OnNodesChanged(object sender, EventArgs e)
		{
			AdjustScrollbars();
		}
		
		protected void OnSelectedItemChanged()
		{
			if (SelectedItemChanged != null)
			{
				SelectedItemChanged(this, new EventArgs());
			}
		}

		#endregion

		#region Variables

		public event EventHandler SelectedItemChanged;
		//private new event EventHandler SelectedIndexChanged;

		protected TreeListNodeCollection nodes;
		protected int indent = 19;
		protected int itemheight = 20;

		protected bool showlines = true, showrootlines = true, showplusminus = true;
		protected bool alwaysShowPM = false;
		private bool mouseActivate = false;
		private bool allCollapsed = false;

		protected ListDictionary pmRects;
		protected ListDictionary nodeRowRects;

		protected Bitmap bmpMinus, bmpPlus;

		private TreeListNode curNode = null;
		private TreeListNode firstSelectedNode = null;
		private TreeListNode virtualParent = null;

		// private SelectedTreeListNodeCollection selectedNodes;

		private System.ComponentModel.Container components = null;

		#endregion

		#region Constructor
		public TreeListView(): base()
		{
			virtualParent = new TreeListNode();
			virtualParent.m_bIsRoot = true;			// BMS 2003-05-27 - so that we know which parent is the root as this 
			                                        //                  is special.  For example, nodes are always visible

			this.SelectedItemChanged = null;
			//this.SelectedIndexChanged = null;

			nodes = virtualParent.Nodes;
			nodes.Owner = virtualParent;
			nodes.MouseDown += new MouseEventHandler(OnSubControlMouseDown);
			nodes.NodesChanged += new EventHandler(OnNodesChanged);

			// selectedNodes = new SelectedTreeListNodeCollection();

			nodeRowRects = new ListDictionary();
			pmRects = new ListDictionary();	

			// Use reflection to load the
			// embedded bitmaps for the
			// styles plus and minus icons
			Assembly myAssembly = Assembly.GetAssembly(Type.GetType("Lyquidity.Controls.ExtendedListViews.TreeListView"));
			Stream bitmapStream1 = myAssembly.GetManifestResourceStream("Lyquidity.Controls.ExtendedListViews.Resources.tv_minus.bmp");
			bmpMinus = new Bitmap(bitmapStream1);

			Stream bitmapStream2 = myAssembly.GetManifestResourceStream("Lyquidity.Controls.ExtendedListViews.Resources.tv_plus.bmp");
			bmpPlus = new Bitmap(bitmapStream2);

		}
		#endregion

		#region Properties

		[
		Browsable(false)
		]
		public SelectedTreeListNodeCollection SelectedNodes
		{
			get { return GetSelectedNodes(virtualParent); }
		}

		[
		Category("Behavior"),
		Description("Determins wether an item is activated or expanded by a double click."),
		DefaultValue(false)
		]
		public bool MouseActivte
		{
			get { return mouseActivate; }
			set { mouseActivate = value; }
		}

		[
		Category("Behavior"),
		Description("Specifies wether to always show plus/minus signs next to each node."),
		DefaultValue(false)
		]
		public bool AlwaysShowPlusMinus
		{
			get { return alwaysShowPM; }
			set { alwaysShowPM = value; }
		}

		[
		Category("Data"), 
		Description("The collection of root nodes in the treelist."),
		DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
		Editor(typeof(CollectionEditor), typeof(UITypeEditor))
		]
		public TreeListNodeCollection Nodes
		{
			get { return nodes; }
		}

		[Browsable(false)]
		private new ContainerListViewItemCollection Items  // Hide this method: should work with Nodes
		{
			get { return items; }
		}

		[
		Category("Behavior"),
		Description("The indentation of child nodes in pixels."),
		DefaultValue(19)
		]
		public int Indent
		{
			get { return indent; }
			set { indent = value; }
		}

		[
		Category("Appearance"),
		Description("The height of every item in the treelistview."),
		DefaultValue(18)
		]
		public int ItemHeight
		{
			get { return itemheight; }
			set { itemheight = value; }
		}

		[
		Category("Behavior"),
		Description("Indicates wether lines are shown between sibling nodes and between parent and child nodes."),
		DefaultValue(false)
		]
		public bool ShowLines
		{
			get { return showlines; }
			set { showlines = value; }
		}

		[
		Category("Behavior"),
		Description("Indicates wether lines are shown between root nodes."),
		DefaultValue(false)
		]
		public bool ShowRootLines
		{
			get { return showrootlines; }
			set { showrootlines = value; }
		}

		[
		Category("Behavior"),
		Description("Indicates wether plus/minus signs are shown next to parent nodes."),
		DefaultValue(true)
		]
		public bool ShowPlusMinus
		{
			get { return showplusminus; }
			set { showplusminus = value; }
		}


		#endregion

		#region Overrides

		protected new void MakeSelectedVisible()
		{
			if ((curNode != null) && ensureVisible)
			{
				TreeListNode item = this.curNode;
				if (item.Selected)
				{
					// Need to work out what the row number of the curNode is in the whole list
					int i = curNode.RowNumber(true)-1;

					Rectangle r = ClientRectangle;
					int pos = r.Top+(itemheight*i)+headerBuffer+2-vscrollBar.Value;

					try
					{
						if (pos+itemheight + ((hscrollBar.Visible) ? hscrollBar.Height : 0) > r.Top+r.Height)
						{
							pos = Math.Abs((r.Top+r.Height)-(pos + itemheight + ((hscrollBar.Visible) ? hscrollBar.Height : 0)));
							pos = (pos > itemheight/2) ? itemheight : 0;
							vscrollBar.Value += pos;
						}
						else if (pos < r.Top+headerBuffer+2)
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

		protected override void OnKeyDown(KeyEventArgs e)
		{
			base.OnCheckShiftState(e);

			if ((e.KeyCode == Keys.Left) || (e.KeyCode == Keys.Right))
				OnLeftRightKeys(e);
			else
				base.OnKeyDown (e);

		}

		protected virtual void OnLeftRightKeys(KeyEventArgs e)
		{
			if (nodes.Count > 0)
			{
				if (curNode != null)
				{
					switch (e.KeyCode)
					{
						#region Left Key

						case Keys.Left:  // collapse current node or move up to parent

							if (curNode.IsExpanded)
							{
								curNode.Collapse();
								AdjustScrollbars();
							}
							else if (curNode.ParentNode() != null)
							{
								TreeListNode t = (TreeListNode)curNode.ParentNode();
								if (t.ParentNode() != null) // never select virtualParent node
								{
									if (this.multiSelectMode == MultiSelectMode.Single)
										curNode = (TreeListNode)curNode.ParentNode();
								}
							}

							break;

						#endregion

						#region Right

						case Keys.Right: // expand current node or move down to first child
							if (!curNode.IsExpanded)
							{
								curNode.Expand();
								AdjustScrollbars();
							}
							else if (curNode.IsExpanded && curNode.Children > 0)
							{
								if (this.multiSelectMode == MultiSelectMode.Single)
									curNode = (TreeListNode)curNode.FirstChild();
							}

							break;

						#endregion
					}

					ShowSelectedItems();
					Invalidate();
					e.Handled = true;

				}
			}
		}

		protected override void OnUpDownKeys(KeyEventArgs e)
		{
			// The basic idea is to unselect everything, work out the new position
			// and then select the nodes between the firstSelectedItem and the curNode
			// When multiSelectMode == MultiSelectMode.Single, the firstSelectedItem
			// will also be the curNode.

			// This process takes the burden of node selection and focusing away 
			// from the code in each "if" statement.  The code in the "if"
			// statements can then focus on just locating the correct node.

			switch (e.KeyCode)
			{ 
				case Keys.Up:
					GetPriorNode(ref curNode);
					break;

				case Keys.Down:
					GetNextNode(ref curNode);
					break;
			}

			if (curNode == null) return;

			if ((this.multiSelectMode == MultiSelectMode.Single) | (firstSelectedNode == null))
			{
				firstSelectedNode = curNode;
				firstSelectedNode.Selected = true;
				firstSelectedNode.Focused = true;
			}

			ShowSelectedItems();
			OnSelectedItemChanged();
			Invalidate();
			e.Handled = true;

		}

		/// <summary>
		/// Gets the prior visible node.
		/// </summary>
		/// <param name="cCurNode"></param>
		/// <returns>Returns TRUE if already on the first node</returns>
		protected bool GetPriorNode(ref TreeListNode cCurNode)
		{
			if (cCurNode == null) return true;

			if (cCurNode.PreviousSibling() == null && cCurNode.ParentNode() != null)
			{
				TreeListNode t = (TreeListNode)cCurNode.ParentNode();
				if (t.ParentNode() != null) // never select virtualParent node
				{
					cCurNode = (TreeListNode)cCurNode.ParentNode();
					return false;
				}

			}
			else if (cCurNode.PreviousSibling() != null)
			{
				TreeListNode t = (TreeListNode)cCurNode.PreviousSibling();
				if (t.Children > 0 && t.IsExpanded)
				{
					do
					{
						t = (TreeListNode)t.LastChild();
						if (!t.IsExpanded | t.Nodes.Count == 0)
						{
							cCurNode = t;
							return false;
						}
					} while (t.Children > 0 && t.IsExpanded);
				}
				else
				{
					cCurNode = (TreeListNode)cCurNode.PreviousSibling();
					return false;
				}
			}
			return true;
		}

		/// <summary>
		/// Gets the next visible node.
		/// </summary>
		/// <param name="cCurNode"></param>
		/// <returns>Returns TRUE if already on the last node</returns>

		private bool GetNextNode(ref TreeListNode cCurNode)
		{
			if (cCurNode == null) return true;
			if (cCurNode.IsExpanded && cCurNode.Children > 0)
			{
				cCurNode = (TreeListNode)cCurNode.FirstChild();
				return false;	
			}
			else if (cCurNode.NextSibling() == null && cCurNode.ParentNode() != null)
			{
				TreeListNode t = cCurNode;
				do
				{
					t = (TreeListNode)t.ParentNode();
					if (t.NextSibling() != null)
					{
						cCurNode = (TreeListNode)t.NextSibling();
						return false;
					}	
				} while (t.NextSibling() == null && t.ParentNode() != null);
			}						
			else if (cCurNode.NextSibling() != null)
			{
				cCurNode = (TreeListNode)cCurNode.NextSibling();							
				return false;
			}

			return true;
		}

		/// <summary>
		/// 
		/// </summary>
		private void ShowSelectedItems()
		{
			if (this.curNode == null) return;

			this.UnselectNodes(nodes);

			// Figure out the path between firstSelectedItem and curNode
			// then select all visible (children of expanded) nodes
			if (firstSelectedNode == curNode)
			{
				curNode.Selected = true;
			}
			else
			{
				// Now set each node from the first to the current node selecting as we go
				int iCurrentNodeIsAbove = this.FirstNodeRelativeToCurrentNode();
				TreeListNode cCurNode = firstSelectedNode;

				cCurNode.Selected = true;
				while (cCurNode != curNode)
				{
					if (iCurrentNodeIsAbove == -1)
					{
						if (this.GetPriorNode(ref cCurNode)) break;
					}
					else
					{
						if (this.GetNextNode(ref cCurNode)) break;
					}

					cCurNode.Selected = true;
				}
			}
			MakeSelectedVisible();
		}

		
		protected override void OnResize(EventArgs e)
		{
			base.OnResize(e);

			AdjustScrollbars();
			
			Invalidate();
		}

		protected override void OnMouseDown(MouseEventArgs e)
		{
			base.OnMouseDown(e);

			#region Columns

			for (int i=0; i<columns.Count; i++)
			{
				if (columnSizeRects.Length > 0 && MouseInRect(e, columnSizeRects[i]))
				{
					// autosize column
					if (e.Clicks == 2 && e.Button == MouseButtons.Left)
					{
						int mwid = 0;
						int twid = 0;

						AutoSetColWidth(nodes, ref mwid, ref twid, i);					
								
						twid = GetStringWidth(columns[i].Text);
						if (twid > mwid)
							mwid = twid;

						mwid += 5;

						if (columns[i].Image != null)
							mwid += 18;

						columns[i].Width = mwid;
						GenerateColumnRects();
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

			#endregion

			#region Body

			if (MouseInRect(e, rowsRect))
			{
				if (e.Button == MouseButtons.Left)
				{
					TreeListNode cnode;

					#region Expand/Collapse

					// check if a nodes plus/minus has been clicked
					cnode = NodePlusClicked(e);
					if (cnode != null)
					{
						cnode.Toggle();
						AdjustScrollbars();
					} 
					else

						#endregion
					
						#region Left
					{
						// check if a noderow has been clicked
						cnode = NodeInNodeRow(e);
						if (cnode == null) return;

						switch (multiSelectMode)
						{
							case MultiSelectMode.Single:

								this.UnselectNodes(nodes);

								if (e.Clicks == 2 && !mouseActivate)
								{
									cnode.Toggle();
									AdjustScrollbars();
								}
								else if (e.Clicks == 2 && mouseActivate)
								{} // OnItemActivate(new EventArgs());					

								curNode = cnode;
								break;

							case MultiSelectMode.Range:

								this.UnselectNodes(nodes);

								curNode = cnode;
								break;

							case MultiSelectMode.Selective:

								UnfocusNodes(nodes);

								if (cnode.Selected)
								{
									// remove node from collection of selected nodes
									// selectedNodes.Remove(curNode);

									cnode.Focused = false;
									cnode.Selected = false;
									// curNode = null;
								}
								else
								{
									cnode.Focused = true;
									cnode.Selected = true;
									curNode = cnode;

									// add node to collection of selected nodes
									// selectedNodes.Add(curNode);
								}

								if (e.Clicks == 2 && !mouseActivate)
								{
									cnode.Toggle();
									AdjustScrollbars();
								}

								else if (e.Clicks == 2 && mouseActivate)
								{} // OnItemActivate(new EventArgs());

								Invalidate();

								return;
						}
					}
				}

				#endregion

				if ((this.multiSelectMode == MultiSelectMode.Single) | (firstSelectedNode == null))
				{
					if (curNode != null)
					{
						firstSelectedNode = curNode;
						firstSelectedNode.Selected = true;
						firstSelectedNode.Focused = true;
					}
				}

				ShowSelectedItems();
				OnSelectedItemChanged();
				Invalidate();
				// e.Handled = true;

			}
			
			#endregion
		}

		protected override void OnKeyUp(KeyEventArgs e)
		{
			base.OnKeyUp(e);
			if (e.Handled) return;

			if (e.KeyCode == Keys.F5)
			{
				if (allCollapsed)
					ExpandAll();
				else
					CollapseAll();
			}
		}

		protected override void DrawRows(Graphics g, Rectangle r)
		{
			// render listview item rows

			// In update so no painting allowed
			if (this.InUpdateTransaction) return;
			if (this.nodes.Count == 0) return;

			int totalRend = 0;

			int maxrend = ClientRectangle.Height/itemheight+1;
			int prior = vscrollBar.Value/itemheight;
			if (prior < 0)
				prior = 0;

			TreeListNode nodeDraw;
			if (prior > 0)
			{
				if (this.virtualParent.GetNodeAt(prior+1, 0, out nodeDraw))
				{
					int iRowNumber = nodeDraw.RowNumber(true);
					// Console.WriteLine(" RowNumber: " + iRowNumber.ToString());
					// Found node
					Console.WriteLine(nodeDraw.Text);
				}
				else
				{
					try
					{
						prior += 1;
						throw new Exception(string.Format("No node falls within the client rectangle.  Node is {0}", prior.ToString()));
					}
					catch(Exception ex)
					{
						Console.WriteLine(ex.Message);
						return;
					}
				}
			}
			else
			{
				nodeDraw = (TreeListNode)this.virtualParent.FirstChild();
			}

			totalRend = 0;
			nodeRowRects.Clear();
			pmRects.Clear();

			GenerateColumnRects();

			bool bNoMoreNodes = false;
			while (!bNoMoreNodes && (maxrend > totalRend))
			{
				RenderNodeRows(nodeDraw, g, r, ref totalRend);

				// increment number of rendered nodes
				totalRend++;
				
				bNoMoreNodes = this.GetNextNode(ref nodeDraw);
			}
		}

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

		public void ReportStatus()
		{
			Console.WriteLine("Count: " + this.virtualParent.m_iDescendentsCount + 
				" Visible: " + this.virtualParent.m_iDescendentsVisibleCount +
				" Expanded: " + this.virtualParent.m_iExpandedCount);
		}

		public override void AdjustScrollbars()
		{
			// ReportStatus();

			if (nodes.Count > 0 || columns.Count > 0 && !colScaleMode)
			{
				allColsWidth = 0;
				for (int i=0; i<columns.Count; i++)
				{
					allColsWidth += columns[i].Width;
				}

				allRowsHeight = itemheight*this.virtualParent.GetVisibleNodeCount;

				vsize = vscrollBar.Width;
				hsize = hscrollBar.Height;

				vscrollBar.Left = this.ClientRectangle.Left+this.ClientRectangle.Width-vscrollBar.Width-2;
				vscrollBar.Top = this.ClientRectangle.Top+headerBuffer+2;
				vscrollBar.Height = this.ClientRectangle.Height-hsize-headerBuffer-4;
				// vscrollBar.Maximum = allRowsHeight - ((vscrollBar.Height > allRowsHeight) ? 0 : vscrollBar.Height);
				vscrollBar.SmallChange = itemheight;
				vscrollBar.Maximum = allRowsHeight;
				vscrollBar.LargeChange = (this.ClientRectangle.Height-headerBuffer-hsize-4 > 0 ? this.ClientRectangle.Height-headerBuffer-hsize-4 : 0);

				if (allRowsHeight > this.ClientRectangle.Height-headerBuffer-4-hsize)
				{
					vscrollBar.Show();
					vsize = vscrollBar.Width;
				}
				else
				{
					vscrollBar.Hide();
					vscrollBar.Value = 0;
					vsize = 0;
				}

				hscrollBar.Left = this.ClientRectangle.Left+2;
				hscrollBar.Top = this.ClientRectangle.Top+this.ClientRectangle.Height-hscrollBar.Height-2;	
				hscrollBar.Width = this.ClientRectangle.Width-vsize-4;
				hscrollBar.Maximum = allColsWidth;			
				hscrollBar.LargeChange = (this.ClientRectangle.Width - vsize - 4 > 0 ? this.ClientRectangle.Width - vsize - 4 : 0);

				if (allColsWidth > this.ClientRectangle.Width-4-vsize)
				{
					hscrollBar.Show();
					hsize = hscrollBar.Height;
				}
				else
				{
					hscrollBar.Hide();
					hscrollBar.Value = 0;
					hsize = 0;
				}
			}

		}


		#endregion

		#region Helper Functions

		private int vsize, hsize;
		private int rendcnt = 0;
		private void AutoSetColWidth(TreeListNodeCollection nodes, ref int mwid, ref int twid, int i)
		{
			for (int j=0; j<nodes.Count; j++)
			{
				if (i > 0)
					twid = (Nodes[j].SubItems[i-1] == null) ? 0 : GetStringWidth(nodes[j].SubItems[i-1].Text);
					// twid = GetStringWidth(nodes[j].SubItems[i-1].Text);
				else
					twid = GetStringWidth(nodes[j].Text);
				twid += 5;
				if (twid > mwid)
					mwid = twid;

				if (nodes[j].Nodes.Count > 0)
				{
					AutoSetColWidth(nodes[j].Nodes, ref mwid, ref twid, i);
				}
			}
		}


		private void UnfocusNodes(TreeListNodeCollection nodecol)
		{
			for (int i=0; i<nodecol.Count; i++)
			{
				UnfocusNodes(nodecol[i].Nodes);
				nodecol[i].Focused = false;
			}
		}

		private void UnselectNodes(TreeListNodeCollection nodecol)
		{
			for (int i=0; i<nodecol.Count; i++)
			{
				UnselectNodes(nodecol[i].Nodes);
				nodecol[i].Focused = false;
				nodecol[i].Selected = false;
			}
		}

		private TreeListNode NodeInNodeRow(MouseEventArgs e)
		{
			IEnumerator ek = nodeRowRects.Keys.GetEnumerator();
			IEnumerator ev = nodeRowRects.Values.GetEnumerator();

			while (ek.MoveNext() && ev.MoveNext())
			{
				Rectangle r = (Rectangle)ek.Current;

				if (r.Left <= e.X && r.Left+r.Width >= e.X
					&& r.Top <= e.Y && r.Top+r.Height >= e.Y)
				{
					return (TreeListNode)ev.Current;
				}
			}

			return null;
		}

		private TreeListNode NodePlusClicked(MouseEventArgs e)
		{
			IEnumerator ek = pmRects.Keys.GetEnumerator();
			IEnumerator ev = pmRects.Values.GetEnumerator();

			while (ek.MoveNext() && ev.MoveNext())
			{
				Rectangle r = (Rectangle)ek.Current;

				if (r.Left <= e.X && r.Left+r.Width >= e.X
					&& r.Top <= e.Y && r.Top+r.Height >= e.Y)
				{
					return (TreeListNode)ev.Current;
				}
			}

			return null;
		}

		private void RenderNodeRows(TreeListNode node, Graphics g, Rectangle r, ref int totalRend)
		{
			// Get handy references
			TreeListNodeCollection nodesParent = ((TreeListNode)node.ParentNode()).Nodes;
			TreeListNode nodePreviousSibling = ((TreeListNode)node.PreviousSibling());

			// Set working variables
			int childCount = 0; 
			if (nodePreviousSibling != null) childCount = nodePreviousSibling.GetVisibleNodeCount;
			int count = nodesParent.Count;
			int index = nodesParent.IndexOf(node);
			int level = node.Level();

			if (node.IsVisible)
			{
				int eb = 10;	// edge buffer				

				// only render if row is visible in viewport
			if (((r.Top+itemheight*totalRend+eb/4+itemheight >= r.Top+2) 
			   && (r.Top+itemheight*totalRend+eb/4 < r.Top+r.Height)))
			{
					rendcnt++;
					int lb = 0;		// level buffer
					int ib = 0;		// icon buffer
					int hb = headerBuffer;	// header buffer	
					Pen linePen = new Pen(SystemBrushes.ControlDark, 1.0f);
					Pen PMPen = new Pen(SystemBrushes.ControlDark, 1.0f);
					Pen PMPen2 = new Pen(new SolidBrush(Color.Black), 1.0f);

					linePen.DashStyle = DashStyle.Dot;

					// add space for plis/minus icons and/or root lines to the edge buffer
					if (showrootlines || showplusminus)
					{
						eb += 10;
					}

					// set level buffer
					lb = indent*level;

					// set icon buffer
					if ((node.Selected || node.Focused) && stateImageList != null)
					{
						if (node.ImageIndex >= 0 && node.ImageIndex < stateImageList.Images.Count)
						{
							stateImageList.Draw(g, r.Left+lb+eb+2-hscrollBar.Value, r.Top+hb+itemheight*totalRend+eb/4-2, 16, 16, node.ImageIndex);
							ib = 18;
						}
					}
					else
					{
						if (smallImageList != null && node.ImageIndex >= 0 && node.ImageIndex < smallImageList.Images.Count)
						{
							smallImageList.Draw(g, r.Left+lb+eb+2-hscrollBar.Value, r.Top+hb+itemheight*totalRend+eb/4-2, 16, 16, node.ImageIndex);
							ib = 18;
						}
					}

					// add a rectangle to the node row rectangles
					Rectangle sr = new Rectangle(r.Left+lb+ib+eb+4-hscrollBar.Value, r.Top+hb+itemheight*totalRend+2, allColsWidth-(lb+ib+eb+4), itemheight);
					nodeRowRects.Add(sr, node);

					int iColWidth = (columns[0].ScaleStyle == ColumnScaleStyle.Spring ? springWid : columns[0].Width); // BMS 2003-05-24

					// render per-item background
					if (node.BackColor != this.BackColor)
					{
						if (node.UseItemStyleForSubItems)
							g.FillRectangle(new SolidBrush(node.BackColor), r.Left+lb+ib+eb+4-hscrollBar.Value, r.Top+hb+itemheight*totalRend+2, allColsWidth-(lb+ib+eb+4), itemheight);
						else
							g.FillRectangle(new SolidBrush(node.BackColor), r.Left+lb+ib+eb+4-hscrollBar.Value, r.Top+hb+itemheight*totalRend+2, iColWidth /* BMS 2003-05-24 columns[0].Width */ -(lb+ib+eb+4), itemheight);
					}

					g.Clip = new Region(sr);

					// render selection and focus
					if (node.Selected && isFocused)
					{
						g.FillRectangle(new SolidBrush(rowSelectColor), sr);
					}
					else if (node.Selected && !isFocused && !hideSelection)
					{
						g.FillRectangle(SystemBrushes.Control, sr);
					}
					else if (node.Selected && !isFocused && hideSelection)
					{
						ControlPaint.DrawFocusRectangle(g, sr);
					}

					if (node.Focused && ((isFocused && multiSelect) || !node.Selected))
					{
						ControlPaint.DrawFocusRectangle(g, sr);
					}
			
					g.Clip = new Region(new Rectangle(r.Left+2-hscrollBar.Value, r.Top+hb+2, iColWidth /* BMS 2003-05-23 columns[0].Width */, r.Height-hb-4));

					// render root lines if visible
					bool bMoreSiblingNodes = node.NextSibling() != null;
					if (r.Left+eb-hscrollBar.Value > r.Left)
					{						
						if (showrootlines && level == 0)
						{
							if (index == 0)
							{
								// Draw horizontal line with a length of eb/2
								g.DrawLine(linePen, r.Left+eb/2-hscrollBar.Value, r.Top+eb/2+hb, r.Left+eb-hscrollBar.Value, r.Top+eb/2+hb);
								if (bMoreSiblingNodes)
									// Draw vertial line with a length of eb/2
									g.DrawLine(linePen, r.Left+eb/2-hscrollBar.Value, r.Top+eb/2+hb, r.Left+eb/2-hscrollBar.Value, r.Top+eb+hb);
							}
							else if (index == count-1)
							{
								// Draw horizontal line with a length of eb/2 offset vertically by itemHeight*totalrend
								g.DrawLine(linePen, r.Left+eb/2-hscrollBar.Value, r.Top+eb/2+hb+itemheight*(totalRend), r.Left+eb-hscrollBar.Value, r.Top+eb/2+hb+itemheight*(totalRend));
								// if (bMoreSiblingNodes)
									// Draw vertial line with a length of eb/2 offset vertically by itemHeight*totalrend
									g.DrawLine(linePen, r.Left+eb/2-hscrollBar.Value, r.Top+hb+itemheight*(totalRend), r.Left+eb/2-hscrollBar.Value, r.Top+eb/2+hb+itemheight*(totalRend));
							}
							else
							{
								// Draw horizontal line with a length of eb/2 offset vertically by itemHeight*totalrend
								g.DrawLine(linePen, r.Left+eb/2-hscrollBar.Value, r.Top+eb+hb+itemheight*(totalRend)-eb/2, r.Left+eb-hscrollBar.Value, r.Top+eb+hb+itemheight*(totalRend)-eb/2);
								if (bMoreSiblingNodes)
									// Draw vertial line with a length of eb/2 offset vertically by itemHeight*totalrend
									g.DrawLine(linePen, r.Left+eb/2-hscrollBar.Value, r.Top+eb+hb+itemheight*(totalRend-1), r.Left+eb/2-hscrollBar.Value, r.Top+eb+hb+itemheight*(totalRend));
							}

							if (childCount > 0)
								g.DrawLine(linePen, r.Left+eb/2-hscrollBar.Value, r.Top+hb+itemheight*(totalRend-childCount), r.Left+eb/2-hscrollBar.Value, r.Top+hb+itemheight*(totalRend));
						}
					}

					// render child lines if visible
					if (r.Left+lb+eb-hscrollBar.Value > r.Left)
					{						
						if (showlines && level > 0)
						{
							if (index == count-1)
							{
								g.DrawLine(linePen, r.Left+lb+eb/2-hscrollBar.Value, r.Top+eb/2+hb+itemheight*(totalRend), r.Left+lb+eb-hscrollBar.Value, r.Top+eb/2+hb+itemheight*(totalRend));
								// if (bMoreSiblingNodes)
								g.DrawLine(linePen, r.Left+lb+eb/2-hscrollBar.Value, r.Top+hb+itemheight*(totalRend), r.Left+lb+eb/2-hscrollBar.Value, r.Top+eb/2+hb+itemheight*(totalRend));
							}
							else
							{
								g.DrawLine(linePen, r.Left+lb+eb/2-hscrollBar.Value, r.Top+eb/2+hb+itemheight*(totalRend), r.Left+lb+eb-hscrollBar.Value, r.Top+eb/2+hb+itemheight*(totalRend));
								// if (bMoreSiblingNodes)
								g.DrawLine(linePen, r.Left+lb+eb/2-hscrollBar.Value, r.Top+hb+itemheight*(totalRend), r.Left+lb+eb/2-hscrollBar.Value, r.Top+eb+hb+itemheight*(totalRend));
							}

							if (childCount > 0)
								g.DrawLine(linePen, r.Left+lb+eb/2-hscrollBar.Value, r.Top+hb+itemheight*(totalRend-childCount), r.Left+lb+eb/2-hscrollBar.Value, r.Top+hb+itemheight*(totalRend));
						}
					}

					// render +/- signs if visible
					if (r.Left+lb+eb/2+5-hscrollBar.Value > r.Left)
					{
						if (showplusminus && (node.Children > 0 || alwaysShowPM))
						{
							if (index == 0 && level == 0)
							{
								RenderPlus(g, r.Left+lb+eb/2-4-hscrollBar.Value, r.Top+hb+eb/2-4, 8, 8, node);
							}
							else if (index == count-1)
							{

								RenderPlus(g, r.Left+lb+eb/2-4-hscrollBar.Value, r.Top+hb+itemheight*totalRend+eb/2-4, 8, 8, node);
							}
							else
							{
								RenderPlus(g, r.Left+lb+eb/2-4-hscrollBar.Value, r.Top+hb+itemheight*totalRend+eb/2-4, 8, 8, node);
							}
						}
					}

					// render text if visible
					if (r.Left+ iColWidth /* BMS 2003-05024 columns[0].Width */ -hscrollBar.Value > r.Left)
					{
						if (node.Selected && isFocused)
							g.DrawString(TruncatedString(node.Text, columns[0].Width, lb+eb+ib+6, g), Font, SystemBrushes.HighlightText, (float)(r.Left+lb+ib+eb+4-hscrollBar.Value), (float)(r.Top+hb+itemheight*totalRend+eb/4));
						else
							g.DrawString(TruncatedString(node.Text, columns[0].Width, lb+eb+ib+6, g), Font, new SolidBrush(node.ForeColor), (float)(r.Left+lb+ib+eb+4-hscrollBar.Value), (float)(r.Top+hb+itemheight*totalRend+eb/4));
					}

					// render subitems
					int j;
					int last = 0;
					if (columns.Count > 0)
					{
						for (j=0; j<node.SubItems.Count && j<columns.Count; j++)
						{
							iColWidth = (columns[j].ScaleStyle == ColumnScaleStyle.Spring ? springWid : columns[j].Width); // BMS 2003-05-24
							int iColPlus1Width = (columns[j+1].ScaleStyle == ColumnScaleStyle.Spring ? springWid : columns[j+1].Width); // BMS 2003-05-24

							last += iColWidth /* BMS 2003-05-24 columns[j].Width */;

							g.Clip = new Region(new Rectangle(last+6-hscrollBar.Value, r.Top+headerBuffer+2, (last+iColPlus1Width /* BMS 2003-05-24 columns[j+1].Width */ > r.Width-6 ? r.Width-6 : iColPlus1Width /* BMS 2003-05-24 columns[j+1].Width */-6), r.Height-5));
							if (node.SubItems[j].ItemControl != null)
							{
								Control c = node.SubItems[j].ItemControl;
								c.Location = new Point(r.Left+last+4-hscrollBar.Value, r.Top+(itemheight*totalRend)+headerBuffer+4);
								c.ClientSize = new Size(iColPlus1Width /* BMS 2003-05-24 columns[j+1].Width */-6, itemheight-4);
								c.Parent = this;
							}						
							else
							{
								string sp = "";
								if (columns[j+1].TextAlign == HorizontalAlignment.Left)
								{
									if (node.Selected && isFocused)
										g.DrawString(TruncatedString(node.SubItems[j].Text, iColPlus1Width /* BMS 2003-05-24 columns[j+1].Width */, 9, g), this.Font, SystemBrushes.HighlightText, (float)(last+6-hscrollBar.Value), (float)(r.Top+(itemheight*totalRend)+headerBuffer+4));
									else
										g.DrawString(TruncatedString(node.SubItems[j].Text, iColPlus1Width /* BMS 2003-05-24 columns[j+1].Width */, 9, g), this.Font, (node.UseItemStyleForSubItems ? new SolidBrush(node.ForeColor) : SystemBrushes.WindowText), (float)(last+6-hscrollBar.Value), (float)(r.Top+(itemheight*totalRend)+headerBuffer+4));
								}
								else if (columns[j+1].TextAlign == HorizontalAlignment.Right)
								{
									sp = TruncatedString(node.SubItems[j].Text, iColPlus1Width /* BMS 2003-05-24 columns[j+1].Width */, 9, g);
									if (node.Selected && isFocused)
										g.DrawString(sp, this.Font, SystemBrushes.HighlightText, (float)(last+iColPlus1Width /* BMS 2003-05-24 columns[j+1].Width */-Helpers.StringTools.MeasureDisplayStringWidth(g, sp, this.Font)-4-hscrollBar.Value), (float)(r.Top+(itemheight*totalRend)+headerBuffer+4));
									else
										g.DrawString(sp, this.Font, (node.UseItemStyleForSubItems ? new SolidBrush(node.ForeColor) : SystemBrushes.WindowText), (float)(last+iColPlus1Width /* BMS 2003-05-24 columns[j+1].Width */-Helpers.StringTools.MeasureDisplayStringWidth(g, sp, this.Font)-4-hscrollBar.Value), (float)(r.Top+(itemheight*totalRend)+headerBuffer+4));
								}
								else
								{
									sp = TruncatedString(node.SubItems[j].Text, iColPlus1Width /* BMS 2003-05-24 columns[j+1].Width */, 9, g);
									if (node.Selected && isFocused)
										g.DrawString(sp, this.Font, SystemBrushes.HighlightText, (float)(last+(iColPlus1Width /* BMS 2003-05-24 columns[j+1].Width *//2)-(Helpers.StringTools.MeasureDisplayStringWidth(g, sp, this.Font)/2)-hscrollBar.Value), (float)(r.Top+(itemheight*totalRend)+headerBuffer+4));
									else
										g.DrawString(sp, this.Font, (node.UseItemStyleForSubItems ? new SolidBrush(node.ForeColor) : SystemBrushes.WindowText), (float)(last+(iColPlus1Width /* BMS 2003-05-24 columns[j+1].Width *//2)-(Helpers.StringTools.MeasureDisplayStringWidth(g, sp, this.Font)/2)-hscrollBar.Value), (float)(r.Top+(itemheight*totalRend)+headerBuffer+4));
								}
							}
						}
					}
				}
			}
		}

		private void RenderPlus(Graphics g, int x, int y, int w, int h, TreeListNode node)
		{
			if (VisualStyles)
			{
				if (node.IsExpanded)
					g.DrawImage(bmpMinus, x, y);
				else
					g.DrawImage(bmpPlus, x, y);
			}
			else
			{
				g.DrawRectangle(new Pen(SystemBrushes.ControlDark),x, y, w, h);
				g.FillRectangle(new SolidBrush(Color.White), x+1, y+1, w-1, h-1);
				g.DrawLine(new Pen(new SolidBrush(Color.Black)), x+2, y+4, x+w-2, y+4);			

				if (!node.IsExpanded)
					g.DrawLine(new Pen(new SolidBrush(Color.Black)), x+4, y+2, x+4, y+h-2);
			}

			pmRects.Add(new Rectangle(x, y, w, h), node);
		}
		/// <summary>
		/// The stacks used in this collection could be used to return relative or
		/// absolute paths from one node to another.  For the time being, this
		/// information is used ONLY to determine whether one node is "above" or
		/// below another.
		/// </summary>
		private int FirstNodeRelativeToCurrentNode()
		{
			// The return value will be TRUE if curNode falls before firstSelectedNode in
			// common parents node list.

			System.Collections.Stack cFirstSelectedNodeStack = new System.Collections.Stack();
			System.Collections.Stack cCurNodeStack = new System.Collections.Stack();

			// Find the root node for both ends of the selected range
			firstSelectedNode.GetStackToVirtualParent(cFirstSelectedNodeStack);
			curNode.GetStackToVirtualParent(cCurNodeStack);

			// Find the point of divergence
			while ((cFirstSelectedNodeStack.Count > 1) & (cCurNodeStack.Count > 1))
			{
				// Look at the first item in each stack and pop them off if they are the same
				if (cFirstSelectedNodeStack.Peek() != cCurNodeStack.Peek()) break;
				cFirstSelectedNodeStack.Pop();
				cCurNodeStack.Pop();
			}

			// At this point there are two nodes at the same level in the tree, one at the
			// top of each stack.  Use this information to determine which appears first in
			// its parents node collection.

			// Is curNode "above" or "below" firstSelectedItem (affects the node walking direction)
			int iResult = ((TreeListNode)cFirstSelectedNodeStack.Peek()).CompareTo((TreeListNode)cCurNodeStack.Peek());
			// Reverse the sign because if the first node is "after" the curr node then
			// then nodes above need to be selected (and vice-versa)
			if (iResult != 0) return -iResult;  

			// If iResult = 0, it's going to be because the curNode is the parent of the firstnode 
			// or because the firstnode is the parent of the curNode

			if (cFirstSelectedNodeStack.Count > cCurNodeStack.Count)
			{
				// cCurNodeStack is parent of cFirstSelectedNodeStack
				return -1;
			}
			else
			{
				return 1;
			}
		}


		#endregion

		#region Methods
		public void CollapseAll()
		{
			foreach (TreeListNode node in nodes)
			{
				node.CollapseAll();
			}
			allCollapsed = true;
			AdjustScrollbars();
			Invalidate();
		}

		public void ExpandAll()
		{
			foreach (TreeListNode node in nodes)
			{
				node.ExpandAll();
			}
			allCollapsed = false;
			AdjustScrollbars();
			Invalidate();
		}

		public TreeListNode GetNodeAt(int x, int y)
		{
			// To be added
			return null;
		}

		public TreeListNode GetNodeAt(Point pt)
		{
			// To be added
			return null;
		}
		
		private SelectedTreeListNodeCollection GetSelectedNodes(TreeListNode node)
		{
			SelectedTreeListNodeCollection list = new SelectedTreeListNodeCollection();

			for (int i=0; i<node.Nodes.Count; i++)
			{
				// check if current node is selected
				if (node.Nodes[i].Selected)
				{
					list.Add(node.Nodes[i]);
				}

				// chech if node is expanded and has
				// selected children
				if (node.Nodes[i].IsExpanded)
				{
					SelectedTreeListNodeCollection list2 = GetSelectedNodes(node.Nodes[i]);
					for (int j=0; j<list2.Count; j++)
					{
						list.Add(list2[j]);
					}
				}
			}

			return list;
		}

		public int GetNodeCount
		{
			get { return this.virtualParent.m_iDescendentsCount; }
		}

		public int Children
		{
			get { return this.nodes.Count; }
		}

		#endregion
	}
	#endregion

	#region Event Stuff
	#endregion

	#region TreeListNode
	[DesignTimeVisible(false), TypeConverter("Lyquidity.Controls.ExtendedListViews.TreeListNodeConverter")]
	public class TreeListNode: IParentChildList
	{
		#region Event Handlers
		
		public event MouseEventHandler MouseDown;
    public event EventHandler BeforeExpand;
    public event EventHandler AfterCollapse;
    
    #endregion

    #region Variables

    private void OnBeforeExpand()
    {
      if (BeforeExpand != null)
      {
        BeforeExpand(this, new EventArgs());
      }
    }

    private void OnAfterCollapse()
    {
      if (AfterCollapse != null)
      {
        AfterCollapse(this, new EventArgs());
      }
    }
    
		private void OnSubItemsChanged(object sender, ItemsChangedEventArgs e)
		{
			subitems[e.IndexChanged].MouseDown += new MouseEventHandler(OnSubItemMouseDown);
		}

		private void OnSubItemMouseDown(object sender, MouseEventArgs e)
		{
			if (MouseDown != null)
				MouseDown(this, e);
		}

		private void OnSubNodeMouseDown(object sender, MouseEventArgs e)
		{
			if (MouseDown != null)
				MouseDown(sender, e);
		}
		#endregion

		#region Variables

		internal int m_iDescendentsCount = 0;
		internal int m_iDescendentsVisibleCount = 0;
		internal int m_iExpandedCount = 0;  // Potentially visible ones
		internal bool m_bIsRoot = false;

		private Color backcolor = SystemColors.Window;
		private Font font;
		// private Font nodeFont;
		private Color forecolor = SystemColors.WindowText;
		private int imageindex = 0;
		private int stateimageindex = 0;
		private int index = 0;
		// private TreeListView treelistview = null;
		// private ContainerListView containerlistview = null;
		private ContainerSubListViewItemCollection subitems;
		private object tag;
		private string text;

		private TreeListNode curChild = null;
		private TreeListNodeCollection nodes;
		private string fullPath = "";

		private bool selected = false;
		private bool ischecked = false;
		private bool focused = false;
		private bool styleall = false;
		private bool hovered = false;
		private bool expanded = false;
		private bool visible = true;

		private TreeListNode parent;

		#endregion

		#region Constructor
		public TreeListNode()
		{
      this.BeforeExpand = null;
      this.AfterCollapse = null;
      
			subitems = new ContainerSubListViewItemCollection();
			subitems.ItemsChanged += new ItemsChangedEventHandler(OnSubItemsChanged);

			nodes = new TreeListNodeCollection();
			nodes.Owner = this;
			nodes.MouseDown += new MouseEventHandler(OnSubNodeMouseDown);
		}
		#endregion

		#region Properties
		[
		Category("Behavior"),
		Description("Sets the parent node of this node.")
		]
		public TreeListNode Parent
		{
			set { parent = value; }
		}

		[
		Category("Data"), 
		Description("The collection of root nodes in the treelist."),
		DesignerSerializationVisibility(DesignerSerializationVisibility.Content),
		Editor(typeof(CollectionEditor), typeof(UITypeEditor))
		]
		public TreeListNodeCollection Nodes
		{
			get { return nodes; }
		}

		[Category("Behavior"), DefaultValue(false)]
		public bool IsExpanded
		{
			get { return expanded; }
			set { expanded = value; }
		}

		[Category("Behavior"), DefaultValue(true)]
		public bool IsVisible
		{
			get { return visible; }
			set { visible = value; }
		}

		[Category("Behavior")]
		public string FullPath
		{
			get { return fullPath; }
		}

		[Category("Appearance")]
		public Color BackColor
		{
			get { return backcolor;	}
			set { backcolor = value; }
		}

		[Category("Behavior")]
		public bool Checked
		{
			get { return ischecked; }
			set { ischecked = value; }
		}

		[Browsable(false)]
		public bool Focused
		{
			get { return focused; }
			set { focused = value; }
		}

		[Category("Appearance")]
		public Font Font
		{
			get { return font; }
			set { font = value; }
		}

		[Category("Appearance")]
		public Color ForeColor
		{
			get { return forecolor; }
			set { forecolor = value; }
		}

		[Category("Behavior")]
		public int ImageIndex
		{
			get { return imageindex; }
			set { imageindex = value; }
		}

		[Browsable(false)]
		public int Index
		{
			get { return index; }
			set { index = value; }
		}
		[Browsable(false)]
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

		[Category("Behavior")]
		public int StateImageIndex
		{
			get { return stateimageindex; }
			set { stateimageindex = value; }
		}

		[Browsable(false)]
		public object Tag
		{
			get { return tag; }
			set { tag = value; }
		}

		[Category("Appearance")]
		public string Text
		{
			get { return text; }
			set { text = value; }
		}

		[Category("Behavior")]
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
		public void Collapse()
		{
			expanded = false;

			// Update counts as the number of sub nodes and their respective
			// visibilities may have changed BMS 2003-05-27

			// As a result of this collapse action, there will be m_iDescendentsVisibleCount
			// fewer visible nodes and nodes.Count fewer nodes actually expanded
			this.PropagateNodeChange(0, -this.m_iDescendentsVisibleCount, -nodes.Count);
			
			OnAfterCollapse();
		}

		public void CollapseAll()
		{
			for(int i=0; i<nodes.Count; i++)
			{
				nodes[i].CollapseAll();
			}
			Collapse();
		}
		
		public void Expand()
		{
			expanded = true;

      OnBeforeExpand();
      
			// Update counts as the number of sub nodes and their respective
			// visibilities may have changed BMS 2003-05-27

			// The visible sub-nodes will be the sub of the Visible count of the subnodes
			// while the total count will stay the same
			this.m_iDescendentsVisibleCount = 0;  // We're counting all the visible nodes so assume a zero starting point
			this.PropagateNodeChange(0, this.VisibleNodes, nodes.Count);
		}

		public void ExpandAll()
		{
			Expand();  // This MUST come before the sub-nodes are expanded

			for (int i=0; i<nodes.Count; i++) 
				((TreeListNode)nodes[i]).ExpandAll();
		}

		public int GetNodeCount
		{
			get { return this.m_iDescendentsCount; }
		}

		public int Children
		{
			get { return this.nodes.Count; }
		}

		public int GetVisibleNodeCount
		{
			get { return this.m_iDescendentsVisibleCount; }
		}

		public void Remove()
		{
			int c = nodes.IndexOf(curChild);
			nodes.Remove(curChild);
			if (nodes.Count > 0 && nodes.Count > c)
				curChild = nodes[c];
			else
				curChild = nodes[nodes.Count];

			// Update counts as the number of sub nodes and their respective
			// visibilities may have changed BMS 2003-05-27

		}

		public void Toggle()
		{
			if (expanded)
				this.Collapse();
			else
				this.Expand();
		}

		public bool IsNodeAt(int iFirstVisiblePixelY, int iOffsetY, int iRowHeight)
		{
			// iFirstVisiblePixelY is the absolute number of pixels in the Y axis to the first visible pixel in the viewport
			// iOffsetY is the number of pixels attributable to ancestral and prior sibling nodes.

			return (iFirstVisiblePixelY > iOffsetY) & 
				((iOffsetY + (this.m_iDescendentsVisibleCount * iRowHeight) /* Height consumed by sub nodes */
				+ iRowHeight ) <= iFirstVisiblePixelY);          /* Height consumed by this node */
		}

		public bool GetNodeAt(int iFirstVisiblePixelY, int iOffsetY, int iRowHeight, out TreeListNode node)
		{
			// iFirstVisiblePixelY is the absolute number of pixels in the Y axis to the first visible pixel in the viewport
			// iOffsetY is the number of pixels attributable to ancestral and prior sibling nodes.

			node = null;  // Just in case

			bool bResult = (iFirstVisiblePixelY > iOffsetY) & 
				           ((iOffsetY + (this.m_iDescendentsVisibleCount * iRowHeight)   /* Height consumed by sub nodes */
				             + iRowHeight ) <= iFirstVisiblePixelY);          /* Height consumed by this node */
			if (!bResult) return false;

			// Is the pixel in this node?
			if (iOffsetY + iRowHeight  <= iFirstVisiblePixelY)
			{
				node = this;
				return true;
			}

			// If not the find the node
			for(int iIndex=1; iIndex <= nodes.Count; iIndex++)
			{
				TreeListNode tln = nodes[iIndex-1];
				if (tln.GetNodeAt(iFirstVisiblePixelY, iOffsetY+iIndex*iRowHeight, iRowHeight, out node)) return true; 
			}

			// Should never get this far!!
			node = null;
			return false;
		}

		public bool IsNodeAt(int iRow, int iPrior)
		{
			// iRow is the row to be found
			// iPrior is the number of rows preceding this node.

			return ((iRow - iPrior) <= this.m_iDescendentsVisibleCount+1);
		}

		public bool GetNodeAt(int iRow, int iPrior, out TreeListNode node)
		{
			// iRow is the row to be found
			// iPrior is the number of rows preceding this node.

			node = null;  // Just in case
			// Can't select a root node
			if (!this.m_bIsRoot)
			{
				int iFirstRow = iRow - iPrior;

				if (!IsNodeAt(iFirstRow, 0)) return false;

				// Is the pixel in this node?
				if (iFirstRow == 1)
				{
					node = this;
					return true;
				}

				iPrior += 1;

			}

			// If not the find the node
			for(int iIndex=0; iIndex < nodes.Count; iIndex++)
			{
				TreeListNode tln = nodes[iIndex];
				if (tln.GetNodeAt(iRow, iPrior, out node)) return true; 
				iPrior += tln.GetVisibleNodeCount+1;
			}

			// Should never get this far!!
			node = null;
			return false;
		}

		public int Level()
		{
			Stack cStack = new Stack();
			GetStackToVirtualParent(cStack);
			return cStack.Count-2;  // The stack will always contain the additional virtual node
			                        // and since Level should return a zero-based reference, need to subtract 2.
		}

		public int RowNumber(bool bVisibleOnly)
		{
			Stack cStack = new Stack();
			GetStackToVirtualParent(cStack);

			int iNodes = 0;

			while(cStack.Count > 1)
			{
				TreeListNode parent = (TreeListNode)cStack.Pop();
				TreeListNode child = (TreeListNode)cStack.Peek();

				// Find child in parent and add up prior nodes
				foreach(TreeListNode node in parent.nodes)
				{
					if (node == child) 
					{
						// Need to add on the position of child in parent
						iNodes += parent.Nodes.IndexOf(child)+1;

						break;
					}
					iNodes += node.m_iDescendentsVisibleCount;
				}
			}

			cStack.Pop();

			return iNodes;
		}

		#endregion

		#region IParentChildList
		public IParentChildList ParentNode()
		{
			return parent;
		}

		public IParentChildList PreviousSibling()
		{
			if (parent != null)
			{
				int thisIndex = parent.Nodes[this];
				if (thisIndex > 0)
					return parent.Nodes[thisIndex-1];
			}

			return null;
		}

		public IParentChildList NextSibling()
		{
			if (parent != null)
			{
				int thisIndex = parent.Nodes[this];
				if (thisIndex < parent.Nodes.Count-1)
					return parent.Nodes[thisIndex+1];
			}

			return null;
		}

		public IParentChildList FirstChild()
		{
			curChild = Nodes[0];
			return curChild;
		}

		public IParentChildList LastChild()
		{
			curChild = Nodes[Nodes.Count-1];
			return curChild;
		}

		public IParentChildList NextChild()
		{
			curChild = (TreeListNode)curChild.NextSibling();
			return curChild;
		}

		public IParentChildList PreviousChild()
		{
			curChild = (TreeListNode)curChild.PreviousSibling();
			return curChild;
		}

		public int CompareTo(TreeListNode comparisonNode)
		{
			if (this == comparisonNode) return 0;
			if (this.IsAfter(comparisonNode)) return 1;
			return -1; // Before
		}

		#endregion

		#region Private
		internal void GetStackToVirtualParent(System.Collections.Stack cStack)
		{
			cStack.Push(this);
			if (this.ParentNode() != null) GetStackToVirtualParent(cStack, this.ParentNode());
			// Got to the root
		}
		private void GetStackToVirtualParent(System.Collections.Stack cStack, IParentChildList cNode)
		{
			cStack.Push(cNode);
			if (cNode.ParentNode() != null) GetStackToVirtualParent(cStack, cNode.ParentNode()); // Recursive call
			// Got to the root
		}

		private int VisibleNodes
		{
			get 
			{
				int iDescendentsVisibleCount = this.m_iDescendentsVisibleCount;

				for (int i=0; i<nodes.Count; i++) 
				{
					TreeListNode node = (TreeListNode)nodes[i];
					// Add the number of expanded node beneach this node
					if (node.IsExpanded) iDescendentsVisibleCount += node.m_iExpandedCount; 
					// And this node is also now visible
					iDescendentsVisibleCount += 1;
				}
				return iDescendentsVisibleCount;
			}
		}

		private bool IsAfter(TreeListNode node)
		{
			int thisIndex = parent.Nodes[this];
			int nodeIndex = parent.Nodes[node];

			return (thisIndex > nodeIndex);
		}

		private bool IsBefore(TreeListNode node)
		{
			int thisIndex = parent.Nodes[this];
			int nodeIndex = parent.Nodes[node];

			return (thisIndex < nodeIndex);
		}

		internal void PropagateNodeChange(int iTotalCountDelta, int iDescendentsVisibleCountDelta, int iExpandedCount)
		{
			// This function passes the change in numbers of nodes...
			this.m_iDescendentsCount += iTotalCountDelta;
			this.m_iDescendentsVisibleCount += iDescendentsVisibleCountDelta;
			this.m_iExpandedCount += iExpandedCount;

			// ...up the chain
			if (this.parent == null) return;
			this.parent.PropagateNodeChange(iTotalCountDelta, iDescendentsVisibleCountDelta, iExpandedCount);
			
		}

		#endregion

	}

	public class TreeListNodeCollection: CollectionBase
	{
		#region Events
		public event MouseEventHandler MouseDown;
		public event EventHandler NodesChanged;

		private void OnMouseDown(object sender, MouseEventArgs e)
		{
			if (MouseDown != null)
				MouseDown(sender, e);
		}

		private void OnNodesChanged()
		{
			OnNodesChanged(this, new EventArgs());
		}

		private void OnNodesChanged(object sender, EventArgs e)
		{
			if (NodesChanged != null)
				NodesChanged(sender, e);
		}
		#endregion

		#region Variables

		private TreeListNode owner;
		// internal int m_iDescendentsVisibleCount;
		// internal int m_iDescendentsCount;

		#endregion

		public TreeListNodeCollection()
		{
		}

		public TreeListNodeCollection(TreeListNode owner)
		{
			this.owner = owner;
		}

		public TreeListNode Owner
		{
			get { return owner; }
			set { owner = value; }
		}

		public int TotalCount
		{
			get 
			{
				int tcnt = 0;
				tcnt += List.Count;
				foreach (TreeListNode n in List)
				{
					tcnt += n.m_iDescendentsCount;
				}

				return tcnt;
			}
		}

		#region Implementation
		public TreeListNode this[int index]
		{
			get 
			{ 
				if (List.Count > 0)
				{
					return List[index] as TreeListNode;
				}
				else
					return null;
			}
			set 
			{

				TreeListNode tln = ((TreeListNode)List[index]);
				int iOldDescendentCount = tln.m_iDescendentsCount;
				int iOldVisibleCount = tln.m_iDescendentsVisibleCount;
				int iOldExpandedCount = tln.m_iExpandedCount;

				tln.MouseDown -= new MouseEventHandler(OnMouseDown);
				tln.Nodes.NodesChanged -= new EventHandler(OnNodesChanged);

				List[index] = value;

				tln = null;
				tln = value;
				tln.MouseDown += new MouseEventHandler(OnMouseDown);
				tln.Nodes.NodesChanged += new EventHandler(OnNodesChanged);
				tln.Parent = owner;

				// Update counts as the number of sub nodes and their respective
				// visibilities may have changed BMS 2003-05-27
				this.owner.PropagateNodeChange(tln.m_iDescendentsCount-iOldDescendentCount, 
											   tln.m_iDescendentsVisibleCount-iOldVisibleCount,
											   tln.m_iExpandedCount-iOldExpandedCount);

				OnNodesChanged();
			}
		}

		public int this[TreeListNode item]
		{
			get { return List.IndexOf(item); }
		}

		public int Add(TreeListNode item)
		{
			item.MouseDown += new MouseEventHandler(OnMouseDown);
			item.Nodes.NodesChanged += new EventHandler(OnNodesChanged);
			item.Parent = owner;

			// Special case when the parent is null because the root is visible
			if (owner.m_bIsRoot) 
			{
				owner.m_iDescendentsVisibleCount += 1;
				owner.m_iExpandedCount += 1;
			}

			// Update counts as the number of sub nodes and their respective
			// visibilities may have changed BMS 2003-05-27
			this.owner.PropagateNodeChange(item.m_iDescendentsCount+1, item.m_iDescendentsVisibleCount, item.m_iExpandedCount);

			OnNodesChanged();
			return item.Index = List.Add(item);

		}

		public void AddRange(TreeListNode[] items)
		{
			lock(List.SyncRoot)
			{
				int iDescendentsCount = 0;
				int iDescendentsVisibleCount = 0;
				int iExpandedCount = 0;

				for (int i=0; i<items.Length; i++)
				{
					TreeListNode item = items[i];

					item.MouseDown += new MouseEventHandler(OnMouseDown);
					item.Nodes.NodesChanged+= new EventHandler(OnNodesChanged);
					item.Parent = owner;
					item.Index = List.Add(item);

					// Special case when the owner is the rootbecause all root nodes are visible
					if (owner.m_bIsRoot) 
					{
						owner.m_iDescendentsVisibleCount += 1;
						owner.m_iExpandedCount += 1;
					}

					iDescendentsCount += item.m_iDescendentsCount + 1;
					iDescendentsVisibleCount += item.m_iDescendentsVisibleCount;
					iExpandedCount += item.m_iExpandedCount;
				}

				// Update counts as the number of sub nodes and their respective
				// visibilities may have changed BMS 2003-05-27
				this.owner.PropagateNodeChange(iDescendentsCount, iDescendentsVisibleCount, iExpandedCount);

				OnNodesChanged();
			}
		}

		public void Remove(TreeListNode item)
		{
			List.Remove(item);

			// Reverse the changes made when adding
			item.MouseDown -= new MouseEventHandler(OnMouseDown);
			item.Nodes.NodesChanged -= new EventHandler(OnNodesChanged);
			item.Parent = null;

			// Special case when the owner is the rootbecause all root nodes are visible
			if (owner.m_bIsRoot) 
			{
				owner.m_iDescendentsVisibleCount -= 1;
				owner.m_iExpandedCount -=1;
			}

			// Update counts as the number of sub nodes and their respective
			// visibilities may have changed BMS 2003-05-27
			this.owner.PropagateNodeChange(item.m_iDescendentsCount-1, item.m_iDescendentsVisibleCount, item.m_iExpandedCount);

			OnNodesChanged();
		}

		public new void Clear()
		{
			// BMS 2003-05-27 - Change the code to use a local variable "item"
			//                  And added call to syncroot
			int iDescendentsCount = 0;
			int iDescendentsVisibleCount = 0;
			//int iExpandedCount = 0;

			lock(List.SyncRoot)
			{
				for (int i=0; i<List.Count; i++)
				{
					TreeListNode item = (TreeListNode)List[i];
					item.Nodes.Clear();

					ContainerSubListViewItemCollection col = item.SubItems;
					for (int j=0; j<col.Count; j++)
					{

						iDescendentsCount -= (item.m_iDescendentsCount+1);
						iDescendentsVisibleCount -= item.m_iDescendentsVisibleCount;

						// Special case when the owner is the rootbecause all root nodes are visible
						if (owner.m_bIsRoot) 
						{
							owner.m_iDescendentsVisibleCount -= 1;
							owner.m_iExpandedCount -= 1;
						}

						if (col[j].ItemControl != null)
						{
							col[j].ItemControl.Parent = null;
							col[j].ItemControl.Visible = false;
							col[j].ItemControl = null;
						}

						// Reverse the changes made when adding
						item.MouseDown -= new MouseEventHandler(OnMouseDown);
						item.Nodes.NodesChanged -= new EventHandler(OnNodesChanged);
						item.Parent = null;

					}
				}
			}
			List.Clear();

			// Update counts as the number of sub nodes and their respective
			// visibilities may have changed BMS 2003-05-27

			// this.owner.PropagateNodeChange(iDescendentsCount, iDescendentsVisibleCount, iExpandedCount);

			if (owner.m_bIsRoot)
			{
				owner.m_iDescendentsCount = 0;
				owner.m_iDescendentsVisibleCount = 0;
				owner.m_iExpandedCount = 0;
			}

			OnNodesChanged();
		}

		public int IndexOf(TreeListNode item)
		{
			return List.IndexOf(item);
		}
		#endregion

	}

	public class SelectedTreeListNodeCollection: CollectionBase
	{
		#region Interface Implementations
		public TreeListNode this[int index]
		{
			get { return List[index] as TreeListNode; }
			set
			{
				List[index] = value;
			}
		}
		
		public int this[TreeListNode item]
		{
			get { return List.IndexOf(item); }
		}
		public int Add(TreeListNode item)
		{
			return item.Index = List.Add(item);
		}

		public void AddRange(TreeListNode[] items)
		{
			lock(List.SyncRoot)
			{
				for (int i=0; i<items.Length; i++)
				{
					items[i].Index = List.Add(items[i]);
				}
			}
		}

		public void Remove(TreeListNode item)
		{
			List.Remove(item);
		}

		public new void Clear()
		{
			List.Clear();
		}

		public int IndexOf(TreeListNode item)
		{
			return List.IndexOf(item);
		}
		#endregion
	}

	#endregion

	#region Type Converters
	public class TreeListNodeConverter: TypeConverter
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
			if (destinationType == typeof(InstanceDescriptor) && value is TreeListNode)
			{
				TreeListNode tln = (TreeListNode)value;

				ConstructorInfo ci = typeof(TreeListNode).GetConstructor(new Type[] {});
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
