/**
 * description: 跟java中 Pusher.java 思想一样  <br>
 * @author 秦城季
 * @date 2020/12/6
 */

#ifndef RTMPPUSH_PUSHER_H
#define RTMPPUSH_PUSHER_H

#include "AudioTask.h"
#include "VideoTask.h"


static uint32_t start_time = 0;
static SyncQueue<RTMPPacket *> packets;

static void callback(RTMPPacket *packet);

static void release(RTMPPacket *&packet);

static void *pushTask(void *args);


class Pusher {
public:
    Pusher(int fps, int bitRate, int sampleRate, int channels);

    void encodeAudioData(signed char *data);

    void prepare(char *url);

    void stop();

    void videoDataChange(int width, int height);

    void encodeVideoData(int8_t *data);

    u_long audioGetSamples();

public:
    int isReadyPushing = 0;
    char *url = 0;
private:
    pthread_t pid;
    void *audioTask = 0;
    void *videoTask = 0;
};


#endif //RTMPPUSH_PUSHER_H
