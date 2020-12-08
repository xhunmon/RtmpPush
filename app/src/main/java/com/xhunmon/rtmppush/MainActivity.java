package com.xhunmon.rtmppush;

import android.Manifest;
import android.content.pm.PackageManager;
import android.os.Build;
import android.os.Bundle;
import android.view.View;
import android.widget.Toast;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;

import com.xhunmon.rtmppush.core.Pusher;

public class MainActivity extends AppCompatActivity {

    private static final int PERMISSION_REQUEST_CAMERA = 0x01;
    private Pusher pusher;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        if (requestPermission()) {
            init();
        }
    }

    private void init() {
        //在阿里云买了一年的最便宜的服务器，搭建了rtmp推送服务器。把yourself改一下，预防重名。有条件最好自己买一个低配服务器，要不然人多画面都出不来
        pusher = new Pusher.Config(this, findViewById(R.id.surfaceView), "rtmp://8.129.163.125:1935/myapp/yourself").build();
        //在电脑使用工具，或者ffmpeg进行播放。
        // ffplay -loglevel verbose "rtmp://8.129.163.125:1935/myapp/yourself"

        //在  http://8.129.163.125:8081/stat 查看当前服务器rtmp情况
    }

    private boolean requestPermission() {
        if (Build.VERSION.SDK_INT < Build.VERSION_CODES.M) {
            return true;
        }
        if (checkSelfPermission(Manifest.permission.CAMERA) == PackageManager.PERMISSION_GRANTED
                && checkSelfPermission(Manifest.permission.WRITE_EXTERNAL_STORAGE) == PackageManager.PERMISSION_GRANTED) {
            return true;
        }
        requestPermissions(new String[]{Manifest.permission.RECORD_AUDIO,
                Manifest.permission.CAMERA,
                Manifest.permission.WRITE_EXTERNAL_STORAGE,
                Manifest.permission.READ_EXTERNAL_STORAGE}, PERMISSION_REQUEST_CAMERA);
        return false;
    }


    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        if (requestCode == PERMISSION_REQUEST_CAMERA) {
            if (grantResults.length > 0 && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                init();
            } else {
                Toast.makeText(MainActivity.this, "申请相机失败！", Toast.LENGTH_SHORT).show();
            }
        }
    }

    public void preview(View view) {
        if (!pusher.preview()) {
            Toast.makeText(this, "在预览中……", Toast.LENGTH_SHORT).show();
        }
    }

    public void push(View view) {
        if (!pusher.push()) {
            Toast.makeText(this, "在推送中……", Toast.LENGTH_SHORT).show();
        }
    }

    public void stop(View view) {
        if (pusher.stop()) {
            Toast.makeText(this, "画面和推送已暂停成功！", Toast.LENGTH_SHORT).show();
        }
    }

    public void switchCamera(View view) {
        pusher.switchCamera();
    }

    public void write(View view) {
//        pusher.write();
    }
}