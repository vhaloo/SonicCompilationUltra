package com.vhaloo.soniccompilationultra.updates;

import android.app.Activity;
import android.app.PendingIntent;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.pm.PackageInfo;
import android.content.pm.PackageInstaller;
import android.content.pm.PackageManager;
import android.content.pm.Signature;
import android.net.Uri;
import android.os.Build;
import android.provider.Settings;
import java.io.*;
import java.lang.ref.WeakReference;
import java.net.URL;
import java.security.MessageDigest;
import java.util.Arrays;
import java.util.Locale;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;
import javax.net.ssl.HttpsURLConnection;
import org.json.JSONArray;
import org.json.JSONObject;
import org.libsdl.app.SDLActivity;

/** Shared by the mobile, Wear and standalone Quest launchers. */
public class UpdateActivity extends SDLActivity {
  private static final String REPO = "vhaloo/SonicCompilationUltra";
  private static final String FEED =
      "https://api.github.com/repos/" + REPO + "/releases?per_page=12";
  private static final long LIMIT = 160L * 1024 * 1024;
  private static final int SOURCES = 4161;
  private static final ExecutorService worker =
      Executors.newSingleThreadExecutor(r -> new Thread(r, "SCU release updates"));
  private static volatile String state = "idle";
  private static volatile Offer offer;
  private static volatile boolean awaitingSources;
  private static WeakReference<UpdateActivity> foreground = new WeakReference<>(null);
  private static Intent confirmation;

