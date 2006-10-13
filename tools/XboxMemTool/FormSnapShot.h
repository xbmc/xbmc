#pragma once

#include "source\Controller.h"
using namespace System;
using namespace System::ComponentModel;
using namespace System::Collections;
using namespace System::Windows::Forms;
using namespace System::Data;
using namespace System::Drawing;


namespace XboxMemoryUsage
{
	/// <summary> 
	/// Summary for FormSnapshot
	///
	/// WARNING: If you change the name of this class, you will need to change the 
	///          'Resource File Name' property for the managed resource compiler tool 
	///          associated with all .resx files this class depends on.  Otherwise,
	///          the designers will not be able to interact properly with localized
	///          resources associated with this form.
	/// </summary>
	public __gc class FormSnapShot : public System::Windows::Forms::Form
	{
	public: 
		FormSnapShot(void)
		{
			InitializeComponent();
		}
        
	protected: 
		void Dispose(Boolean disposing)
		{
			if (disposing && components)
			{
				components->Dispose();
			}
			__super::Dispose(disposing);
		}

  private: System::Windows::Forms::ColumnHeader *  columnHeader1;
  private: System::Windows::Forms::ColumnHeader *  columnHeader2;
  private: System::Windows::Forms::ColumnHeader *  columnHeader3;
  private: System::Windows::Forms::Button *  buttonNew;
  private: System::Windows::Forms::Button *  buttonRemove;


  public: CController* pController;
  private: System::Windows::Forms::Button *  buttonDiff;
  private: System::Windows::Forms::ColumnHeader *  columnHeader4;
  public: System::Windows::Forms::ListView *  listViewSnapshots;
  private: System::Windows::Forms::ColumnHeader *  columnHeader5;



	private:
		/// <summary>
		/// Required designer variable.
		/// </summary>
		System::ComponentModel::Container* components;

		/// <summary>
		/// Required method for Designer support - do not modify
		/// the contents of this method with the code editor.
		/// </summary>
		void InitializeComponent(void)
		{
      this->listViewSnapshots = new System::Windows::Forms::ListView();
      this->columnHeader4 = new System::Windows::Forms::ColumnHeader();
      this->columnHeader1 = new System::Windows::Forms::ColumnHeader();
      this->columnHeader2 = new System::Windows::Forms::ColumnHeader();
      this->columnHeader3 = new System::Windows::Forms::ColumnHeader();
      this->columnHeader5 = new System::Windows::Forms::ColumnHeader();
      this->buttonNew = new System::Windows::Forms::Button();
      this->buttonRemove = new System::Windows::Forms::Button();
      this->buttonDiff = new System::Windows::Forms::Button();
      this->SuspendLayout();
      // 
      // listViewSnapshots
      // 
      System::Windows::Forms::ColumnHeader* __mcTemp__1[] = new System::Windows::Forms::ColumnHeader*[5];
      __mcTemp__1[0] = this->columnHeader4;
      __mcTemp__1[1] = this->columnHeader1;
      __mcTemp__1[2] = this->columnHeader2;
      __mcTemp__1[3] = this->columnHeader3;
      __mcTemp__1[4] = this->columnHeader5;
      this->listViewSnapshots->Columns->AddRange(__mcTemp__1);
      this->listViewSnapshots->FullRowSelect = true;
      this->listViewSnapshots->GridLines = true;
      this->listViewSnapshots->Location = System::Drawing::Point(0, 40);
      this->listViewSnapshots->Name = S"listViewSnapshots";
      this->listViewSnapshots->Size = System::Drawing::Size(592, 160);
      this->listViewSnapshots->TabIndex = 0;
      this->listViewSnapshots->View = System::Windows::Forms::View::Details;
      this->listViewSnapshots->DoubleClick += new System::EventHandler(this, listView1_DoubleClick);
      this->listViewSnapshots->SelectedIndexChanged += new System::EventHandler(this, listViewSnapshots_SelectedIndexChanged);
      // 
      // columnHeader4
      // 
      this->columnHeader4->Text = S"Id";
      this->columnHeader4->Width = 31;
      // 
      // columnHeader1
      // 
      this->columnHeader1->Text = S"Time";
      this->columnHeader1->Width = 56;
      // 
      // columnHeader2
      // 
      this->columnHeader2->Text = S"Used (bytes)";
      this->columnHeader2->Width = 79;
      // 
      // columnHeader3
      // 
      this->columnHeader3->Text = S"Offset";
      this->columnHeader3->Width = 65;
      // 
      // columnHeader5
      // 
      this->columnHeader5->Text = S"Comment";
      this->columnHeader5->Width = 357;
      // 
      // buttonNew
      // 
      this->buttonNew->Location = System::Drawing::Point(16, 8);
      this->buttonNew->Name = S"buttonNew";
      this->buttonNew->Size = System::Drawing::Size(96, 24);
      this->buttonNew->TabIndex = 1;
      this->buttonNew->Text = S"New";
      this->buttonNew->Click += new System::EventHandler(this, buttonNew_Click);
      // 
      // buttonRemove
      // 
      this->buttonRemove->Location = System::Drawing::Point(240, 8);
      this->buttonRemove->Name = S"buttonRemove";
      this->buttonRemove->Size = System::Drawing::Size(96, 24);
      this->buttonRemove->TabIndex = 2;
      this->buttonRemove->Text = S"Remove";
      // 
      // buttonDiff
      // 
      this->buttonDiff->Location = System::Drawing::Point(128, 8);
      this->buttonDiff->Name = S"buttonDiff";
      this->buttonDiff->Size = System::Drawing::Size(96, 24);
      this->buttonDiff->TabIndex = 3;
      this->buttonDiff->Text = S"Diff";
      this->buttonDiff->Click += new System::EventHandler(this, buttonDiff_Click);
      // 
      // FormSnapShot
      // 
      this->AutoScaleBaseSize = System::Drawing::Size(5, 13);
      this->ClientSize = System::Drawing::Size(594, 204);
      this->ControlBox = false;
      this->Controls->Add(this->buttonDiff);
      this->Controls->Add(this->buttonRemove);
      this->Controls->Add(this->buttonNew);
      this->Controls->Add(this->listViewSnapshots);
      this->FormBorderStyle = System::Windows::Forms::FormBorderStyle::FixedSingle;
      this->Name = S"FormSnapShot";
      this->Text = S"Snapshot";
      this->ResumeLayout(false);

    }		
  private: System::Void buttonNew_Click(System::Object *  sender, System::EventArgs *  e)
           {
             pController->LockGuiControls();
             pController->TakeSnapshot();
             pController->UnlockGuiControls();
           }

private: System::Void listView1_DoubleClick(System::Object *sender, System::EventArgs *  e)
         {
          ListView* listView = static_cast<ListView*>(sender);
          pController->LockGuiControls();
          ListViewItem* item = listView->SelectedItems->get_Item(0);
          int id = Convert::ToInt32(item->Text);
          pController->OnSelectSnapShot(id);
          pController->UnlockGuiControls();
         }

private: System::Void buttonDiff_Click(System::Object *  sender, System::EventArgs *  e)
         {
          pController->LockGuiControls();
          
          // find the two selected snapshot
          ListView* listView = listViewSnapshots;
          int nrOfSelectedItems = listView->SelectedItems->get_Count();
          if (nrOfSelectedItems == 2)
          {
            ListViewItem* firstItem = listView->SelectedItems->get_Item(0);
            ListViewItem* secondItem = listView->SelectedItems->get_Item(1);
            
            int firstId = Convert::ToInt32(firstItem->Text);
            int secondId = Convert::ToInt32(secondItem->Text);
            
            pController->DiffSnapShots(firstId, secondId);
          }
          else
          {
            // show message user has to select two snapshots
            MessageBox(NULL, "Select two snapshots", "Error", MB_OK);
          }
          pController->UnlockGuiControls();
         }

private: System::Void listViewSnapshots_SelectedIndexChanged(System::Object *  sender, System::EventArgs *  e)
         {
         }

};
}