package com.xhunmon.rtmppush.core;

import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.os.Environment;
import android.util.Log;
import android.view.Surface;
import android.view.SurfaceHolder;

import java.io.File;
import java.io.FileOutputStream;
import java.util.List;

/**
 * description: 处理视频任务  <br>
 *
 * @author 秦城季
 * @date 2020/12/6
 */
public class VideoTask implements SurfaceHolder.Callback, Camera.PreviewCallback {
    private static final String TAG = "VideoTask";
    private SurfaceHolder holder;
    private Camera camera;
    private Pusher pusher;
    int width;
    int height;
    byte[] buffer;
    byte[] bytes;
    private int rotation;
    public static boolean isWriteTest = false;

    public VideoTask(Pusher pusher) {
        this.pusher = pusher;
        holder = pusher.config.surfaceView.getHolder();
        width = pusher.config.width;
        height = pusher.config.height;
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {

    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
        stop();
        preview();
    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {
        stop();
    }


    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        switch (rotation) {
            case Surface.ROTATION_0:
                rotation90(data);
                break;
            case Surface.ROTATION_90: // 横屏 左边是头部(home键在右边)
                Log.d(TAG, "ROTATION_90 横屏 左边是头部(home键在右边)");
                break;
            case Surface.ROTATION_270:// 横屏 头部在右边
                Log.d(TAG, "ROTATION_270 横屏 头部在右边");
                break;
            default:
                Log.d(TAG, "ROTATION_other");
                break;
        }
        if (pusher.isPushing) {//送去编码
            pusher.native_videoPush(bytes);
        }
        //注意：旋转之后宽高需要交互，但是不能直接改变全局变量
        camera.addCallbackBuffer(buffer);
    }

    /**
     * codec中对旋转做了以下处理：
     * <p>
     * // 根据rotation判断transform
     * int transform = 0;
     * if ((rotation % 90) == 0) {
     * switch ((rotation / 90) & 3) {
     * case 1:  transform = HAL_TRANSFORM_ROT_90;  break;
     * case 2:  transform = HAL_TRANSFORM_ROT_180; break;
     * case 3:  transform = HAL_TRANSFORM_ROT_270; break;
     * default: transform = 0;                     break;
     * }
     * }
     * <p>
     * 把歪的图片数据变为正的
     */
    private void rotation90(byte[] simple) {

        int ySize = width * height;
        int uvHeight = height / 2;

        //后置摄像头，顺时针旋转90
        if (pusher.config.facing == Camera.CameraInfo.CAMERA_FACING_BACK) {
            int k = 0;
            //宽高取反，把竖变行
            //y数据
            for (int w = 0; w < width; w++) {
                for (int h = height - 1; h >= 0; h--) {
                    bytes[k++] = simple[h * width + w];
                }
            }
            //uv数据 height*width -> 3/2height*width
            for (int w = 0; w < width; w += 2) {
                for (int h = uvHeight - 1; h >= 0; h--) {
//                *(simpleOut + k) = simple[width * height + h * width + w];
                    // u
                    bytes[k++] = simple[ySize + width * h + w];
                    // v
                    bytes[k++] = simple[ySize + width * h + w + 1];
                }
            }
        } else {
            //前置摄像头，顺时针旋转90
            int k = 0;
            //y数据
            for (int w = width - 1; w >= 0; w--) {
                for (int h = 0; h < height; h++) {
                    bytes[k++] = simple[h * width + w];
                }
            }
            //uv数据 height*width -> 3/2height*width
            for (int w = 0; w < width; w += 2) {
                for (int h = uvHeight - 1; h >= 0; h--) {
//                *(simpleOut + k) = simple[width * height + h * width + w];
                    bytes[k++] = simple[ySize + width * h + w];
                    bytes[k++] = simple[ySize + width * h + w + 1];
                }
            }
        }
    }


    /**
     * TODO 测试代码：把传感器的数据输出到本地，使用ffplay进行查看，命令如下：
     * ffplay -f rawvideo -video_size 800x480 xxx.yuv
     */
    private void testWriteFile(byte[] data) {
        if (!isWriteTest) {
            return;
        }
        isWriteTest = false;
        //yuv21的数据保存到手机打开看一下
        String path = Environment.getExternalStorageDirectory() + File.separator + "yuv_na21_" + width + "x" + height + "_" + System.currentTimeMillis() + ".yuv";
        File file = new File(path);
        try (
                FileOutputStream fos = new FileOutputStream(file);
        ) {
            fos.write(data);
            fos.flush();
        } catch (Exception e) {
            e.printStackTrace();
        }
        Log.d(TAG, "写入结束：" + file.getAbsolutePath());
    }


    public void preview() {
        try {
            holder.removeCallback(this);
            holder.addCallback(this);
            camera = Camera.open(pusher.config.facing);
            //Parameters这里封装着当前摄像头所能提供的参数（真实宽高等）
            Camera.Parameters parameters = camera.getParameters();
            //设置预览数据为nv21（注意：仅仅是预览的数据，通过onPreviewFrame回调的仍没有发生变化）
            parameters.setPreviewFormat(ImageFormat.NV21);
            resetPreviewSize(parameters);
            // 设置摄像头 图像传感器的角度、方向
            setDisplayOrientation();
            camera.setParameters(parameters);
            buffer = new byte[width * height * 3 / 2];
            bytes = new byte[buffer.length];
            //数据缓存区
            camera.addCallbackBuffer(buffer);
            camera.setPreviewCallbackWithBuffer(this);
            //设置预览画面
            camera.setPreviewDisplay(holder);
            camera.startPreview();
        } catch (Exception e) {
            e.printStackTrace();
        }
    }

    /**
     * 通过与用户选择的尺寸与手机所提供的对比，重新设置预览尺寸
     */
    private void resetPreviewSize(Camera.Parameters parameters) {
        List<Camera.Size> sizeList = parameters.getSupportedPreviewSizes();
        Camera.Size preSize = null;
        int dif = 0;
        for (Camera.Size size : sizeList) {
            int tempdif = Math.abs(size.height * size.width - height * width);
            if (preSize == null) {
                preSize = size;
                dif = tempdif;
            } else {
                //计算出与用户选择最接近的宽高size
                if (dif > tempdif) {
                    dif = tempdif;
                    preSize = size;
                }
            }
            Log.d(TAG, "摄像头支持尺寸：w=" + size.width + " | h=" + size.toString());
        }
        if (preSize == null) {
            Log.e(TAG, "改摄像头不提供Size？");
            return;
        }
        Log.d(TAG, "设置摄像头预览宽高：w=" + width + " | h=" + height);
        width = preSize.width;
        height = preSize.height;
        parameters.setPreviewSize(width, height);
        //宽高改变了，通知native进行重新设置参数 需要旋转
        boolean needRotate = rotation == Surface.ROTATION_0;
        pusher.native_videoDataChange(needRotate ? height : width, needRotate ? width : height);
    }

    private void setDisplayOrientation() {
        Camera.CameraInfo info = new Camera.CameraInfo();
        Camera.getCameraInfo(pusher.config.facing, info);
        rotation = pusher.config.activity.getWindowManager().getDefaultDisplay().getRotation();
        int degrees = 0;
        switch (rotation) {
            case Surface.ROTATION_0:
                degrees = 0;
                break;
            case Surface.ROTATION_90:
                degrees = 90;
                break;
            case Surface.ROTATION_180:
                degrees = 180;
                break;
            case Surface.ROTATION_270:
                degrees = 270;
                break;
            default:
                break;
        }
        int result;
        if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
            result = (info.orientation + degrees) % 360;
            result = (360 - result) % 360; // compensate the mirror
        } else { // back-facing
            result = (info.orientation - degrees + 360) % 360;
        }
        //设置角度
        camera.setDisplayOrientation(result);
    }

    public void stop() {
        if (camera != null) {
            camera.setPreviewCallback(null);
            camera.stopPreview();
            camera.release();
            camera = null;
        }
    }
}
