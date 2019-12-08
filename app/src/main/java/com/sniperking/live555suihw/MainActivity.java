package com.sniperking.live555suihw;

import androidx.appcompat.app.AppCompatActivity;

import android.os.Bundle;
import android.os.Environment;
import android.widget.TextView;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;

public class MainActivity extends AppCompatActivity {

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        final String filePath = copyTestFileToSdcard();
        new Thread(new Runnable() {
            @Override
            public void run() {
                Live555Func.start(filePath);
            }
        }).start();
        final TextView textView = findViewById(R.id.text);
        textView.postDelayed(new Runnable() {
            @Override
            public void run() {
                textView.setText(Live555Func.getUrl());
            }
        }, 2000);
    }

    private String copyTestFileToSdcard() {
        File file = new File(Environment.getExternalStorageDirectory(), "rtsp_test_butterfly.h264");
        if (file.exists()) {
            file.delete();
        }
        InputStream inputStream = null;
        FileOutputStream fileOutputStream = null;
        try {
            inputStream = getAssets().open("butterfly.h264");
            fileOutputStream = new FileOutputStream(file);
            byte[] buf = new byte[1024];
            int count;
            while ((count = inputStream.read(buf)) > 0) {
                fileOutputStream.write(buf, 0, count);
            }
            fileOutputStream.flush();
        } catch (IOException e) {
            e.printStackTrace();
        } finally {
            try {
                if (inputStream != null) {
                    inputStream.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
            try {
                if (fileOutputStream != null) {
                    fileOutputStream.close();
                }
            } catch (IOException e) {
                e.printStackTrace();
            }
        }
        return file.getAbsolutePath();
    }
}
