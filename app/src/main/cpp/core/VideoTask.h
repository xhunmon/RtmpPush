/**
 * description:   <br>
 * @author 秦城季
 * @date 2020/12/7
 */

#ifndef RTMPPUSH_VIDEOTASK_H
#define RTMPPUSH_VIDEOTASK_H


#include "stdint.h"
#include <x264.h>
#include <cstring>
#include <pthread.h>
#include "utils.h"
#include "../librtmp/rtmp.h"

class VideoTask{
    typedef void (*VideoCallback)(RTMPPacket* packet);
public:
    VideoTask(int fps, int bitRate);
    void setDataCallback(VideoCallback callback);

    void dataChange(int width, int height);

    void encodeData(int8_t *data);
    void sendSpsPps(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len);
    void sendFrame(int type, uint8_t *payload, int i_payload);

private:
    int fps;
    int bitRate;
    int ySize;
    int uvSize;
    VideoCallback callback;
    x264_t *videoCodec = 0;
    x264_picture_t *pic_in = 0;
    pthread_mutex_t mutex;
};


#endif //RTMPPUSH_VIDEOTASK_H
