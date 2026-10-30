package com.vhaloo.soniccompilationultra.mobile;

import org.libsdl.app.SDLActivity;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.widget.Toast;
import java.io.*;

/** Local-only storage and Android's document picker. No camera frames or network access. */
public final class MobileActivity extends SDLActivity {
    private volatile int[] display = {1000,0,0,0,0,0,0,60000};
    public int[] displayInfo() { return display.clone(); }
    private void updateDisplayInfo() {
        android.view.View view = mSurface;
        if (view == null) return;
        android.view.View decor = getWindow().getDecorView();
        int l=0,t=0,r=0,b=0;
        android.view.WindowInsets insets=decor.getRootWindowInsets();
        if(insets!=null) {
            if(android.os.Build.VERSION.SDK_INT>=30) {
                android.graphics.Insets edges=insets.getInsets(android.view.WindowInsets.Type.systemBars()
                    | android.view.WindowInsets.Type.displayCutout() | android.view.WindowInsets.Type.systemGestures());
                l=edges.left;t=edges.top;r=edges.right;b=edges.bottom;
            } else {
                l=insets.getSystemWindowInsetLeft();t=insets.getSystemWindowInsetTop();
                r=insets.getSystemWindowInsetRight();b=insets.getSystemWindowInsetBottom();
                if(android.os.Build.VERSION.SDK_INT>=28 && insets.getDisplayCutout()!=null) {
                    android.view.DisplayCutout c=insets.getDisplayCutout();
                    l=Math.max(l,c.getSafeInsetLeft());t=Math.max(t,c.getSafeInsetTop());
                    r=Math.max(r,c.getSafeInsetRight());b=Math.max(b,c.getSafeInsetBottom());
                }
            }
        }
        int[] pos=new int[2]; view.getLocationInWindow(pos);
        display=new int[]{Math.round(getResources().getDisplayMetrics().density*1000),
            Math.max(0,l-pos[0]),Math.max(0,t-pos[1]),
            Math.max(0,r-(decor.getWidth()-pos[0]-view.getWidth())),
            Math.max(0,b-(decor.getHeight()-pos[1]-view.getHeight())),view.getWidth(),view.getHeight(),
            Math.round(view.getDisplay()!=null ? view.getDisplay().getRefreshRate()*1000 : 60000)};
    }
    public void configureDisplay(int orientation, int refresh, boolean immersive) {
        runOnUiThread(() -> {
            int requested=orientation==1?android.content.pm.ActivityInfo.SCREEN_ORIENTATION_SENSOR_LANDSCAPE:
                orientation==2?android.content.pm.ActivityInfo.SCREEN_ORIENTATION_SENSOR_PORTRAIT:
                android.content.pm.ActivityInfo.SCREEN_ORIENTATION_FULL_USER;
            setRequestedOrientation(requested);
            android.view.View decor=getWindow().getDecorView();
            if(android.os.Build.VERSION.SDK_INT>=30 && getWindow().getInsetsController()!=null) {
                android.view.WindowInsetsController c=getWindow().getInsetsController();
                if(immersive) c.hide(android.view.WindowInsets.Type.systemBars());
                else c.show(android.view.WindowInsets.Type.systemBars());
                c.setSystemBarsBehavior(android.view.WindowInsetsController.BEHAVIOR_SHOW_TRANSIENT_BARS_BY_SWIPE);
            } else {
                decor.setSystemUiVisibility(immersive ? android.view.View.SYSTEM_UI_FLAG_FULLSCREEN
                    |android.view.View.SYSTEM_UI_FLAG_HIDE_NAVIGATION|android.view.View.SYSTEM_UI_FLAG_IMMERSIVE_STICKY : 0);
            }
            int[] rates={60,30,90,120,0};int rate=rates[Math.max(0,Math.min(4,refresh))];
            android.view.WindowManager.LayoutParams params=getWindow().getAttributes();
            if(rate==0 && decor.getDisplay()!=null) {
                android.view.Display.Mode current=decor.getDisplay().getMode();
                android.view.Display.Mode best=current;
                for(android.view.Display.Mode mode:decor.getDisplay().getSupportedModes())
                    if(mode.getPhysicalWidth()==current.getPhysicalWidth()
                        && mode.getPhysicalHeight()==current.getPhysicalHeight()
                        && mode.getRefreshRate()>best.getRefreshRate()) best=mode;
                params.preferredDisplayModeId=best.getModeId();
                rate=Math.round(best.getRefreshRate());
            } else params.preferredDisplayModeId=0;
            params.preferredRefreshRate=rate;
            getWindow().setAttributes(params);
            if(android.os.Build.VERSION.SDK_INT>=30) {
                android.view.View view=mSurface;
                if(view instanceof android.view.SurfaceView) {
                    android.view.Surface surface=((android.view.SurfaceView)view).getHolder().getSurface();
                    if(surface.isValid()) surface.setFrameRate(rate,android.view.Surface.FRAME_RATE_COMPATIBILITY_DEFAULT);
                }
            }
            updateDisplayInfo();
        });
    }
    private File root;
    private String preparationError;
    private static final int IMPORT_ROM = 73;

