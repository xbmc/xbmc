package org.xbmc.xbmc;

import android.app.NativeActivity;
import android.content.Intent;
import android.view.View;
import android.os.Bundle;
import android.os.Handler;

public class Main extends NativeActivity 
{
  native void OnSystemUiNavigationShown();

  private static final String TAG = "XBMC";
  private View thisView;
  private Handler handler = new Handler();
  
  public Main() 
  {
    super();
  }

  @Override
  public void onCreate(Bundle savedInstanceState) 
  {
    super.onCreate(savedInstanceState);
    
    thisView = getWindow().getDecorView(); 
    thisView.setOnSystemUiVisibilityChangeListener(new View.OnSystemUiVisibilityChangeListener() {
      @Override
      public void onSystemUiVisibilityChange(int visibility) {
        if ((visibility & View.SYSTEM_UI_FLAG_HIDE_NAVIGATION) == 0) {
          OnSystemUiNavigationShown();
        }
      }
    });
  }

  protected void showActionBar()
  {
    handler.post(new Runnable() {
      public void run() {
        thisView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_VISIBLE);
      }
    });
  }

  protected void hideActionBar()
  {
    handler.post(new Runnable() {
      public void run() {
        thisView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_HIDE_NAVIGATION);
      }
    });
  }

}