  private static final class Offer {
    final String version, url, sha;
    final long bytes;
    Offer(String v, String u, String h, long b) {
      version = v;
      url = u;
      sha = h;
      bytes = b;
    }
  }
  private static int[] parts(String version) {
    if (!version.matches("(0|[1-9][0-9]{0,8})\\.(0|[1-9][0-9]{0,8})\\.(0|[1-9][0-9]{0,8})"))
      return null;
    String[] s = version.split("\\.");
    return new int[] {Integer.parseInt(s[0]), Integer.parseInt(s[1]), Integer.parseInt(s[2])};
  }
  private static int compare(int[] a, int[] b) {
    for (int i = 0; i < 3; i++) {
      int v = Integer.compare(a[i], b[i]);
      if (v != 0)
        return v;
    }
    return 0;
  }
  private static HttpsURLConnection connect(String address) throws IOException {
    URL url = new URL(address);
    for (int redirects = 0; redirects <= 5; redirects++) {
      String host = url.getHost();
      if (!"https".equals(url.getProtocol()) || url.getUserInfo() != null
          || (url.getPort() != -1 && url.getPort() != 443)
          || !(host.equals("api.github.com") || host.equals("github.com")
              || host.equals("release-assets.githubusercontent.com")
              || host.equals("objects.githubusercontent.com")
              || host.equals("github-releases.githubusercontent.com")))
        throw new IOException("Unexpected release host");
      HttpsURLConnection c = (HttpsURLConnection) url.openConnection();
      c.setInstanceFollowRedirects(false);
      c.setConnectTimeout(6000);
      c.setReadTimeout(15000);
      c.setRequestProperty("User-Agent", "SonicCompilationUltra-Updater");
      c.setRequestProperty("Accept",
          host.equals("api.github.com") ? "application/vnd.github+json"
                                        : "application/octet-stream");
      int code = c.getResponseCode();
      if (code == 200)
        return c;
      String location = c.getHeaderField("Location");
      c.disconnect();
      if (code < 300 || code > 399 || location == null)
        throw new IOException("Release HTTP " + code);
      url = new URL(url, location);
    }
    throw new IOException("Too many redirects");
  }
  private static Offer latest(Context c) throws Exception {
    HttpsURLConnection conn = connect(FEED);
    ByteArrayOutputStream out = new ByteArrayOutputStream();
    try (InputStream in = conn.getInputStream()) {
      byte[] b = new byte[16384];
      int n;
      while ((n = in.read(b)) != -1) {
        if (out.size() + n > 2 * 1024 * 1024)
          throw new IOException("Oversized release list");
        out.write(b, 0, n);
      }
    } finally {
      conn.disconnect();
    }
    String packageName = c.getPackageName();
    String suffix = packageName.endsWith(".wear") ? "Android-Wear"
        : packageName.endsWith(".quest")          ? "Quest"
        : packageName.endsWith(".mobile")         ? "Android-Mobile"
                                                  : "";
    if (suffix.isEmpty())
      return null;
    PackageInfo installed = c.getPackageManager().getPackageInfo(packageName, 0);
    int[] best = parts(installed.versionName.split("-", 2)[0]);
    if (best == null)
      throw new IOException("Unknown installed version");
    JSONArray releases = new JSONArray(out.toString("UTF-8"));
    Offer result = null;
    for (int i = 0; i < releases.length(); i++) {
      JSONObject r = releases.getJSONObject(i);
      String tag = r.optString("tag_name");
      if (r.optBoolean("draft", true))
        continue;
      String v = tag.startsWith("runtime-preview-") ? tag.substring("runtime-preview-".length())
          : tag.startsWith("v")                     ? tag.substring(1)
                                                    : tag;
      int[] numeric = parts(v);
      if (numeric == null || compare(numeric, best) <= 0)
        continue;
      String name = "SonicCompilationUltra-" + v + "-" + suffix + "-ROM-free.apk";
      String url = "https://github.com/" + REPO + "/releases/download/" + tag + "/" + name;
      JSONArray assets = r.optJSONArray("assets");
      if (assets == null)
        continue;
      for (int j = 0; j < assets.length(); j++) {
        JSONObject a = assets.getJSONObject(j);
        long size = a.optLong("size");
        String hash = a.optString("digest");
        if (name.equals(a.optString("name")) && "uploaded".equals(a.optString("state"))
            && url.equals(a.optString("browser_download_url")) && size > 0 && size <= LIMIT
            && hash.matches("sha256:[0-9a-fA-F]{64}")) {
          result = new Offer(v, url, hash.substring(7), size);
          best = numeric;
          break;
        }
      }
    }
    return result;
  }
  private static boolean working() {
    return state.startsWith("downloading") || state.equals("verifying") || state.equals("system")
        || state.equals("checking");
  }
  public synchronized String scuUpdate(String action) {
    final Context app = getApplicationContext();
    if ("check".equals(action) && !working()) {
      offer = null;
      state = "checking";
      worker.execute(() -> {
        try {
          Offer o = latest(app);
          offer = o;
          state = o == null ? "current" : "available " + o.version;
        } catch (Exception e) {
          failed("check", e);
        }
      });
    } else if ("install".equals(action) && state.startsWith("available ") && offer != null) {
      state = "system";
      final Offer choice = offer;
      runOnUiThread(() -> {
        try {
          if (Build.VERSION.SDK_INT >= 26 && !getPackageManager().canRequestPackageInstalls()) {
            awaitingSources = true;
            startActivityForResult(new Intent(Settings.ACTION_MANAGE_UNKNOWN_APP_SOURCES,
                                       Uri.parse("package:" + getPackageName())),
                SOURCES);
          } else
            begin(app, choice);
        } catch (RuntimeException e) {
          awaitingSources = false;
          failed("permission", e);
        }
      });
    }
    return state;
  }
  private static void failed(String cause, Exception ex) {
    android.util.Log.w("SCU", "Update " + cause, ex);
    state = "error " + cause;
  }
  private static void begin(Context app, Offer o) {
    state = "downloading 0";
    worker.execute(() -> {
      File apk = null;
      try {
        // A fixed private cache file also replaces any partial download left by a killed process.
        apk = new File(app.getCacheDir(), "scu-verified-update.apk");
        HttpsURLConnection conn = connect(o.url);
        MessageDigest digest = MessageDigest.getInstance("SHA-256");
        long bytes = 0, deadline = android.os.SystemClock.elapsedRealtime() + 240000;
        try (InputStream in = conn.getInputStream();
            FileOutputStream out = new FileOutputStream(apk)) {
          byte[] buf = new byte[65536];
          int n, percent = -1;
          while ((n = in.read(buf)) != -1) {
            bytes += n;
            if (bytes > o.bytes || android.os.SystemClock.elapsedRealtime() > deadline)
              throw new IOException("Invalid transfer");
            out.write(buf, 0, n);
            digest.update(buf, 0, n);
            int p = (int) (bytes * 100 / o.bytes);
            if (p != percent) {
              percent = p;
              state = "downloading " + p;
            }
          }
          out.getFD().sync();
        } finally {
          conn.disconnect();
        }
        StringBuilder sha = new StringBuilder(64);
        for (byte b : digest.digest()) sha.append(String.format(Locale.ROOT, "%02x", b));
        if (bytes != o.bytes || !o.sha.equalsIgnoreCase(sha.toString()))
          throw new IOException("Package checksum mismatch");
        state = "verifying";
        verify(app, apk, o);
        commit(app, apk);
      } catch (Exception e) {
        failed("install", e);
      } finally {
        if (apk != null && apk.exists() && !apk.delete())
          android.util.Log.w("SCU", "Update cache will be replaced on retry");
      }
    });
  }

