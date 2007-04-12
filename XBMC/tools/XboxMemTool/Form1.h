#pragma once

#include "source\FormManager.h"
#include "source\Controller.h"
#include "listviewcolumnsorter.h"
#include "FormSnapshot.h"
namespace XboxMemoryUsage
{
	using namespace System;
	using namespace System::ComponentModel;
	using namespace System::Collections;
	using namespace System::Windows::Forms;
	using namespace System::Data;
	using namespace System::Drawing;
  using namespace Lyquidity::Controls::ExtendedListViews;
	/// <summary> 
	/// Summary for Form1
	///
	/// WARNING: If you change the name of this class, you will need to change the 
	///          'Resource File Name' property for the managed resource compiler tool 
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public __gc class Form1 : public System::Windows::Forms::Form
	{	
	public:
		Form1(void)
		{
			InitializeComponent();
			
      // Create an instance of a ListView column sorter and assign it 
      // to the ListView control.
      lvwColumnSorter = new ListViewColumnSorter();
      //listViewFlat->ListViewItemSorter = lvwColumnSorter;
		}
  public: System::Windows::Forms::ListBox *  listBoxDebug;
  private: System::Windows::Forms::TabControl *  tabControl;
  private: ListViewColumnSorter *lvwColumnSorter;

  private: System::Windows::Forms::ColumnHeader *  columnHeader4;
  private: System::Windows::Forms::ColumnHeader *  columnHeader3;
  private: System::Windows::Forms::ColumnHeader *  columnHeader5;

  public: Lyquidity::Controls::ExtendedListViews::TreeListView *  treeListView;
  public: System::Windows::Forms::RichTextBox *  textBoxSourceFile;
  private: System::Windows::Forms::Label *  numberLabel;
  private: System::Windows::Forms::MenuItem *  menuItem4;
  private: System::Windows::Forms::Panel *  panel1;
  private: System::Windows::Forms::Panel *  panel2;

  public: System::Windows::Forms::RichTextBox *  richTextBox1;
  private: System::Windows::Forms::Splitter *  splitter1;











  public: CController* pController;
    
	protected:
		void Dispose(Boolean disposing)
		{
			if (disposing && components)
			{
				components->Dispose();
			}
			__super::Dispose(disposing);
		}

  private: System::Windows::Forms::MenuItem *  menuItem1;
  private: System::Windows::Forms::MenuItem *  menuItem2;
  private: System::Windows::Forms::MenuItem *  menuItem3;

  public: System::Windows::Forms::TabPage *  tabFlat;
  public: System::Windows::Forms::TabPage *  tabTree;
  private: System::Windows::Forms::MainMenu *  mainMenu;

  private: System::Windows::Forms::MenuItem *  menuItem6;
  public: System::Windows::Forms::TabPage *  tabDebug;

  public: System::Windows::Forms::ListView *  listViewFlat;
  private: System::Windows::Forms::ColumnHeader *  columnHeader1;
  private: System::Windows::Forms::ColumnHeader *  columnHeader2;
  private: System::ComponentModel::IContainer *  components;


	private:
	  //private bool bRedrawTabFlat;
	  //private bool bRedrawTabTree;
	  
		/// <summary>
		/// Required designer variable.
		/// </summary>


		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
      Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader *  toggleColumnHeader1 = new Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader();
      Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader *  toggleColumnHeader2 = new Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader();
      Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader *  toggleColumnHeader3 = new Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader();
      Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader *  toggleColumnHeader4 = new Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader();
      Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader *  toggleColumnHeader5 = new Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader();
      Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader *  toggleColumnHeader6 = new Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader();
      this->tabControl = new System::Windows::Forms::TabControl();
      this->tabDebug = new System::Windows::Forms::TabPage();
      this->listBoxDebug = new System::Windows::Forms::ListBox();
      this->tabFlat = new System::Windows::Forms::TabPage();
      this->listViewFlat = new System::Windows::Forms::ListView();
      this->columnHeader1 = new System::Windows::Forms::ColumnHeader();
      this->columnHeader2 = new System::Windows::Forms::ColumnHeader();
      this->columnHeader3 = new System::Windows::Forms::ColumnHeader();
      this->columnHeader4 = new System::Windows::Forms::ColumnHeader();
      this->columnHeader5 = new System::Windows::Forms::ColumnHeader();
      this->tabTree = new System::Windows::Forms::TabPage();
      this->treeListView = new Lyquidity::Controls::ExtendedListViews::TreeListView();
      this->mainMenu = new System::Windows::Forms::MainMenu();
      this->menuItem1 = new System::Windows::Forms::MenuItem();
      this->menuItem4 = new System::Windows::Forms::MenuItem();
      this->menuItem6 = new System::Windows::Forms::MenuItem();
      this->menuItem2 = new System::Windows::Forms::MenuItem();
      this->menuItem3 = new System::Windows::Forms::MenuItem();
      this->textBoxSourceFile = new System::Windows::Forms::RichTextBox();
      this->numberLabel = new System::Windows::Forms::Label();
      this->panel1 = new System::Windows::Forms::Panel();
      this->richTextBox1 = new System::Windows::Forms::RichTextBox();
      this->panel2 = new System::Windows::Forms::Panel();
      this->splitter1 = new System::Windows::Forms::Splitter();
      this->tabControl->SuspendLayout();
      this->tabDebug->SuspendLayout();
      this->tabFlat->SuspendLayout();
      this->tabTree->SuspendLayout();
      this->panel1->SuspendLayout();
      this->panel2->SuspendLayout();
      this->SuspendLayout();
      // 
      // tabControl
      // 
      this->tabControl->Controls->Add(this->tabDebug);
      this->tabControl->Controls->Add(this->tabFlat);
      this->tabControl->Controls->Add(this->tabTree);
      this->tabControl->Dock = System::Windows::Forms::DockStyle::Bottom;
      this->tabControl->Location = System::Drawing::Point(0, 198);
      this->tabControl->Name = S"tabControl";
      this->tabControl->SelectedIndex = 0;
      this->tabControl->Size = System::Drawing::Size(1040, 392);
      this->tabControl->TabIndex = 1;
      this->tabControl->SelectedIndexChanged += new System::EventHandler(this, tabControl_SelectedIndexChanged);
      // 
      // tabDebug
      // 
      this->tabDebug->Controls->Add(this->listBoxDebug);
      this->tabDebug->Location = System::Drawing::Point(4, 22);
      this->tabDebug->Name = S"tabDebug";
      this->tabDebug->Size = System::Drawing::Size(1032, 366);
      this->tabDebug->TabIndex = 2;
      this->tabDebug->Text = S"Debug";
      // 
      // listBoxDebug
      // 
      this->listBoxDebug->Dock = System::Windows::Forms::DockStyle::Fill;
      this->listBoxDebug->Location = System::Drawing::Point(0, 0);
      this->listBoxDebug->Name = S"listBoxDebug";
      this->listBoxDebug->ScrollAlwaysVisible = true;
      this->listBoxDebug->SelectionMode = System::Windows::Forms::SelectionMode::None;
      this->listBoxDebug->Size = System::Drawing::Size(1032, 355);
      this->listBoxDebug->TabIndex = 1;
      // 
      // tabFlat
      // 
      this->tabFlat->Controls->Add(this->listViewFlat);
      this->tabFlat->Location = System::Drawing::Point(4, 22);
      this->tabFlat->Name = S"tabFlat";
      this->tabFlat->Size = System::Drawing::Size(1032, 366);
      this->tabFlat->TabIndex = 0;
      this->tabFlat->Text = S"Flat";
      // 
      // listViewFlat
      // 
      System::Windows::Forms::ColumnHeader* __mcTemp__1[] = new System::Windows::Forms::ColumnHeader*[5];
      __mcTemp__1[0] = this->columnHeader1;
      __mcTemp__1[1] = this->columnHeader2;
      __mcTemp__1[2] = this->columnHeader3;
      __mcTemp__1[3] = this->columnHeader4;
      __mcTemp__1[4] = this->columnHeader5;
      this->listViewFlat->Columns->AddRange(__mcTemp__1);
      this->listViewFlat->Dock = System::Windows::Forms::DockStyle::Fill;
      this->listViewFlat->HideSelection = false;
      this->listViewFlat->LabelWrap = false;
      this->listViewFlat->Location = System::Drawing::Point(0, 0);
      this->listViewFlat->MultiSelect = false;
      this->listViewFlat->Name = S"listViewFlat";
      this->listViewFlat->Size = System::Drawing::Size(1032, 366);
      this->listViewFlat->TabIndex = 0;
      this->listViewFlat->View = System::Windows::Forms::View::Details;
      this->listViewFlat->ColumnClick += new System::Windows::Forms::ColumnClickEventHandler(this, listViewFlat_ColumnClick);
      this->listViewFlat->SelectedIndexChanged += new System::EventHandler(this, listViewFlat_SelectedIndexChanged);
      // 
      // columnHeader1
      // 
      this->columnHeader1->Text = S"Address";
      this->columnHeader1->Width = 62;
      // 
      // columnHeader2
      // 
      this->columnHeader2->Text = S"Name";
      this->columnHeader2->Width = 418;
      // 
      // columnHeader3
      // 
      this->columnHeader3->Text = S"Offset";
      this->columnHeader3->Width = 43;
      // 
      // columnHeader4
      // 
      this->columnHeader4->Text = S"Allocated Size";
      this->columnHeader4->Width = 80;
      // 
      // columnHeader5
      // 
      this->columnHeader5->Text = S"Location";
      this->columnHeader5->Width = 187;
      // 
      // tabTree
      // 
      this->tabTree->Controls->Add(this->treeListView);
      this->tabTree->Location = System::Drawing::Point(4, 22);
      this->tabTree->Name = S"tabTree";
      this->tabTree->Size = System::Drawing::Size(1032, 366);
      this->tabTree->TabIndex = 1;
      this->tabTree->Text = S"Tree";
      // 
      // treeListView
      // 
      this->treeListView->BackColor = System::Drawing::SystemColors::Window;
      toggleColumnHeader1->Hovered = false;
      toggleColumnHeader1->Image = 0;
      toggleColumnHeader1->Index = 0;
      toggleColumnHeader1->Pressed = false;
      toggleColumnHeader1->ScaleStyle = Lyquidity::Controls::ExtendedListViews::ColumnScaleStyle::Slide;
      toggleColumnHeader1->Selected = false;
      toggleColumnHeader1->Text = S"Symbol";
      toggleColumnHeader1->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
      toggleColumnHeader1->Visible = true;
      toggleColumnHeader1->Width = 400;
      toggleColumnHeader2->Hovered = false;
      toggleColumnHeader2->Image = 0;
      toggleColumnHeader2->Index = 0;
      toggleColumnHeader2->Pressed = false;
      toggleColumnHeader2->ScaleStyle = Lyquidity::Controls::ExtendedListViews::ColumnScaleStyle::Slide;
      toggleColumnHeader2->Selected = false;
      toggleColumnHeader2->Text = S"OffSet";
      toggleColumnHeader2->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
      toggleColumnHeader2->Visible = true;
      toggleColumnHeader2->Width = 65;
      toggleColumnHeader3->Hovered = false;
      toggleColumnHeader3->Image = 0;
      toggleColumnHeader3->Index = 0;
      toggleColumnHeader3->Pressed = false;
      toggleColumnHeader3->ScaleStyle = Lyquidity::Controls::ExtendedListViews::ColumnScaleStyle::Slide;
      toggleColumnHeader3->Selected = false;
      toggleColumnHeader3->Text = S"Memory";
      toggleColumnHeader3->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
      toggleColumnHeader3->Visible = true;
      toggleColumnHeader4->Hovered = false;
      toggleColumnHeader4->Image = 0;
      toggleColumnHeader4->Index = 0;
      toggleColumnHeader4->Pressed = false;
      toggleColumnHeader4->ScaleStyle = Lyquidity::Controls::ExtendedListViews::ColumnScaleStyle::Slide;
      toggleColumnHeader4->Selected = false;
      toggleColumnHeader4->Text = S"Allocations";
      toggleColumnHeader4->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
      toggleColumnHeader4->Visible = true;
      toggleColumnHeader4->Width = 70;
      toggleColumnHeader5->Hovered = false;
      toggleColumnHeader5->Image = 0;
      toggleColumnHeader5->Index = 0;
      toggleColumnHeader5->Pressed = false;
      toggleColumnHeader5->ScaleStyle = Lyquidity::Controls::ExtendedListViews::ColumnScaleStyle::Slide;
      toggleColumnHeader5->Selected = false;
      toggleColumnHeader5->Text = S"Module";
      toggleColumnHeader5->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
      toggleColumnHeader5->Visible = true;
      toggleColumnHeader5->Width = 100;
      toggleColumnHeader6->Hovered = false;
      toggleColumnHeader6->Image = 0;
      toggleColumnHeader6->Index = 0;
      toggleColumnHeader6->Pressed = false;
      toggleColumnHeader6->ScaleStyle = Lyquidity::Controls::ExtendedListViews::ColumnScaleStyle::Slide;
      toggleColumnHeader6->Selected = false;
      toggleColumnHeader6->Text = S"Called at";
      toggleColumnHeader6->TextAlign = System::Windows::Forms::HorizontalAlignment::Left;
      toggleColumnHeader6->Visible = true;
      toggleColumnHeader6->Width = 300;
      Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader* __mcTemp__2[] = new Lyquidity::Controls::ExtendedListViews::ToggleColumnHeader*[6];
      __mcTemp__2[0] = toggleColumnHeader1;
      __mcTemp__2[1] = toggleColumnHeader2;
      __mcTemp__2[2] = toggleColumnHeader3;
      __mcTemp__2[3] = toggleColumnHeader4;
      __mcTemp__2[4] = toggleColumnHeader5;
      __mcTemp__2[5] = toggleColumnHeader6;
      this->treeListView->Columns->AddRange(__mcTemp__2);
      this->treeListView->ColumnSortColor = System::Drawing::Color::Gainsboro;
      this->treeListView->ColumnTrackColor = System::Drawing::Color::WhiteSmoke;
      this->treeListView->Dock = System::Windows::Forms::DockStyle::Fill;
      this->treeListView->GridLineColor = System::Drawing::Color::WhiteSmoke;
      this->treeListView->GridLines = true;
      this->treeListView->HeaderMenu = 0;
      this->treeListView->ItemHeight = 20;
      this->treeListView->ItemMenu = 0;
      this->treeListView->LabelEdit = false;
      this->treeListView->Location = System::Drawing::Point(0, 0);
      this->treeListView->Name = S"treeListView";
      this->treeListView->RowSelectColor = System::Drawing::SystemColors::Highlight;
      this->treeListView->RowTrackColor = System::Drawing::Color::WhiteSmoke;
      this->treeListView->ShowLines = true;
      this->treeListView->ShowRootLines = true;
      this->treeListView->Size = System::Drawing::Size(1032, 366);
      this->treeListView->SmallImageList = 0;
      this->treeListView->StateImageList = 0;
      this->treeListView->TabIndex = 0;
      this->treeListView->Text = S"treeListView";
      this->treeListView->DoubleClick += new System::EventHandler(this, treeListView_DoubleClick);
      this->treeListView->SelectedItemChanged += new System::EventHandler(this, treeListView_SelectedItemChanged);
      // 
      // mainMenu
      // 
      System::Windows::Forms::MenuItem* __mcTemp__3[] = new System::Windows::Forms::MenuItem*[2];
      __mcTemp__3[0] = this->menuItem1;
      __mcTemp__3[1] = this->menuItem3;
      this->mainMenu->MenuItems->AddRange(__mcTemp__3);
      // 
      // menuItem1
      // 
      this->menuItem1->Index = 0;
      System::Windows::Forms::MenuItem* __mcTemp__4[] = new System::Windows::Forms::MenuItem*[3];
      __mcTemp__4[0] = this->menuItem4;
      __mcTemp__4[1] = this->menuItem6;
      __mcTemp__4[2] = this->menuItem2;
      this->menuItem1->MenuItems->AddRange(__mcTemp__4);
      this->menuItem1->Text = S"File";
      // 
      // menuItem4
      // 
      this->menuItem4->Index = 0;
      this->menuItem4->Text = S"OnTest";
      this->menuItem4->Click += new System::EventHandler(this, menuItem4_Click);
      // 
      // menuItem6
      // 
      this->menuItem6->Index = 1;
      this->menuItem6->Text = S"-";
      // 
      // menuItem2
      // 
      this->menuItem2->Index = 2;
      this->menuItem2->Text = S"Exit";
      this->menuItem2->Click += new System::EventHandler(this, menuItem2_Click);
      // 
      // menuItem3
      // 
      this->menuItem3->Index = 1;
      this->menuItem3->Text = S"Settings";
      // 
      // textBoxSourceFile
      // 
      this->textBoxSourceFile->Dock = System::Windows::Forms::DockStyle::Fill;
      this->textBoxSourceFile->Location = System::Drawing::Point(50, 0);
      this->textBoxSourceFile->Name = S"textBoxSourceFile";
      this->textBoxSourceFile->ReadOnly = true;
      this->textBoxSourceFile->Size = System::Drawing::Size(990, 198);
      this->textBoxSourceFile->TabIndex = 2;
      this->textBoxSourceFile->TabStop = false;
      this->textBoxSourceFile->Text = S"";
      this->textBoxSourceFile->WordWrap = false;
      this->textBoxSourceFile->SizeChanged += new System::EventHandler(this, textBoxSourceFile_SizeChanged);
      this->textBoxSourceFile->FontChanged += new System::EventHandler(this, textBoxSourceFile_FontChanged);
      this->textBoxSourceFile->TextChanged += new System::EventHandler(this, textBoxSourceFile_TextChanged);
      this->textBoxSourceFile->MouseUp += new System::Windows::Forms::MouseEventHandler(this, textBoxSourceFile_MouseUp);
      this->textBoxSourceFile->VScroll += new System::EventHandler(this, textBoxSourceFile_VScroll);
      // 
      // numberLabel
      // 
      this->numberLabel->Dock = System::Windows::Forms::DockStyle::Left;
      this->numberLabel->Location = System::Drawing::Point(0, 0);
      this->numberLabel->Name = S"numberLabel";
      this->numberLabel->RightToLeft = System::Windows::Forms::RightToLeft::Yes;
      this->numberLabel->Size = System::Drawing::Size(50, 198);
      this->numberLabel->TabIndex = 4;
      this->numberLabel->UseMnemonic = false;
      // 
      // panel1
      // 
      this->panel1->Controls->Add(this->textBoxSourceFile);
      this->panel1->Controls->Add(this->numberLabel);
      this->panel1->Controls->Add(this->richTextBox1);
      this->panel1->Dock = System::Windows::Forms::DockStyle::Fill;
      this->panel1->Location = System::Drawing::Point(0, 0);
      this->panel1->Name = S"panel1";
      this->panel1->Size = System::Drawing::Size(1040, 198);
      this->panel1->TabIndex = 5;
      // 
      // richTextBox1
      // 
      this->richTextBox1->Dock = System::Windows::Forms::DockStyle::Fill;
      this->richTextBox1->Location = System::Drawing::Point(0, 0);
      this->richTextBox1->Name = S"richTextBox1";
      this->richTextBox1->ReadOnly = true;
      this->richTextBox1->Size = System::Drawing::Size(1040, 198);
      this->richTextBox1->TabIndex = 2;
      this->richTextBox1->TabStop = false;
      this->richTextBox1->Text = S"";
      this->richTextBox1->WordWrap = false;
      // 
      // panel2
      // 
      this->panel2->Controls->Add(this->splitter1);
      this->panel2->Controls->Add(this->panel1);
      this->panel2->Controls->Add(this->tabControl);
      this->panel2->Dock = System::Windows::Forms::DockStyle::Fill;
      this->panel2->Location = System::Drawing::Point(0, 0);
      this->panel2->Name = S"panel2";
      this->panel2->Size = System::Drawing::Size(1040, 590);
      this->panel2->TabIndex = 6;
      this->panel2->Paint += new System::Windows::Forms::PaintEventHandler(this, panel2_Paint);
      // 
      // splitter1
      // 
      this->splitter1->Dock = System::Windows::Forms::DockStyle::Bottom;
      this->splitter1->Location = System::Drawing::Point(0, 195);
      this->splitter1->Name = S"splitter1";
      this->splitter1->Size = System::Drawing::Size(1040, 3);
      this->splitter1->TabIndex = 6;
      this->splitter1->TabStop = false;
      // 
      // Form1
      // 
      this->AutoScaleBaseSize = System::Drawing::Size(5, 13);
      this->ClientSize = System::Drawing::Size(1040, 590);
      this->Controls->Add(this->panel2);
      this->Menu = this->mainMenu;
      this->Name = S"Form1";
      this->Text = S"Xbox Memory Usage Tool";
      this->Load += new System::EventHandler(this, Form1_Load);
      this->tabControl->ResumeLayout(false);
      this->tabDebug->ResumeLayout(false);
      this->tabFlat->ResumeLayout(false);
      this->tabTree->ResumeLayout(false);
      this->panel1->ResumeLayout(false);
      this->panel2->ResumeLayout(false);
      this->ResumeLayout(false);

    }	

private: System::Void menuItem5_Click(System::Object *  sender, System::EventArgs *  e)
         {
          this->pController->OnStart();
         }

private: System::Void menuItem2_Click(System::Object *  sender, System::EventArgs *  e)
         {
          this->pController->OnExit();
         }
private: System::Void tabFlat_Enter(System::Object *  sender, System::EventArgs *  e)
         {
          
         }

private: System::Void listViewFlat_ColumnClick(System::Object *  sender, System::Windows::Forms::ColumnClickEventArgs *  e)
         {
            listViewFlat->ListViewItemSorter = lvwColumnSorter;
            
            // Determine if the clicked column is already the column that is being sorted.
            if ( e->Column == lvwColumnSorter->SortColumn )
            {
	            // Reverse the current sort direction for this column.
	            if (lvwColumnSorter->Order == SortOrder::Ascending)
	            {
		            lvwColumnSorter->Order = SortOrder::Descending;
	            }
	            else
	            {
		            lvwColumnSorter->Order = SortOrder::Ascending;
	            }
            }
            else
            {
	            // Set the column number that is to be sorted. By default, this is in ascending order.
	            lvwColumnSorter->SortColumn = e->Column;
	            lvwColumnSorter->Order = SortOrder::Ascending;
            }

            lvwColumnSorter->isNumberColumn = (lvwColumnSorter->SortColumn == 4);
            
            // Perform the sort with these new sort options.
            listViewFlat->Sort();
         }

private: System::Void listViewFlat_SelectedIndexChanged(System::Object *  sender, System::EventArgs *  e)
         {
         }

private: System::Void menuSettingsPdbLocation_Click(System::Object *  sender, System::EventArgs *  e)
         {
          pController->LoadPdbFile();
         }
private: System::Void tabControl_SelectedIndexChanged(System::Object *  sender, System::EventArgs *  e)
         {
         }

private: System::Void Form1_Load(System::Object *  sender, System::EventArgs *  e)
         {

         }

private: System::Void treeListView_SelectedItemChanged(System::Object *  sender, System::EventArgs *  e)
         {
          //TreeListView* thisTreeListView = (TreeListView*)sender;
          //thisTreeListView->Columns->get_Item(0)->
          //SelectedContainerListViewItemCollection* items = thisTreeListView->get_SelectedItems();
          //items->get_Item(0)->
          //OutputDebugString("ItemChanged\n");
         }

private: System::Void treeListView_DoubleClick(System::Object *  sender, System::EventArgs *  e)
         {
          TreeListView* thisTreeListView = static_cast<TreeListView*>(sender);
          SelectedTreeListNodeCollection* items = thisTreeListView->get_SelectedNodes();
          if (items->get_Count() == 1)
          {
            TreeListNodeInfo* tag = static_cast<TreeListNodeInfo*>(items->get_Item(0)->Tag);
            pController->OnShowSourceFile(tag->sourceFile, tag->lineNumber);

          }
         }

private: System::Void updateNumberLabel()
        {
            //we get index of first visible char and number of first visible line
            Point pos(0, 0);
            int firstIndex = textBoxSourceFile->GetCharIndexFromPosition(pos);
            int firstLine = textBoxSourceFile->GetLineFromCharIndex(firstIndex);

            //now we get index of last visible char and number of last visible line
            pos.X = ClientRectangle.Width;
            pos.Y = ClientRectangle.Height;
            int lastIndex = textBoxSourceFile->GetCharIndexFromPosition(pos);
            int lastLine = textBoxSourceFile->GetLineFromCharIndex(lastIndex);

            //this is point position of last visible char, we'll use its Y value for calculating numberLabel size
            pos = textBoxSourceFile->GetPositionFromCharIndex(lastIndex);

           
            //finally, renumber label
            numberLabel->Text = "";
            for (int i = firstLine; i <= lastLine; i++)
            {
              numberLabel->Text = String::Concat(numberLabel->Text, Convert::ToString(i + 1));
              numberLabel->Text = String::Concat(numberLabel->Text, "\n");
                //numberLabel->Text += i + 1 + "\n";
            }

        }
        
private: System::Void textBoxSourceFile_TextChanged(System::Object *  sender, System::EventArgs *  e)
         {
          //updateNumberLabel();
         }

private: System::Void textBoxSourceFile_VScroll(System::Object *  sender, System::EventArgs *  e)
         {
          //int d = textBoxSourceFile->GetPositionFromCharIndex(0).Y % (textBoxSourceFile->Font->Height + 2);
          //numberLabel->Location = Point(0, d);

          //updateNumberLabel();
         }

private: System::Void textBoxSourceFile_FontChanged(System::Object *  sender, System::EventArgs *  e)
         {
          //textBoxSourceFile_VScroll(NULL, NULL);
         }

private: System::Void textBoxSourceFile_SizeChanged(System::Object *  sender, System::EventArgs *  e)
         {
          //updateNumberLabel();
          //textBoxSourceFile_VScroll(NULL, NULL);
         }

private: System::Void menuItem4_Click(System::Object *  sender, System::EventArgs *  e)
         {
          pController->OnTest();          
         }

private: System::Void textBoxSourceFile_MouseUp(System::Object *  sender, System::Windows::Forms::MouseEventArgs *  e)
         {
          //textBoxSourceFile_VScroll(NULL, NULL);
         }

private: System::Void panel2_Paint(System::Object *  sender, System::Windows::Forms::PaintEventArgs *  e)
         {
         }

};
}
