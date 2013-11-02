package org.xbmc.xbmc;

import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.util.Enumeration;
import java.util.List;
import java.util.regex.Matcher;
import java.util.regex.Pattern;
import java.util.zip.ZipEntry;
import java.util.zip.ZipFile;
import java.util.Properties;

import android.os.AsyncTask;
import android.os.Build;
import android.os.Bundle;
import android.app.Activity;
import android.app.ActivityManager;
import android.app.ActivityManager.RunningTaskInfo;
import android.app.AlertDialog;
import android.content.Context;
import android.content.DialogInterface;
import android.content.Intent;
import android.util.Log;
import android.text.Html;
import android.text.method.LinkMovementMethod;
import android.view.View;
import android.widget.ProgressBar;
import android.widget.TextView;
import android.content.res.Resources;
import android.content.res.Resources.NotFoundException;

public class Splash extends Activity {

  public enum State {
    Uninitialized, InError, Checking, Caching, StartingXBMC
  }

  private static final String TAG = "XBMC";

  private String mCpuinfo = "";
  private String mErrorMsg = "";

  private ProgressBar mProgress = null;
  private TextView mTextView = null;
  private State mState = State.Uninitialized;
  public AlertDialog myAlertDialog;

  private String sPackagePath;
  private String sApkDir;
  private File fPackagePath;
  private File fApkDir;

  public void showErrorDialog(Context context, String title, String message) {
    if (myAlertDialog != null && myAlertDialog.isShowing())
      return;

    AlertDialog.Builder builder = new AlertDialog.Builder(context);
    builder.setTitle(title);
    builder.setIcon(android.R.drawable.ic_dialog_alert);
    builder.setMessage(Html.fromHtml(message));
    builder.setPositiveButton("Exit",
        new DialogInterface.OnClickListener() {
          public void onClick(DialogInterface dialog, int arg1) {
            dialog.dismiss();
            finish();
          }
        });
    builder.setCancelable(false);
    myAlertDialog = builder.create();
    myAlertDialog.show();

    // Make links actually clickable
    ((TextView) myAlertDialog.findViewById(android.R.id.message))
        .setMovementMethod(LinkMovementMethod.getInstance());
  }

  // Do the Work
  class Work extends AsyncTask<Void, Integer, Integer> {

    private Splash mSplash = null;
    private int mProgressStatus = 0;

    public Work(Splash splash) {
      this.mSplash = splash;
    }

    void DeleteRecursive(File fileOrDirectory) {
      if (fileOrDirectory.isDirectory())
        for (File child : fileOrDirectory.listFiles())
          DeleteRecursive(child);

      fileOrDirectory.delete();
    }

    @Override
    protected Integer doInBackground(Void... param) {
      if (fApkDir.exists()) {
        // Remove existing files
        Log.d(TAG, "Removing existing " + fApkDir.toString());
        DeleteRecursive(fApkDir);
      }
      fApkDir.mkdirs();

      // Log.d(TAG, "apk: " + sPackagePath);
      // Log.d(TAG, "output: " + sApkDir);

      ZipFile zip;
      byte[] buf = new byte[4096];
      int n;
      try {
        zip = new ZipFile(sPackagePath);
        Enumeration<? extends ZipEntry> entries = zip.entries();
        mProgress.setProgress(0);
        mProgress.setMax(zip.size());

        mState = State.Caching;
        publishProgress(mProgressStatus);
        while (entries.hasMoreElements()) {
          // Update the progress bar
          publishProgress(++mProgressStatus);

          ZipEntry e = (ZipEntry) entries.nextElement();

          if (!e.getName().startsWith("assets/"))
            continue;
          if (e.getName().startsWith("assets/python2.6"))
            continue;

          String sFullPath = sApkDir + "/" + e.getName();
          File fFullPath = new File(sFullPath);
          if (e.isDirectory()) {
            // Log.d(TAG, "creating dir: " + sFullPath);
            fFullPath.mkdirs();
            continue;
          }

          fFullPath.getParentFile().mkdirs();

          try {
            InputStream in = zip.getInputStream(e);
            BufferedOutputStream out = new BufferedOutputStream(
                new FileOutputStream(sFullPath));
            while ((n = in.read(buf, 0, 4096)) > -1)
              out.write(buf, 0, n);

            in.close();
            out.close();
          } catch (IOException e1) {
            e1.printStackTrace();
          }
        }

        zip.close();

        fApkDir.setLastModified(fPackagePath.lastModified());

      } catch (FileNotFoundException e1) {
        e1.printStackTrace();
        mErrorMsg = "Cannot find package.";
        return -1;
      } catch (IOException e) {
        e.printStackTrace();
        mErrorMsg = "Cannot read package.";
        return -1;
      }

      mState = State.StartingXBMC;
      publishProgress(0);

      return 0;
    }

    @Override
    protected void onProgressUpdate(Integer... values) {
      switch (mState) {
      case Checking:
        mSplash.mTextView.setText("Checking package validity...");
        mSplash.mProgress.setVisibility(View.INVISIBLE);
        break;
      case Caching:
        mSplash.mTextView.setText("Preparing for first run. Please wait...");
        mSplash.mProgress.setVisibility(View.VISIBLE);
        mSplash.mProgress.setProgress(values[0]);
        break;
      case StartingXBMC:
        mSplash.mTextView.setText("Starting XBMC...");
        mSplash.mProgress.setVisibility(View.INVISIBLE);
        break;
      }
    }

