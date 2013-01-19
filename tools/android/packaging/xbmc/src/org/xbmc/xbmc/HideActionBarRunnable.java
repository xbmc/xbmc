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
		/* We're using Integers instead of the statics, because the symbol won't be defined at API level 11 */
		rootView.setSystemUiVisibility(0x2); //View.SYSTEM_UI_FLAG_HIDE_NAVIGATION
      }
      else if (android.os.Build.VERSION.SDK_INT >= 11) {
        rootView.setSystemUiVisibility(View.STATUS_BAR_HIDDEN);
      }
    }

}