    @Override protected void onCreate(Bundle state) {
        // Runtime files belong to the app UID. Android 14 scoped-storage rules can
        // reject native stat/open on an external directory populated by ADB.
        root = getFilesDir();
        try {
            copyTree("scu", root);
            File data = new File(root, "UserData");
            ensureDirectory(data);
            ensureDirectory(new File(root, "ROMs"));
            File settings = new File(data, "settings.ini");
            if (!settings.exists()) {
                try (Writer out = new OutputStreamWriter(new FileOutputStream(settings), "UTF-8")) {
                    out.write("language=-1\nview=2\nquality=0\nbackground=0\nblur=3\n"
                        + "renderer=0\nfullscreen=0\nvsync=0\nrewind=1\nrewindMemory=0\n"
                        + "lastDirectory=ROMs\nmobileLayout=0\nmobileRefresh=0\nmobileRenderQuality=1\n");
                }
            }
        } catch (IOException failure) {
            // SDL still owns Activity initialization; native startup will report inaccessible data.
            android.util.Log.e("SCU", "Could not prepare local data", failure);
            preparationError = "Could not prepare private app storage: " + failure.getMessage();
        }
        super.onCreate(state);
        getWindow().addFlags(android.view.WindowManager.LayoutParams.FLAG_KEEP_SCREEN_ON);
        getWindow().getDecorView().setOnApplyWindowInsetsListener((v, insets) -> {
            updateDisplayInfo(); return v.onApplyWindowInsets(insets);
        });
        getWindow().getDecorView().getViewTreeObserver().addOnGlobalLayoutListener(this::updateDisplayInfo);
        updateDisplayInfo();
        if(android.os.Build.VERSION.SDK_INT>=33)
            getOnBackInvokedDispatcher().registerOnBackInvokedCallback(
                android.window.OnBackInvokedDispatcher.PRIORITY_DEFAULT,this::sendBack);
    }
    private void sendBack() {
        SDLActivity.onNativeKeyDown(android.view.KeyEvent.KEYCODE_BACK);
        SDLActivity.onNativeKeyUp(android.view.KeyEvent.KEYCODE_BACK);
    }
    @Override @android.annotation.SuppressLint("GestureBackNavigation")
    public void onBackPressed() { // API 24-32 fallback; API 33+ uses the callback above.
        sendBack();
    }
    private static void ensureDirectory(File path) throws IOException {
        if (!path.isDirectory() && !path.mkdirs()) throw new IOException("Cannot create " + path.getName());
    }
    private void copyTree(String source, File destination) throws IOException {
        String[] children = getAssets().list(source);
        if (children == null) return;
        ensureDirectory(destination);
        for (String child : children) {
            String asset = source + "/" + child;
            File target = new File(destination, child);
            if (!target.getCanonicalPath().startsWith(root.getCanonicalPath() + File.separator))
                throw new IOException("Invalid bundled path");
            String[] nested = getAssets().list(asset);
            if (nested != null && nested.length > 0) copyTree(asset, target);
            else if (!target.exists() || asset.startsWith("scu/assets/") || asset.startsWith("scu/licences/")) {
                File staging = new File(target.getPath() + ".seed");
                try {
                    try (InputStream in = getAssets().open(asset); OutputStream out = new FileOutputStream(staging)) {
                        byte[] buffer = new byte[65536]; int count;
                        while ((count = in.read(buffer)) != -1) out.write(buffer, 0, count);
                    }
                    // Same-filesystem rename publishes a complete file. An interrupted
                    // seed never leaves a partial ROM that would be skipped on retry.
                    if (!staging.renameTo(target)) throw new IOException("Cannot finish bundled file copy.");
                } finally {
                    if (staging.exists()) staging.delete();
                }
            }
        }
    }
    @Override protected String[] getLibraries() { return new String[]{"c++_shared", "SDL2", "main"}; }
    @Override protected String[] getArguments() {
        java.util.ArrayList<String> args = new java.util.ArrayList<>();
        args.add("--data-dir"); args.add(new File(root, "UserData").getAbsolutePath());
        if (getIntent().getBooleanExtra("scu_menu_only", false)) {
            args.add("--menu-only");
            String language = getIntent().getStringExtra("scu_language");
            if ("en".equals(language) || "fr".equals(language) || "es".equals(language) || "ja".equals(language)) {
                args.add("--language"); args.add(language);
            }

        }
        if (preparationError != null) { args.add("--startup-error"); args.add(preparationError); }
        return args.toArray(new String[0]);
    }

