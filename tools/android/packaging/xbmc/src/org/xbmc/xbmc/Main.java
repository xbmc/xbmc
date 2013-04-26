package org.xbmc.xbmc;

import android.app.NativeActivity;
import android.content.Intent;
import android.os.Bundle;

public class Main extends NativeActivity 
{
  native void ReceiveViewIntent(Intent intent);

  public Main() 
  {
    super();
  }

  @Override
  public void onCreate(Bundle savedInstanceState) 
  {
    super.onCreate(savedInstanceState);
  }

  @Override
  protected void onNewIntent(Intent intent) 
  {
    super.onNewIntent(intent);

    if (intent.getAction() == Intent.ACTION_VIEW)
      ReceiveViewIntent(intent);
  }
}
