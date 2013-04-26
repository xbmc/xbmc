package org.xbmc.xbmc;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;

public class XBMCBroadcastReceiver extends BroadcastReceiver
{
  native void ReceiveIntent(Intent intent);
  static
  {
    System.loadLibrary("xbmc");
  }

  @Override
  public void onReceive(Context context, Intent intent)
  {
    String actionName = intent.getAction();
    if (actionName != null)
    {
        ReceiveIntent(intent);
    }
  }
}
