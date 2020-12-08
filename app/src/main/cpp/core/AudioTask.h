/**
 * description:   <br>
 * @author 秦城季
 * @date 2020/12/7
 */

#ifndef RTMPPUSH_AUDIOTASK_H
#define RTMPPUSH_AUDIOTASK_H


#include <sys/types.h>
#include <faac.h>
#include "sync_queue.h"
#include "../librtmp/rtmp.h"

class AudioTask{
    typedef void (*AudioCallback)(RTMPPacket* packet);
public:
    void init(int sampleRate, int channels);
    void setDataCallback(AudioCallback callback);
    void encodeData(int8_t *data);

    u_long getSamples();

private:
    AudioCallback callback;
    u_long inputSamples;
    faacEncHandle audioCodec = 0;
    u_long maxOutputBytes;
    int channels;
    u_char *buffer = 0;
};


#endif //RTMPPUSH_AUDIOTASK_H
