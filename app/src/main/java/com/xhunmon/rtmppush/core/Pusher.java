package com.xhunmon.rtmppush.core;


import android.app.Activity;
import android.hardware.Camera;
import android.view.SurfaceView;

/**
 * description: 推送统一的对外接口类  <br>
 *
 * @author 秦城季
 * @date 2020/11/5
 */
public class Pusher {

    static {
        System.loadLibrary("native-lib");
    }

    private static Pusher pusher;
    private final VideoTask videoTask;
    private final AudioTask audioTask;

    Config config;
    boolean isPreviewing = false;
    boolean isPushing = false;

    private Pusher(Config config) {
        this.config = config;
        //初始化底层的
        native_init(config.fps, config.bitRate, config.sampleRate, config.channels);
        videoTask = new VideoTask(this);
        audioTask = new AudioTask(this);
    }

    public synchronized boolean preview() {
        if (isPreviewing) {
            return false;
        }
        isPreviewing = true;
        //预览 画面
        videoTask.preview();
        return true;
    }

    public synchronized boolean push() {
        if (isPushing) {
            return false;
        }
        if (!isPreviewing) {
            videoTask.preview();
        }
        //通知native马上要开始
        native_prepare(config.url);
        //设置推送开关
        isPreviewing = true;
        isPushing = true;
        audioTask.push();
        return true;
    }

    public synchronized boolean stop() {
        if (!isPushing && !isPreviewing) {
            return false;
        }
        native_stop();
        //设置推送开关
        isPreviewing = false;
        isPushing = false;
        videoTask.stop();
        audioTask.stop();
        return true;
    }

    public synchronized void switchCamera() {
        config.facing = config.facing == Camera.CameraInfo.CAMERA_FACING_BACK
                ? Camera.CameraInfo.CAMERA_FACING_FRONT
                : Camera.CameraInfo.CAMERA_FACING_BACK;
        videoTask.stop();
        videoTask.preview();
    }

    public native void native_init(int fps, int bitRate, int sampleRate, int channels);

    public native void native_prepare(String url);

    public native void native_stop();

    public native int native_audioGetSamples();

    public native void native_audioPush(byte[] bytes);

    public native void native_videoDataChange(int width, int height);

    public native void native_videoPush(byte[] bytes);


    public static class Config {
        Activity activity;
        //这是用户的宽高，与相机真实提供的可能并不一样
        int height = 800;
        int width = 480;
        //码率(也称比特率，指单位时间内连续播放的媒体的比特数量；文件大小=码率 x 时长)
        int bitRate = 800_000;
        //帧率
        int fps = 5;

        SurfaceView surfaceView;
        //前置或者后置摄像头
        int facing = Camera.CameraInfo.CAMERA_FACING_BACK;
        //声道数；默认双声道
        int channels = 2;
        //音频采样率
        int sampleRate = 44100;
        //推送的地址
        String url;

        public Config(Activity activity, SurfaceView surfaceView, String url) {
            this.activity = activity;
            this.surfaceView = surfaceView;
            this.url = url;
        }

        public Config height(int height) {
            if (pusher != null) {
                return this;
            }
            this.height = height;
            return this;
        }

        public Config width(int width) {
            if (pusher != null) {
                return this;
            }
            this.width = width;
            return this;
        }

        public Config bitRate(int bitRate) {
            if (pusher != null) {
                return this;
            }
            this.bitRate = bitRate;
            return this;
        }

        public Config facing(int facing) {
            if (pusher != null) {
                return this;
            }
            this.facing = facing;
            return this;
        }

        public Config fps(int fps) {
            if (pusher != null) {
                return this;
            }
            this.fps = fps;
            return this;
        }


        public Config channels(int channels) {
            if (pusher != null) {
                return this;
            }
            this.channels = channels;
            return this;
        }


        public Config sampleRate(int sampleRate) {
            if (pusher != null) {
                return this;
            }
            this.sampleRate = sampleRate;
            return this;
        }

        public Pusher build() {
            if (pusher == null) {
                pusher = new Pusher(this);
            }
            return pusher;
        }
    }
}
