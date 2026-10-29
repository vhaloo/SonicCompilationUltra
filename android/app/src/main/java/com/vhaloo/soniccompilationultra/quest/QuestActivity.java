package com.vhaloo.soniccompilationultra.quest;

import org.libsdl.app.SDLActivity;
import android.app.AlertDialog;
import android.content.Intent;
import android.database.Cursor;
import android.net.Uri;
import android.os.Bundle;
import android.provider.OpenableColumns;
import android.widget.Toast;
import java.io.*;

/** Local-only storage and Android's document picker. No camera frames or network access. */
public final class QuestActivity extends SDLActivity {
    private File root;
    private static final int IMPORT_ROM = 73;

    @Override protected void onCreate(Bundle state) {
        root = getExternalFilesDir(null);
        if (root == null) root = getFilesDir();
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
                        + "lastDirectory=ROMs\nquestResolution=1\n");
                }
            }
        } catch (IOException failure) {
            // SDL still owns Activity initialization; native startup will report inaccessible data.
            android.util.Log.e("SCU", "Could not prepare local data", failure);
        }
        super.onCreate(state);
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
                try (InputStream in = getAssets().open(asset); OutputStream out = new FileOutputStream(target)) {
                    byte[] buffer = new byte[65536]; int count;
                    while ((count = in.read(buffer)) != -1) out.write(buffer, 0, count);
                }
            }
        }
    }
    @Override protected String[] getLibraries() { return new String[]{"c++_shared", "openxr_loader", "SDL2", "main"}; }
    @Override protected String[] getArguments() { return new String[]{"--data-dir", new File(root, "UserData").getAbsolutePath()}; }

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
