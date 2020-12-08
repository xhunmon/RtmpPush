package com.xhunmon.rtmppush.core;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.util.Log;

import java.util.concurrent.ExecutorService;
import java.util.concurrent.Executors;

/**
 * description: 处理音频任务  <br>
 *
 * @author 秦城季
 * @date 2020/12/6
 */
public class AudioTask {
    private static final String TAG = "AudioTask";
    private Pusher pusher;
    private final ExecutorService executor;
    private AudioRecord audioRecord;
    private int minBufferSize = 0;

    public AudioTask(Pusher pusher) {
        this.pusher = pusher;

        executor = Executors.newSingleThreadExecutor();
        //准备录音机 采集pcm 数据
        int channelConfig;
        if (pusher.config.channels == 2) {
            channelConfig = AudioFormat.CHANNEL_IN_STEREO;
        } else {
            channelConfig = AudioFormat.CHANNEL_IN_MONO;
        }

        //通过faac框架获取缓冲区大小（16 位 2个字节）
        int inputSamples = pusher.native_audioGetSamples() * 2;

        //android系统提供的：最小需要的缓冲区；
        minBufferSize = AudioRecord.getMinBufferSize(pusher.config.sampleRate, AudioFormat.CHANNEL_CONFIGURATION_DEFAULT, AudioFormat.ENCODING_PCM_16BIT) * 2;
        //说明设备不支持用户所取的采样率
        if (minBufferSize <= 0) {
            //通过api查询设备是否支持该采样率
            for (int rate : new int[]{44100, 22050, 11025, 16000, 8000}) {
                //最小需要的缓冲区
                minBufferSize = AudioRecord.getMinBufferSize(rate, AudioFormat.CHANNEL_CONFIGURATION_DEFAULT, AudioFormat.ENCODING_PCM_16BIT) * 2;
                if (minBufferSize > 0) {
                    pusher.config.sampleRate = rate;
                    break;
                }
            }
        }

        Log.d(TAG, "minBufferSize：" + minBufferSize+" | inputSamples："+inputSamples);
        if (inputSamples > 0 && inputSamples < minBufferSize) {
            minBufferSize = inputSamples;
        }

        //设备采样率有限
        if (minBufferSize <= 0) {
            Log.e(TAG, "设备采样率有限~~~~~~~~~~");
            return;
        }


        //1、麦克风 2、采样率 3、声道数 4、采样位
        audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, pusher.config.sampleRate, channelConfig, AudioFormat.ENCODING_PCM_16BIT, minBufferSize);
    }

    public void push() {
        //录音
        executor.execute(new RecordingTask());
    }

    public void stop() {
        if (audioRecord != null) {
            //停止录音机
            audioRecord.stop();
//            audioRecord.release();
        }
    }

    class RecordingTask implements Runnable {

        @Override
        public void run() {
            //启动录音机
            audioRecord.startRecording();
            byte[] bytes = new byte[minBufferSize];
            while (pusher.isPushing) {
                int len = audioRecord.read(bytes, 0, bytes.length);
                if (len > 0) {
                    pusher.native_audioPush(bytes);
//                    Log.d(TAG,"把音频信息送去编码……");
                }
            }
        }
    }
}