    public void chooseRom() {
        runOnUiThread(() -> {
            Intent intent = new Intent(Intent.ACTION_OPEN_DOCUMENT);
            intent.addCategory(Intent.CATEGORY_OPENABLE); intent.setType("*/*");
            startActivityForResult(intent, IMPORT_ROM);
        });
    }
    @Override protected void onActivityResult(int request, int result, Intent data) {
        super.onActivityResult(request, result, data);
        if (request != IMPORT_ROM || result != RESULT_OK || data == null || data.getData() == null) return;
        Uri uri = data.getData();
        new Thread(() -> {
            File temporary = null;
            try {
                String name = "cartridge.bin";
                try (Cursor cursor = getContentResolver().query(uri, new String[]{OpenableColumns.DISPLAY_NAME}, null, null, null)) {
                    if (cursor != null && cursor.moveToFirst()) name = cursor.getString(0);
                }
                name = name.replaceAll("[\\\\/\\t\\r\\n]", "_");
                String lower = name.toLowerCase(java.util.Locale.ROOT);
                if (!(lower.endsWith(".zip") || lower.endsWith(".bin") || lower.endsWith(".md")
                        || lower.endsWith(".gen") || lower.endsWith(".smd")))
                    throw new IOException("Choose a cartridge file or ZIP.");
                File folder = new File(root, "ROMs"); ensureDirectory(folder);
                File target = new File(folder, name);
                if (target.exists()) target = new File(folder, System.currentTimeMillis() + "-" + name);
                temporary = new File(target.getPath() + ".importing");
                try (InputStream in = getContentResolver().openInputStream(uri); OutputStream out = new FileOutputStream(temporary)) {
                    if (in == null) throw new IOException("Cannot read this document.");
                    byte[] buffer = new byte[65536]; int count; long total = 0;
                    while ((count = in.read(buffer)) != -1) {
                        total += count;
                        if (total > 32L * 1024 * 1024) throw new IOException("Cartridge archive exceeds 32 MB.");
                        out.write(buffer, 0, count);
                    }
                }
                if (!temporary.renameTo(target)) throw new IOException("Cannot finish the cartridge copy.");
                temporary = null;
                SDLActivity.onNativeDropFile(target.getAbsolutePath());
            } catch (Exception failure) {
                if (temporary != null) temporary.delete();
                final String message = failure.getMessage();
                runOnUiThread(() -> Toast.makeText(this, message, Toast.LENGTH_LONG).show());
            }
        }, "SCU-cartridge-import").start();
    }
}
