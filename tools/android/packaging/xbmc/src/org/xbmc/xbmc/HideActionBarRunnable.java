/*
 * Helper class to ensure the ActionBar can be hidden properly on 3.0+
 */

package org.xbmc.xbmc;

import android.os.Build;
import android.view.View;

public class HideActionBarRunnable implements Runnable{

    private View rootView;
    
    public HideActionBarRunnable() {
    }

    public void setRootView(View rootView) {
        this.rootView = rootView;
    }
    
    public View getRootView() {
        return rootView;
    }
    
    public void run() {
      if (android.os.Build.VERSION.SDK_INT >= 14) {
        rootView.setSystemUiVisibility(View.SYSTEM_UI_FLAG_LOW_PROFILE);
      }
      else if (android.os.Build.VERSION.SDK_INT >= 11) {
        rootView.setSystemUiVisibility(View.STATUS_BAR_HIDDEN);
      }
    }

}