    @Override
    protected void onPostExecute(Integer result) {
      super.onPostExecute(result);
      if (result < 0) {
        showErrorDialog(mSplash, "Error", mErrorMsg);
        return;
      }

      startXBMC();
    }
  }

  private boolean ParseCpuFeature() {
    ProcessBuilder cmd;

    try {
      String[] args = { "/system/bin/cat", "/proc/cpuinfo" };
      cmd = new ProcessBuilder(args);

      Process process = cmd.start();
      InputStream in = process.getInputStream();
      byte[] re = new byte[1024];
      while (in.read(re) != -1) {
        mCpuinfo = mCpuinfo + new String(re);
      }
      in.close();
    } catch (IOException ex) {
      ex.printStackTrace();
      return false;
    }
    return true;
  }

  private boolean CheckCpuFeature(String feat) {
    final Pattern FeaturePattern = Pattern.compile(":.*?\\s" + feat
        + "(?:\\s|$)");
    Matcher m = FeaturePattern.matcher(mCpuinfo);
    return m.find();
  }

  protected void startXBMC() {
    // NB: We only preload libxbmc to be able to get info on missing symbols.
    //     This is not normally needed
    System.loadLibrary("xbmc");
    
    // Run XBMC
    Intent intent = getIntent();
    intent.setClass(this, org.xbmc.xbmc.Main.class);
    startActivity(intent);
    finish();
  }

  @Override
  public void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);

    // Check if XBMC is not already running
    ActivityManager activityManager = (ActivityManager) getBaseContext()
        .getSystemService(Context.ACTIVITY_SERVICE);
    List<RunningTaskInfo> tasks = activityManager
        .getRunningTasks(Integer.MAX_VALUE);
    for (RunningTaskInfo task : tasks)
      if (task.topActivity.toString().equalsIgnoreCase(
          "ComponentInfo{org.xbmc.xbmc/org.xbmc.xbmc.Main}")) {
        // XBMC already running; just activate it
        startXBMC();
        return;
      }

    mState = State.Checking;

    String curArch = "";
    try {
      curArch = Build.CPU_ABI.substring(0,3);
    } catch (IndexOutOfBoundsException e) {
      mErrorMsg = "Error! Unexpected architecture: " + Build.CPU_ABI;
      Log.e(TAG, mErrorMsg);
      mState = State.InError;
   }
    
    if (mState != State.InError) {
      // Check if we are on the proper arch

      // Read the properties
      try {
        Resources resources = this.getResources();
        InputStream xbmcprop = resources.openRawResource(R.raw.xbmc);
        Properties properties = new Properties();
        properties.load(xbmcprop);

        if (!curArch.equalsIgnoreCase(properties.getProperty("native_arch"))) {
          mErrorMsg = "This XBMC package is not compatible with your device (" + curArch + " vs. " + properties.getProperty("native_arch") +").\nPlease check the <a href=\"http://wiki.xbmc.org/index.php?title=XBMC_for_Android_specific_FAQ\">XBMC Android wiki</a> for more information.";
          Log.e(TAG, mErrorMsg);
          mState = State.InError;
        }
      } catch (NotFoundException e) {
        mErrorMsg = "Cannot find properties file";
        Log.e(TAG, mErrorMsg);
        mState = State.InError;
      } catch (IOException e) {
        mErrorMsg = "Failed to open properties file";
        Log.e(TAG, mErrorMsg);
        mState = State.InError;
      }
    }
    
    if (mState != State.InError) {
      if (curArch.equalsIgnoreCase("arm")) {
        // arm arch: check if the cpu supports neon
        boolean ret = ParseCpuFeature();
        if (!ret) {
          mErrorMsg = "Error! Cannot parse CPU features.";
          Log.e(TAG, mErrorMsg);
          mState = State.InError;
        } else {
          ret = CheckCpuFeature("neon");
          if (!ret) {
            mErrorMsg = "This XBMC package is not compatible with your device (NEON).\nPlease check the <a href=\"http://wiki.xbmc.org/index.php?title=XBMC_for_Android_specific_FAQ\">XBMC Android wiki</a> for more information.";
            Log.e(TAG, mErrorMsg);
            mState = State.InError;
          }
        }
      }
    }
    
    if (mState != State.InError) {
      sPackagePath = getPackageResourcePath();
      fPackagePath = new File(sPackagePath);
      File fCacheDir = getCacheDir();
      sApkDir = fCacheDir.getAbsolutePath() + "/apk";
      fApkDir = new File(sApkDir);

      if (fApkDir.exists()
          && fApkDir.lastModified() >= fPackagePath.lastModified()) {
        mState = State.StartingXBMC;
      }
    }

    if (mState == State.StartingXBMC) {
      startXBMC();
      return;
    }
    
    setContentView(R.layout.activity_splash);

    mProgress = (ProgressBar) findViewById(R.id.progressBar1);
    mTextView = (TextView) findViewById(R.id.textView1);
    
    if (mState == State.InError) {
      showErrorDialog(this, "Error", mErrorMsg);
      return;
    }

    new Work(this).execute();
  }

}
