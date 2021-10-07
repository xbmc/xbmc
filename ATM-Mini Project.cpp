#include <conio.h>
#include <iostream>
#include <string>
using namespace std;

class atm
{
private:
	long int acc_no;
	string name;
	int pin;
	double balance;
	string mobile_no;
public:
	void setData(long int ac_no, string nameN, int pinN, double bal, string num)
	{
		acc_no = ac_no;
		name = nameN;
		pin = pinN;
		balance = bal;
		mobile_no = num;
	}
	
	long int getAccountnum()
	{
		return acc_no;
	}
	string getName()
	{
		return name;
	}
	int getPin()
	{
		return pin;
	}
	double getBal()
	{
		return balance;
	}
	string getMobile()
	{
		return mobile_no;
	}
	
	void updateMobile(string mobile_prev, string mobile_new)
	{
		if(mobile_prev == mobile_no)
		{
			mobile_no = mobile_new;
			cout<<endl<<"Sucessfully updated";
			_getch();
		}
		else
		{
			cout<<endl<<"Incorrect!!! Old Mobile No.";
			_getch();
		}
	}
	void withDraw(int amount)
	{
		if(amount>0 && amount<balance)
		{
			balance -= amount;
			cout<<endl<<"Please collect your cash";
			cout<<endl<<"Available balance : "<<balance;
			_getch();
		}
		else
		{
			cout<<endl<<"Invalid Input or Insufficient amount";
			_getch();
		}
	}
	
};

int main()
{
	int choice = 0,enterPIN;
	long int Enter_acc_no;
	system("cls");	//clears the screen automatically
	atm user1;
	user1.setData(123456789,"Chandana",1234,56982.23,"9346316864");
	do{
		system("cls");
		cout<<endl<<"Welcome to ATM"<<endl;
		cout<<endl<<"Enter your account number: ";
		cin>>Enter_acc_no;
		cout<<endl<<"Enter PIN: ";
		cin>>enterPIN;	
		if((Enter_acc_no==user1.getAccountnum()) && (enterPIN == user1.getPin()))
		{
			do
			{
				int amt = 0;
				string old,newN;
				system("cls");
				cout<<endl<<"Welcome to ATM"<<endl;
				cout<<endl<<"Select Options:";
				cout<<endl<<"1. Check Balance";
				cout<<endl<<"2. Cash Withdraw";
				cout<<endl<<"3. Show User Details";
				cout<<endl<<"4. Update Mobile Number";
				cout<<endl<<"5. Exit"<<endl;
				cin>>choice;
				switch(choice)
				{
					case 1:
						cout<<endl<<"Your Bank balance is: "<<user1.getBal();
						_getch();
						break;
					case 2:
						cout<<endl<<"Enter the amount: ";
						cin>>amt;
						user1.withDraw(amt);
						break;
					case 3:
						cout<<endl<<"User Details are:- ";
						cout<<endl<<"-> Account no: "<<user1.getAccountnum();
						cout<<endl<<"-> Name: "<<user1.getName();
						cout<<endl<<"-> Balance: "<<user1.getBal();
						cout<<endl<<"-> Mobile No. "<<user1.getMobile();
						_getch();
						break;
					case 4:
						cout<<endl<<"Enter Old Mobile No. ";
						cin>>old;
						cout<<endl<<"Enter New Mobile No. ";
						cin>>newN;
						user1.updateMobile(old,newN);
						break;
					case 5:
						exit(0);
					default:
						cout<<endl<<"Enter Valid Data !!!";
				}
			}while(1);
		}
	}while(1);
	
	
	
	return 0;
}
