 #include<iostream>
using namespace std;

int findsmallest(int [],int,int);
int main(){
    int a[10]={23,11,20,3,1,14,19,9,12,10};
    int pos,temp,n=sizeof(a)/sizeof(a[0]);
    cout<<"Initial array is\n";
    for(int i=0;i<n;i++)
     cout<<a[i]<<" ";
     cout<<endl;
    for(int i=0;i<n;i++){
      pos = findsmallest(a,i,n);
       temp = a[i];
       a[i] = a[pos];
       a[pos] = temp;
    }
    cout<<"Sorted array is\n";
    for(int i=0;i<n;i++)
      cout<<a[i]<<" ";

    return 0;
}

int findsmallest(int a[],int i,int n){
    int small,j,pos = i;
    small = a[i];
    for(j=i+1 ; j < n; j++){
        if(a[j] < small){
            small = a[j];
            pos = j;
        }
    }
    return pos;
}