  @SuppressWarnings("deprecation")
  private static void verify(Context c, File apk, Offer o) throws Exception {
    PackageManager pm = c.getPackageManager();
    int flags = Build.VERSION.SDK_INT >= 28 ? PackageManager.GET_SIGNING_CERTIFICATES
                                            : PackageManager.GET_SIGNATURES;
    PackageInfo candidate = pm.getPackageArchiveInfo(apk.getPath(), flags);
    PackageInfo installed = pm.getPackageInfo(c.getPackageName(), flags);
    if (candidate == null || !installed.packageName.equals(candidate.packageName)
        || candidate.versionName == null
        || !candidate.versionName.split("-", 2)[0].equals(o.version))
      throw new IOException("Wrong package");
    long oldCode =
        Build.VERSION.SDK_INT >= 28 ? installed.getLongVersionCode() : installed.versionCode;
    long newCode =
        Build.VERSION.SDK_INT >= 28 ? candidate.getLongVersionCode() : candidate.versionCode;
    Signature[] oldKeys = Build.VERSION.SDK_INT >= 28
        ? installed.signingInfo.getApkContentsSigners()
        : installed.signatures;
    Signature[] newKeys = Build.VERSION.SDK_INT >= 28
        ? candidate.signingInfo.getApkContentsSigners()
        : candidate.signatures;
    if (oldKeys == null || oldKeys.length == 0 || !Arrays.equals(oldKeys, newKeys)
        || newCode <= oldCode)
      throw new IOException("Wrong version or signing identity");
  }
  private static void commit(Context c, File apk) throws Exception {
    PackageInstaller installer = c.getPackageManager().getPackageInstaller();
    PackageInstaller.SessionParams params =
        new PackageInstaller.SessionParams(PackageInstaller.SessionParams.MODE_FULL_INSTALL);
    params.setAppPackageName(c.getPackageName());
    params.setSize(apk.length());
    if (Build.VERSION.SDK_INT >= 31)
      params.setRequireUserAction(PackageInstaller.SessionParams.USER_ACTION_REQUIRED);
    int id = installer.createSession(params);
    try (PackageInstaller.Session session = installer.openSession(id)) {
      try (InputStream in = new FileInputStream(apk);
          OutputStream out = session.openWrite("base.apk", 0, apk.length())) {
        byte[] buf = new byte[65536];
        int n;
        while ((n = in.read(buf)) != -1) out.write(buf, 0, n);
        session.fsync(out);
      }
      Intent callback =
          new Intent(c, Result.class).setAction(c.getPackageName() + ".UPDATE_RESULT");
      int flags = PendingIntent.FLAG_UPDATE_CURRENT;
      if (Build.VERSION.SDK_INT >= 31)
        flags |= PendingIntent.FLAG_MUTABLE;
      PendingIntent receiver = PendingIntent.getBroadcast(c, id, callback, flags);
      state = "system";
      session.commit(receiver.getIntentSender());
    } catch (Exception e) {
      installer.abandonSession(id);
      throw e;
    }
  }
  public static final class Result extends BroadcastReceiver {
    @Override
    public void onReceive(Context c, Intent intent) {
      int code = intent.getIntExtra(PackageInstaller.EXTRA_STATUS, PackageInstaller.STATUS_FAILURE);
      if (code == PackageInstaller.STATUS_PENDING_USER_ACTION) {
        confirmation = Build.VERSION.SDK_INT >= 33
            ? intent.getParcelableExtra(Intent.EXTRA_INTENT, Intent.class)
            : intent.getParcelableExtra(Intent.EXTRA_INTENT);
        if (confirmation == null) {
          state = "error install";
          return;
        }
        showConfirmation();
      } else if (code == PackageInstaller.STATUS_SUCCESS)
        state = "current";
      else if (code == PackageInstaller.STATUS_FAILURE_ABORTED)
        state = "later";
      else
        state = "error install";
    }
  }
  private static void showConfirmation() {
    Activity a = foreground.get();
    if (a == null || confirmation == null)
      return;
    Intent launch = confirmation;
    confirmation = null;
    try {
      a.startActivity(launch);
    } catch (RuntimeException e) {
      failed("permission", e);
    }
  }
  @Override
  public void onResume() {
    super.onResume();
    foreground = new WeakReference<>(this);
    showConfirmation();
  }
  @Override
  public void onPause() {
    if (foreground.get() == this)
      foreground.clear();
    super.onPause();
  }
  @Override
  protected void onActivityResult(int req, int result, Intent data) {
    super.onActivityResult(req, result, data);
    if (req == SOURCES && awaitingSources) {
      awaitingSources = false;
      if (offer != null
          && (Build.VERSION.SDK_INT < 26 || getPackageManager().canRequestPackageInstalls()))
        begin(getApplicationContext(), offer);
      else
        state = "later";
    }
  }
}
