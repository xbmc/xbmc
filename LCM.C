       int max,i=2,lcm,n;
      if(b>a)
	{ max=b;      n=a;}
      else
	{max=a;   n=b;}
      for(i=2;i<max;i++)
      { if(max%a==0&&max%b==0)
	{ lcm=max;
	 break;
	 }
	 else
	 lcm=max*n;

      }